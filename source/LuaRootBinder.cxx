#include "LuaRootBinder.h"
#include "TPad.h"
#include <llimits.h>
#include <csignal>
#include <readline/readline.h>

#include "TUnixSystem.h"
#include "TSysEvtHandler.h"

map<string, string> rootObjectAliases;

map<TObject*, LuaCanvas*> canvasTracker;
mutex syncSafeGuard;
int updateRequestPending;

void sigtstp_handler_stop(int i)
{
	if (i == SIGTSTP)
	{
		signal( SIGTSTP, sigtstp_handler_pause);

		for (auto itr = tasksNames.begin(); itr != tasksNames.end(); itr++)
		{
			PushSignal(itr->second, "stop");
		}

		cout << endl;
	}
}

void sigtstp_handler_pause(int i)
{
	if (i == SIGTSTP)
	{
		signal( SIGTSTP, sigtstp_handler_stop);

		for (auto itr = tasksNames.begin(); itr != tasksNames.end(); itr++)
		{
			PushSignal(itr->second, "wait");
		}

		cout << endl;
	}
}

vector<string> getpossiblefields(string field)
{
	vector<string> autocompletes;

	if (field.empty())
	{
		return autocompletes;
	}

	bool searchFn = false;
	string rootfield = "";

	size_t functionSep = field.find(":");

	if (functionSep != string::npos)
	{
		rootfield = field.substr(0, functionSep);
		TryGetGlobalField(lua, rootfield);
		rootfield += ":";
		searchFn = true;
	}
	else
	{
		size_t lastDot = field.find_last_of(".");

		if (lastDot != string::npos)
		{
			rootfield = field.substr(0, lastDot);
			//         cout << "Checking field " << rootfield  << endl;

			TryGetGlobalField(lua, rootfield);

			rootfield += ".";
		}
		else
		{
			lua_getglobal(lua, "_G");
		}
	}

	if (lua_type(lua, -1) != LUA_TTABLE)
	{
		if (lua_type(lua, -1) != LUA_TNIL)
		{
			lua_getmetatable(lua, -1);
			lua_remove(lua, -2);
		}

		if (lua_type(lua, -1) != LUA_TTABLE)
		{
			lua_pop(lua, 1);
			return autocompletes;
		}
	}

	//     cout << "**********************************Trying to check all the possibilities*****************************" << endl;

	lua_pushnil(lua);

	while (lua_next(lua, -2) != 0)
	{
		if (lua_type(lua, -2) == LUA_TSTRING)
		{
			if (!searchFn || lua_type(lua, -1) == LUA_TFUNCTION)
			{
				//             cout << "Possibility: " << lua_tostring ( lua, -2 ) << endl;
				autocompletes.push_back(rootfield + ((string) lua_tostring(lua, -2)));
			}
		}

		lua_pop(lua, 1);
	}

	lua_pop(lua, 1);

	return autocompletes;
}

char* possibilities_generator(const char *text, int state)
{
	static int list_index, len;
	char *name;

	if (!state)
	{
		list_index = 0;
		len = strlen(text);
	}

	vector<string> autocompletes = getpossiblefields(text);

	char** possibilities = new char*[autocompletes.size() + 1];

	for (unsigned int i = 0; i < autocompletes.size(); i++)
	{
		possibilities[i] = new char[1024];
		sprintf(possibilities[i], "%s", autocompletes[i].c_str());
	}

	possibilities[autocompletes.size()] = NULL;

	while ((name = possibilities[list_index++]))
	{
		if (strncmp(name, text, len) == 0)
		{
			//             cout << "Possibility: " << name << endl;
			return strdup(name);
		}
	}

	return NULL;
}

char** input_completion(const char *text, int start, int end)
{
	rl_completion_append_character = '\0';
	return rl_completion_matches(text, possibilities_generator);
}

mutex rootProcessLoopLock;

RootAppManager* theApp = 0;

RootAppManager::RootAppManager(const char *appClassName, Int_t *argc, char **argv, void *options, Int_t numOptions) :
		TApplication(appClassName, argc, argv, options, numOptions)
{

}

LuaCanvas::LuaCanvas() :
		TCanvas()
{
}

ClassImp(LuaCanvas)

extern "C" int luaopen_libLuaXRootlib(lua_State* L)
{
	luaL_requiref(L, "_G", luaopen_base, 1);
	lua_pop(L, 1);

	lua_getglobal(L, "_G");
	luaL_setfuncs(L, luaXroot_lib, 0);
	lua_pop(L, 1);

	lua_getglobal(L, "_G");
	lua_pop(L, 1);

	InitializeBranchesFuncs(L);

	LuaRegisterSysOpenConsts(L);
	LuaRegisterSemaphoresConsts(L);
	LuaRegisterSocketConsts(L);

	return 0;
}

int CompilePostInit_C(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, false, "CompilePostInit_C", LUA_TTABLE)) return 0;

	lua_getfield(L, 1, "script");

	if (!CheckLuaArgs(L, 1, false, "CompilePostInit_C missing or invalid argument \"script\":", LUA_TTABLE)) return 0;

	const char* scriptname = lua_tostring(L, -1);
	const char* targetLanguage = "++";

	if (lua_checkfield(L, 1, "target", LUA_TSTRING))
	{
		string targetstr = lua_tostring(L, -1);

		if (targetstr == "C") targetLanguage = "";
		else if (targetstr == "C++") targetLanguage = "++";
	}

	char* linecall = new char[2048];

	sprintf(linecall, ".L %s%s", scriptname, targetLanguage);

	gROOT->ProcessLine(linecall);

	return 0;
}

map<string, lua_State*> tasksStates;
map<lua_State*, string> tasksNames;
map<string, string> tasksStatus;
map<string, mutex> tasksMutexes;

map<lua_State*, vector<string>> tasksPendingSignals;

int TasksList_C(lua_State* L)
{
	lua_newtable(L);

	for (auto itr = tasksNames.begin(); itr != tasksNames.end(); itr++)
	{
		string taskname = itr->second;
		lua_pushstring(L, tasksStatus[taskname].c_str());
		lua_setfield(L, -2, taskname.c_str());
	}

	return 1;
}

int LockTaskMutex(lua_State* L)
{
	string mutexName;

	if (CheckLuaArgs(L, 1, false, "", LUA_TSTRING))
	{
		mutexName = lua_tostring(L, 1);
	}
	else
	{
		lua_getglobal(L, "taskName");

		if (CheckLuaArgs(L, -1, true, "LockTaskMutex", LUA_TSTRING)) mutexName = lua_tostring(L, -1);
	}

//	cout << "Trying to acquire lock on mutex " << mutexName << endl;

	tasksMutexes[mutexName].lock();

//	cout << "Locked mutex " << mutexName << endl;

	return 0;
}

int ReleaseTaskMutex(lua_State* L)
{
	string mutexName;

	if (CheckLuaArgs(L, 1, false, "", LUA_TSTRING)) mutexName = lua_tostring(L, 1);
	else
	{
		lua_getglobal(L, "taskName");
		if (CheckLuaArgs(L, -1, true, "ReleaseTaskMutex", LUA_TSTRING)) mutexName = lua_tostring(L, -1);
	}

//	cout << "Releasing mutex " << mutexName << endl;

	tasksMutexes[mutexName].unlock();

	return 0;
}

void* NewTaskFn(void* arg)
{
	//     cout << "Test task setup..." << endl;
	NewTaskArgs* args = (NewTaskArgs*) arg;

	lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	const char* luaXrootPath = std::getenv("LUAXROOTLIBPATH");
	lua_pushstring(L, luaXrootPath);
	lua_setglobal(L, "LUAXROOTLIBPATH");

	string logonFile = (string) luaXrootPath + "/../scripts/luaXrootlogon.lua";
	luaL_dofile(L, logonFile.c_str());

	//     luaopen_libLuaXRootlib ( L );
	if (!args->packages.empty())
	{
		//         cout << "Some additional packages need to be loaded" << endl;

		lua_getglobal(L, "LoadAdditionnalPackages");

		luaL_loadstring(L, (args->packages).c_str());

		lua_pcall(L, 0, LUA_MULTRET, 0);

		if (!CheckLuaArgs(L, -1, true, "NewTaskFn packages table:", LUA_TTABLE))
		{
			return 0;
		}

		lua_pcall(L, 1, LUA_MULTRET, 0);
	}

	//     cout << "Will retrieve arguments..." << endl;

	luaL_loadstring(L, (args->task).c_str());

	lua_pcall(L, 0, LUA_MULTRET, 0);

	if (!CheckLuaArgs(L, -1, true, "NewTaskFn", LUA_TTABLE))
	{
		return 0;
	}

	lua_getfield(L, -1, "name");
	string taskname = lua_tostring(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, taskname.c_str());
	lua_setglobal(L, "taskName");

	tasksStates[taskname] = L;
	tasksNames[L] = taskname;
	tasksStatus[taskname] = "waiting";

	lua_getfield(L, -1, "taskfn");

	if (lua_type(L, -1) == LUA_TSTRING)
	{
		const char* taskfn_name = lua_tostring(L, -1);
		lua_pop(L, 1);
		//         cout << "Task given as string: " << taskfn_name << endl;
		TryGetGlobalField(L, taskfn_name);
	}

	if (!CheckLuaArgs(L, -1, true, "NewTaskFn", LUA_TFUNCTION))
	{
		return 0;
	}

	//     cout << "Task is valid. Getting arguments..." << endl;

	lua_getfield(L, -2, "args");

	if (!CheckLuaArgs(L, -1, true, "NewTaskFn", LUA_TTABLE))
	{
		return 0;
	}

	int nargs = lua_rawlen(L, -1);

	int argsStackPos = lua_gettop(L);

	for (int i = 0; i < nargs; i++)
	{
		lua_geti(L, argsStackPos, i + 1);
	}

	lua_remove(L, argsStackPos);

	tasksMutexes[taskname].lock();

	tasksStatus[taskname] = "running";

	lua_pcall(L, nargs, LUA_MULTRET, 0);

	tasksMutexes[taskname].unlock();

	tasksStatus[taskname] = "done";

	//     cout << "Task complete" << endl;

	return nullptr;
}

int StartNewTask_C(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "StartNewTask_C", LUA_TSTRING))
	{
		return 0;
	}

	if (lua_gettop(L) > 1 && !CheckLuaArgs(L, 2, true, "StartNewTask_C", LUA_TSTRING))
	{
		return 0;
	}

	NewTaskArgs* args = new NewTaskArgs;

	args->task = lua_tostring(L, 1);
	args->packages = "";

	if (lua_gettop(L) > 1)
	{
		args->packages = lua_tostring(L, 2);
	}

	pthread_t newTask;

	pthread_create(&newTask, nullptr, NewTaskFn, (void*) args);

	return 0;
}

void MakeSyncSafe(bool apply)
{
	if (theApp->safeSync) syncSafeGuard.unlock();

	if (apply)
	{
		theApp->shouldStop = true;
		theApp->NotifyUpdatePending();
		syncSafeGuard.lock();
		theApp->safeSync = true;
		theApp->NotifyUpdateDone();
		//         cout << "Setting up sync safetey mutex..." << endl;
	}
}

int LuaMakeSyncSafe(lua_State*L)
{
	bool apply = true;

	if (CheckLuaArgs(L, 1, false, "", LUA_TBOOLEAN))
	{
		apply = lua_toboolean(L, 1);
	}

	MakeSyncSafe(apply);

	return 0;
}

int GetTaskStatus(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "GetTaskStatus", LUA_TSTRING))
	{
		return 0;
	}

	string taskname = lua_tostring(L, 1);

	lua_pushstring(L, tasksStatus[taskname].c_str());

	return 1;
}

void PushSignal(string taskname, string signal)
{
	vector<string>* sigs = &tasksPendingSignals[tasksStates[taskname]];

	//     cout << "Sending signal " << signal << " to " << taskname << endl;

	if (signal == "stop" || signal == "wait" || signal == "resume") sigs->clear();

	sigs->push_back(signal);
}

int SendSignal_C(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "SendSignal_C", LUA_TSTRING, LUA_TSTRING))
	{
		return 0;
	}

	string taskname = lua_tostring(L, 1);
	string signal = lua_tostring(L, 2);

	PushSignal(taskname, signal);

	lua_pushboolean(L, true);

	return 1;
}

int CheckSignals_C(lua_State* L)
{
	vector<string>* sigs = &tasksPendingSignals[L];
	unsigned int nsigs = sigs->size();

	if (nsigs > 0)
	{
//         cout << nsigs << " pending signals for " << tasksNames[L] <<" ..." << endl;
		for (unsigned int i = 0; i < nsigs; i++)
		{
			//             cout << "Treating signal ";
			if (i == 0 && sigs->at(i) == "stop")
			{
//                 cout << "stop" << endl;
				sigs->clear();

				MakeSyncSafe(false);

				return 0;
			}
			if (i == 0 && sigs->at(i) == "wait")
			{
//                 cout << "wait" << endl;
				tasksStatus[tasksNames[L]] = "suspended";

				MakeSyncSafe(false);

				while (sigs->at(i) == "wait")
				{
					sleep(1);
				}

				MakeSyncSafe(true);

				return CheckSignals_C(L);
			}
			if (i == 0 && sigs->at(i) == "resume")
			{
				tasksStatus[tasksNames[L]] = "running";
				sigs->clear();
				signal( SIGTSTP, sigtstp_handler_pause);
				return 1;
			}
			else
			{
//                 cout << sigs->at ( i ) << endl;
				lua_getglobal(L, "ProcessSignal");
				lua_pushstring(L, sigs->at(i).c_str());
				lua_pcall(L, 1, LUA_MULTRET, 0);
			}
		}
	}
//     else cout << "No pending signals" << endl;

	sigs->clear();
	lua_pushboolean(L, 1);

	return 1;
}

// -------------------------------------------------- TApplication Binder -------------------------------------------------------- //

void* StartRootEventProcessor(void* arg)
{
	updateRequestPending = 0;

	theApp->SetupUpdateSignalReceiver();

	restartruntask:

//	if (theApp->shouldStop) rootProcessLoopLock.unlock();

	theApp->shouldStop = false;

	if (theApp->safeSync)
	{
		//         cout << "trying to acquire lock on sync mutex" << endl;
		syncSafeGuard.lock();
		//         cout << "lock acquired and now releasing it..." << endl;
		syncSafeGuard.unlock();
		theApp->safeSync = false;
	}

	rootProcessLoopLock.lock();

	while (!theApp->shouldStop)
	{
//		cout << "Restarting InnerLoop" << endl;
		theApp->StartIdleing();
		gSystem->InnerLoop();
//         cout << "Exiting InnerLoop" << endl;
		theApp->StopIdleing();
	}

//	cout << "Exiting main loop..." << endl;

	rootProcessLoopLock.unlock();

	while (updateRequestPending > 0)
	{
		//         cout << "Waiting for updates: " << updateRequestPending << endl;
		gSystem->Sleep(50);
	}

//	cout << "All Updates have been treated" << endl;

//     rootProcessLoopLock.lock();

//	cout << "theApp acquired master lock" << endl;

	gSystem->Sleep(50);

	goto restartruntask;

	return nullptr;
}

int luaExt_NewTApplication(lua_State* L)
{
	if (theApp == nullptr)
	{
		//         cout << "theApp == nullptr, initializing it..." << endl;

		signal( SIGTSTP, sigtstp_handler_pause);

		sprintf(homeDir, "%s", getenv("HOME"));
		sprintf(histPath, "%s/.luaXrootHist", homeDir);
		read_history(histPath);

		lua = L;
		rl_attempted_completion_function = input_completion;

		RootAppManager** tApp = NewUserData<RootAppManager>(L, "tapp", nullptr, nullptr);

		theApp = *tApp;

		theApp->SetupUpdateSignalSender();

		theApp->SetupRootAppMetatable(L);

		lua_newtable(L);
		lua_pushinteger(L, 0);
		lua_setfield(L, -2, "unnamed");
		lua_setglobal(L, "_stdfunctions");

		gSystem->AddDynamicPath("${LUAXROOTLIBPATH}");
		gSystem->AddIncludePath("-I/usr/include/readline");
		gSystem->AddIncludePath("-I$LUAXROOTLIBPATH/../include");
		gSystem->AddIncludePath("-I$LUAXROOTLIBPATH/../lua");
		gSystem->Load("libguilereadline-v-18.so");
		gSystem->Load("liblualib.so");
		gSystem->Load("libLuaXRootlib.so");
		gSystem->Load("libRootBinderLib.so");

		pthread_t startRootEventProcessrTask;

		pthread_create(&startRootEventProcessrTask, nullptr, StartRootEventProcessor, nullptr);

		theApp->WaitForUpdateReceiver();

		return 1;
	}
	else
	{
		return 0;
	}
}

int luaExt_TApplication_Run(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "luaExt_TApplication_Run", LUA_TUSERDATA)) return 0;

	TApplication* tApp = *(reinterpret_cast<TApplication**>(lua_touserdata(L, 1)));

	tApp->Run();

	return 0;
}

int luaExt_TApplication_Update(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "luaExt_TApplication_Update", LUA_TUSERDATA)) return 0;

	TApplication* tApp = *(reinterpret_cast<TApplication**>(lua_touserdata(L, 1)));

	while (updateRequestPending > 0)
	{
		//         cout << "Waiting for updates: " << updateRequestPending << endl;
		gSystem->Sleep(50);
	}

	theApp->NotifyUpdatePending();

	TTimer* innerloop_timer = new TTimer(2000);
	gSystem->AddTimer(innerloop_timer);

	//     cout << "Force update theApp" << endl;
	tApp->StartIdleing();
	gSystem->InnerLoop();
	tApp->StopIdleing();
	//     cout << "Force update theApp done" << endl;

	gSystem->RemoveTimer(innerloop_timer);

	theApp->NotifyUpdateDone();

	return 0;
}

int luaExt_TApplication_Terminate(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "luaExt_TApplication_Terminate", LUA_TUSERDATA)) return 0;

	for (auto itr = socketsList.begin(); itr != socketsList.end(); itr++)
	{
		unlink(itr->second.address.c_str());
		close(itr->first);
	}

	for (auto itr = canvasTracker.begin(); itr != canvasTracker.end(); itr++)
		itr->second->Close();

	TApplication* tApp = *(reinterpret_cast<TApplication**>(lua_touserdata(L, 1)));

	tApp->Terminate();

	return 0;
}

// ------------------------------------------------------ TF1 Binder -------------------------------------------------------------- //

map<string, function<double(double*, double*)>> registeredTF1fns;

int RegisterTF1fn(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "RegisterTF1fn", LUA_TSTRING, LUA_TFUNCTION, LUA_TNUMBER) || lua_gettop(L) != 3) return 0;

	string fname = lua_tostring(L, 1);
	lua_remove(L, 1);

	int npars = lua_tointeger(L, -1);
	lua_pop(L, 1);

	string fn_gfield = "_stdfunctions." + fname;
	TrySetGlobalField(L, fn_gfield);

	function<double(double*, double*)> fn = [=] ( double* x, double* p )
	{
		TryGetGlobalField ( L, fn_gfield );

		if ( lua_type ( L, -1 ) != LUA_TFUNCTION )
		{
			cerr << "ERROR: failed to retrieve the global field " << fn_gfield << endl;
			lua_settop ( L, 0 );
			return 0.0;
		}

		lua_pushnumber ( L, x[0] );

		for ( int i = 0; i < npars; i++ )
		{
			lua_pushnumber ( L, p[i] );
		}

		lua_pcall ( L, npars+1, 1, 0 );

		double res = lua_tonumber ( L, -1 );
		lua_settop(L, 0); // Required to not flood the console... When moving the mouse over the TCanvas, each mouse movement call this function and push the result on the stack!

			return res;
		};

	registeredTF1fns[fname] = fn;

	return 0;
}

// ------------------------------------------------------ TTree Binder ----------------------------------------------------------- //

map<string, function<void(lua_State*, TTree*, const char*)>> newBranchFns;

const char* GetROOTLeafId(string btype)
{
	const char* leafTypeId = "";

	if (btype == "bool") leafTypeId = "O";
	else if (btype == "unsigned short") leafTypeId = "s";
	else if (btype == "short") leafTypeId = "S";
	else if (btype == "unsigned int") leafTypeId = "i";
	else if (btype == "int") leafTypeId = "I";
	else if (btype == "unsigned long") leafTypeId = "i";
	else if (btype == "long") leafTypeId = "I";
	else if (btype == "unsigned long long") leafTypeId = "l";
	else if (btype == "long long") leafTypeId = "L";
	else if (btype == "float") leafTypeId = "F";
	else if (btype == "double") leafTypeId = "D";

	return leafTypeId;
}

template<typename T> void MakeNewBranchFuncs(string type)
{
	string finalType = type;

	newBranchFns[finalType] = [=] ( lua_State* L, TTree* tree, const char* bname )
	{
		LuaCtor(L, -1);
		lua_remove(L, -2);
		T* branch_ptr = GetUserData<T> ( L, -1, "luaExt_TTree_NewBranch" );
		tree->Branch ( bname, branch_ptr );
	};

	finalType = "vector<" + type + ">";

	newBranchFns[finalType] = [=] ( lua_State* L, TTree* tree, const char* bname )
	{
		LuaCtor(L, -1);
		lua_remove(L, -2);
		vector<T>* branch_ptr = GetUserData<vector<T>> ( L, -1, "luaExt_TTree_NewBranch" );
		tree->Branch ( bname, branch_ptr );
	};

	finalType = "vector<vector<" + type + ">>";

	newBranchFns[finalType] = [=] ( lua_State* L, TTree* tree, const char* bname )
	{
		LuaCtor(L, -1);
		lua_remove(L, -2);
		vector<vector<T>>* branch_ptr = GetUserData<vector<vector<T>>> ( L, -1, "luaExt_TTree_NewBranch" );
		tree->Branch ( bname, branch_ptr );
	};

	finalType = type + "[]";

	newBranchFns[finalType] = [=] ( lua_State* L, TTree* tree, const char* bname )
	{
		LuaCtor(L, -1);
		lua_remove(L, -2);
		lua_getfield ( L, -1, "array_size" );
		int arraySize = lua_tointeger ( L, -1 );
		lua_pop ( L, 1 );
		T* branch_ptr = GetUserData<T> ( L, -1, "luaExt_TTree_NewBranch" );
		string leafList = bname;

		leafList += "[" + to_string ( arraySize ) + "]/" + GetROOTLeafId ( type );
		cout << "array : " << branch_ptr << endl;
		tree->Branch ( bname, branch_ptr, leafList.c_str() );
	};
}

template<typename T> void SetupBranchFuncs(lua_State* L, string type)
{
	MakeNewBranchFuncs<T>(type);

	MakeAccessorsUserDataFuncs<T>(L, type);
}

void InitializeBranchesFuncs(lua_State* L)
{
	SetupBranchFuncs<bool>(L, "bool");
	SetupBranchFuncs<short>(L, "short");
	SetupBranchFuncs<unsigned short>(L, "unsigned short");
	SetupBranchFuncs<int>(L, "int");
	SetupBranchFuncs<unsigned int>(L, "unsigned int");
	SetupBranchFuncs<long>(L, "long");
	SetupBranchFuncs<unsigned long>(L, "unsigned long");
	SetupBranchFuncs<long long>(L, "long long");
	SetupBranchFuncs<unsigned long long>(L, "unsigned long long");
	SetupBranchFuncs<float>(L, "float");
	SetupBranchFuncs<double>(L, "double");
}

