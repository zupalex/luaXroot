#ifndef __LUAROOTBINDER__
#define __LUAROOTBINDER__

#include <streambuf>
#include <pthread.h>
#include <stdio.h>
#include <mutex>
#include <llimits.h>
#include <LuaMMap.h>

#include "TSystem.h"
#include "TROOT.h"
#include "TApplication.h"
#include "TCanvas.h"
#include "TQObject.h"
#include "TF1.h"
#include "TH2.h"
#include "TH1.h"
#include "TSpectrum.h"
#include "TGraphErrors.h"
#include "TCutG.h"
#include "TFile.h"
#include "TTree.h"
#include "TTimer.h"
#include "TClonesArray.h"

#include "LuaMsgq.h"
#include "LuaSemaphoresBinder.h"
#include "LuaShMem.h"
#include "LuaMMap.h"
#include "LuaSocketBinder.h"

class LuaCanvas;

extern map<TObject*, LuaCanvas*> canvasTracker;

extern mutex rootProcessLoopLock;
extern mutex syncSafeGuard;
extern int updateRequestPending;

void sigtstp_handler_stop(int signal);
void sigtstp_handler_pause(int signal);

// ------------------------------------------------------------------------------------------------------------------ //
// ------------------------------------------------------------------------------------------------------------------ //
// ------------------------------------------ RootAppManager -------------------------------------------------------- //
// ------------------------------------------------------------------------------------------------------------------ //
// ------------------------------------------------------------------------------------------------------------------ //

int luaExt_GetTaskName(lua_State* L);

// --------------------------------- TApplication Binder---------------------------- //

int luaExt_NewTApplication(lua_State* L);
int luaExt_TApplication_Run(lua_State* L);
int luaExt_TApplication_Update(lua_State* L);
int luaExt_TApplication_ProcessEvents(lua_State* L);
int luaExt_TApplication_Terminate(lua_State* L);

// --------------------------------------------------------------------------------- //

class RootAppManager : public TApplication {
	private:

	public:
		RootAppManager(const char *appClassName, Int_t *argc, char **argv, void *options = 0, Int_t numOptions = 0);
		virtual ~RootAppManager()
		{
		}

		void SetupRootAppMetatable(lua_State* L)
		{
			MakeMetatable(L);

			lua_pushcfunction(L, luaExt_TApplication_Run);
			lua_setfield(L, -2, "Run");

			lua_pushcfunction(L, luaExt_TApplication_Update);
			lua_setfield(L, -2, "Update");

			lua_pushcfunction(L, luaExt_TApplication_ProcessEvents);
			lua_setfield(L, -2, "ProcessEvents");

			lua_pushcfunction(L, luaExt_TApplication_Terminate);
			lua_setfield(L, -2, "Terminate");
		}

		void SetupUpdateSignalSender()
		{
			msgq_address = "/tmp/.luaXroot" + to_string(getpid()) + "_msgqueue";
			open(msgq_address.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0777);

			msg_fd = socket( AF_UNIX, SOCK_STREAM, 0);

			remove(msgq_address.c_str());

			int yes = 1;
			if (setsockopt(msg_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1)
			{
				perror("setsockopt");
				exit(1);
			}

			sockaddr_un addr;
			memset(&addr, 0, sizeof(addr));

			addr.sun_family = AF_UNIX;
			strcpy(addr.sun_path, msgq_address.c_str());

			if (bind(msg_fd, (sockaddr*) &addr, sizeof(addr)) < 0)
			{
				cerr << "Error opening ROOT sync socket:" << errno << endl;
			}

			listen(msg_fd, 1);
		}

		void WaitForUpdateReceiver()
		{
			sockaddr_in clients_addr;
			socklen_t addr_size;

			memset(&clients_addr, 0, sizeof(clients_addr));
			memset(&addr_size, 0, sizeof(addr_size));

			msg_fd = accept(msg_fd, (sockaddr*) &clients_addr, &addr_size);
			if (msg_fd == -1)
			{
				cerr << "Error accepting ROOT sync socket:" << errno << endl;
			}
		}

		void SetupUpdateSignalReceiver()
		{
			rcv_fd = socket( AF_UNIX, SOCK_STREAM, 0);

			sockaddr_un addr;
			memset(&addr, 0, sizeof(addr));

			addr.sun_family = AF_UNIX;
			strcpy(addr.sun_path, msgq_address.c_str());

			connect(rcv_fd, (sockaddr*) &addr, sizeof(sockaddr_un));

			gSystem->AddFileHandler(new TFileHandler(rcv_fd, 1));
		}

		void NotifyUpdatePending()
		{
			updateRequestPending++;
			shouldStop = true;
			const char* msg = "update";
			if (send(msg_fd, (void*) msg, 6, 0) == -1) cerr << "Failed to send update message:" << errno << endl;
			rootProcessLoopLock.lock();
		}

		void NotifyUpdateDone()
		{
			updateRequestPending--;
			char* buffer = new char[6];
			if (recv(rcv_fd, buffer, 6, 0) == -1) cerr << "Failed to receive update message" << errno << endl;
			rootProcessLoopLock.unlock();
		}

		void CanvasClosed()
		{
			cout << "canvas closed" << endl;
		}

		bool shouldStop = false;
		bool safeSync = false;

		int msg_fd = -1;
		int rcv_fd = -1;

		string msgq_address;

	ClassDef(RootAppManager, 1)
};

extern RootAppManager* theApp;

inline int GetTheApp(lua_State* L)
{
	RootAppManager** root_app = NewUserData<RootAppManager>(L, theApp);

	(*root_app)->SetupRootAppMetatable(L);

	return 1;
}

// ------------------------------------------------------------------------------------------------------------------ //
// ------------------------------------------------------------------------------------------------------------------ //
// --------------------------------------------- LuaCanvas ---------------------------------------------------------- //
// ------------------------------------------------------------------------------------------------------------------ //
// ------------------------------------------------------------------------------------------------------------------ //

static map<string, bool> drawableROOTClasses;

class LuaCanvas : public TCanvas {
	private:

	public:
		LuaCanvas();

		virtual ~LuaCanvas()
		{
			HasBeenKilled();

			for (auto itr = canvasTracker.begin(); itr != canvasTracker.end(); itr++)
			{
				if (itr->second == this)
				{
					canvasTracker.erase(itr);
					break;
				}
			}
		}

		void HasBeenKilled()
		{
//         cout << "Emitting signal to kill the app" << endl;
			Emit("HasBeenKilled()");
		}

		void RequestMasterUpdate()
		{
			Emit("RequestMasterUpdate()");
		}

		void HandleInput(EEventType event, int px, int py);

	ClassDef(LuaCanvas, 1)
};

// ----------------------------------------------Helper Functions -------------------------------------------------------------------- //

extern map<string, lua_State*> tasksStates;
extern map<lua_State*, string> tasksNames;
extern map<string, string> tasksStatus;
extern map<string, mutex> tasksMutexes;

extern map<lua_State*, vector<string>> tasksControlSignals;
extern map<lua_State*, vector<string>> tasksPendingSignals;

struct NewTaskArgs {
		string task;
		string packages;
};

int TasksList_C(lua_State* L);
int StartNewTask_C(lua_State* L);

int LockTaskMutex(lua_State* L);
int ReleaseTaskMutex(lua_State* L);

void MakeSyncSafe(bool apply);
int LuaMakeSyncSafe(lua_State*L);

int GetTaskStatus(lua_State* L);

void PushSignal(string taskname, string signal, bool wipequeue = false);

int SendSignal_C(lua_State* L);
int CheckSignals_C(lua_State* L);

int CompilePostInit_C(lua_State* L);

extern map<string, string> rootObjectAliases;

// ------------------------------------------------------ TF1 Binder -------------------------------------------------------------- //

extern map<string, function<double(double*, double*)>> registeredTF1fns;

int RegisterTF1fn(lua_State* L);

// ------------------------------------------------------ TFile Binder ----------------------------------------------------------- //

int luaExt_NewTFile(lua_State* L);
int luaExt_TFile_Close(lua_State* L);
int luaExt_TFile_cd(lua_State* L);
int luaExt_TFile_ls(lua_State* L);
int luaExt_TFile_Get(lua_State* L);

// ------------------------------------------------------ TTree Binder ----------------------------------------------------------- //

extern map<string, function<void(lua_State*, TTree*, const char*)>> newBranchFns;

int luaExt_NewTTree(lua_State* L);
int luaExt_TTree_Fill(lua_State* L);
int luaExt_TTree_GetEntries(lua_State* L);
int luaExt_TTree_Draw(lua_State* L);
int luaExt_TTree_GetEntry(lua_State* L);

void InitializeBranchesFuncs(lua_State* L);

// --------------------------------------------- ROOT Object API Helper Functions ------------------------------------------------ //

inline void SetupTObjectMetatable(lua_State* L)
{
	MakeMetatable(L);
}

// --------------------------------------------------- Lua Library export -------------------------------------------------------- //

static const luaL_Reg luaXroot_lib[] =
	{
		{ "GetClockTime", luaExt_gettime },
		{ "sleep", luaExt_nanosleep },
		{ "SizeOf", luaExt_GetUserDataSize },

		{ "TasksList_C", TasksList_C },
		{ "StartNewTask_C", StartNewTask_C },
		{ "GetTaskName", luaExt_GetTaskName },
		{ "MakeSyncSafe", LuaMakeSyncSafe },
		{ "LockTaskMutex", LockTaskMutex },
		{ "ReleaseTaskMutex", ReleaseTaskMutex },
		{ "GetTaskStatus", GetTaskStatus },
		{ "SendSignal_C", SendSignal_C },
		{ "CheckSignals_C", CheckSignals_C },
		{ "CompilePostInit_C", CompilePostInit_C },

		{ "TApplication", luaExt_NewTApplication },
		{ "GetTheApp", GetTheApp },

		{ "RegisterTF1fn", RegisterTF1fn },

		{ "appendtohist", appendtohist },
		{ "saveprompthistory", saveprompthistory },
		{ "trunctehistoryfile", trunctehistoryfile },
		{ "clearprompthistory", clearprompthistory },

		{ "_ctor", luaExt_Ctor },

		{ NULL, NULL } };

extern "C" int luaopen_libLuaXRootlib(lua_State* L);

#endif

