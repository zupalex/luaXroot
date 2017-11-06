#ifndef __LUAROOTBINDER__
#define __LUAROOTBINDER__

#include <streambuf>
#include <pthread.h>
#include <stdio.h>
#include <mutex>

#include "TSystem.h"
#include "TROOT.h"
#include "TApplication.h"
#include "TCanvas.h"
#include "TQObject.h"
#include "TF1.h"
#include "TH2.h"
#include "TH1.h"
#include "TGraphErrors.h"
#include "TCutG.h"
#include "TFile.h"
#include "TTree.h"
#include "TTimer.h"
#include "TClonesArray.h"

#include "LuaSocketBinder.h"
#include <llimits.h>

extern map<TObject*, TCanvas*> canvasTracker;

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

// --------------------------------- TApplication Binder---------------------------- //

int luaExt_NewTApplication(lua_State* L);
int luaExt_TApplication_Run(lua_State* L);
int luaExt_TApplication_Update(lua_State* L);
int luaExt_TApplication_Terminate(lua_State* L);

// --------------------------------------------------------------------------------- //

class RootAppManager: public TApplication {
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

		lua_pushcfunction(L, luaExt_TApplication_Terminate);
		lua_setfield(L, -2, "Terminate");
	}

	void KillApp()
	{
		cout << "Received signal to kill the app" << endl;
		gSystem->ExitLoop();
	}

	void OnCanvasKilled()
	{
		cout << "A Canvas Has Been KILLED!" << endl;
		this->shouldStop = true;
	}

	void SetupUpdateSignalSender()
	{
		msg_fd = socket( AF_UNIX, SOCK_STREAM, 0);

		remove("/tmp/.luaXroot_msgqueue");

		sockaddr_un addr;
		addr.sun_family = AF_UNIX;
		strcpy(addr.sun_path, "/tmp/.luaXroot_msgqueue");

		if (bind(msg_fd, (sockaddr*) &addr, sizeof(addr)) < 0)
		{
			cerr << "Error opening socket" << endl;
		}

		listen(msg_fd, 1);
	}

	void WaitForUpdateReceiver()
	{

		sockaddr_storage clients_addr;
		socklen_t addr_size;

		msg_fd = accept(msg_fd, (sockaddr*) &clients_addr, &addr_size);
	}

	void SetupUpdateSignalReceiver()
	{
		rcv_fd = socket( AF_UNIX, SOCK_STREAM, 0);

		sockaddr_un addr;
		addr.sun_family = AF_UNIX;
		strcpy(addr.sun_path, "/tmp/.luaXroot_msgqueue");

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

	bool shouldStop = false;
	bool safeSync = false;

	int msg_fd = -1;
	int rcv_fd = -1;

ClassDef ( RootAppManager, 1 )
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

class LuaCanvas: public TCanvas {
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

ClassDef ( LuaCanvas, 1 )
};

// ----------------------------------------------Helper Functions -------------------------------------------------------------------- //

extern map<string, lua_State*> tasksStates;
extern map<lua_State*, string> tasksNames;
extern map<string, string> tasksStatus;
extern map<string, mutex> tasksMutexes;

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

void PushSignal(string taskname, string signal);

int SendSignal_C(lua_State* L);
int CheckSignals_C(lua_State* L);

int CompilePostInit_C(lua_State* L);

// ------------------------------------------------------- TObject Binder ------------------------------------------------------- //

int luaExt_NewTObject(lua_State* L);
int luaExt_TObject_Write(lua_State* L);
int luaExt_TObject_Draw(lua_State* L);
int luaExt_TObject_Update(lua_State* L);
int luaExt_TObject_GetName(lua_State* L);
int luaExt_TObject_GetTitle(lua_State* L);

// ------------------------------------------------------ TF1 Binder -------------------------------------------------------------- //

int luaExt_NewTF1(lua_State* L);
int luaExt_TF1_SetParameters(lua_State* L);
int luaExt_TF1_Eval(lua_State* L);
int luaExt_TF1_GetPars(lua_State* L);
int luaExt_TF1_GetChi2(lua_State* L);

// -------------------------------------------------- TH[istograms] Binder -------------------------------------------------------- //

int luaExt_NewTHist(lua_State* L);
int luaExt_THist_Clone(lua_State* L);
int luaExt_THist_Fill(lua_State* L);
int luaExt_THist_Add(lua_State* L);
int luaExt_THist_Scale(lua_State* L);
int luaExt_THist_SetRangeUser(lua_State* L);
int luaExt_THist_Fit(lua_State* L);
int luaExt_THist_Reset(lua_State* L);

// -------------------------------------------------- TGraphErrors Binder -------------------------------------------------------- //

int luaExt_NewTGraph(lua_State* L);
int luaExt_TGraph_SetTitle(lua_State* L);
int luaExt_TGraph_Fit(lua_State* L);
int luaExt_TGraph_SetPoint(lua_State* L);
int luaExt_TGraph_GetPoint(lua_State* L);
int luaExt_TGraph_RemovePoint(lua_State* L);
int luaExt_TGraph_SetNPoints(lua_State* L);
int luaExt_TGraph_Eval(lua_State* L);

// ------------------------------------------------------ TFile Binder ----------------------------------------------------------- //

int luaExt_NewTFile(lua_State* L);
int luaExt_TFile_Close(lua_State* L);
int luaExt_TFile_cd(lua_State* L);
int luaExt_TFile_ls(lua_State* L);
int luaExt_TFile_Get(lua_State* L);

// ------------------------------------------------------ TCutG Binder ----------------------------------------------------------- //

int luaExt_NewTCutG(lua_State* L);
int luaExt_TCutG_IsInside(lua_State* L);

// ------------------------------------------------------ TClonesArray Binder ----------------------------------------------------------- //

int luaExt_NewTClonesArray(lua_State* L);
int luaExt_TClonesArray_ConstructedAt(lua_State* L);

// ------------------------------------------------------ TTree Binder ----------------------------------------------------------- //

extern map<string, function<void(lua_State*, TTree*, const char*)>> newBranchFns;

int luaExt_NewTTree(lua_State* L);
int luaExt_TTree_Fill(lua_State* L);
int luaExt_TTree_GetEntries(lua_State* L);
int luaExt_TTree_Draw(lua_State* L);
int luaExt_TTree_GetEntry(lua_State* L);

int luaExt_TTree_NewBranch_Interface(lua_State* L);
int luaExt_TTree_GetBranch_Interface(lua_State* L);

void InitializeBranchesFuncs(lua_State* L);

static const luaL_Reg luaTTreeBranchFns[] =
	{
		{ "NewBranchInterface", luaExt_TTree_NewBranch_Interface },
		{ "GetBranchInterface", luaExt_TTree_GetBranch_Interface },

		{ NULL, NULL } };

// --------------------------------------------- ROOT Object API Helper Functions ------------------------------------------------ //

inline void SetupTObjectMetatable(lua_State* L)
{
	MakeMetatable(L);

	AddMethod(L, luaExt_TObject_Write, "Write");
	AddMethod(L, luaExt_TObject_GetName, "GetName");
	AddMethod(L, luaExt_TObject_GetTitle, "GetTitle");
	AddMethod(L, luaExt_TObject_Draw, "Draw");
	AddMethod(L, luaExt_TObject_Update, "Update");
}

// --------------------------------------------------- Lua Library export -------------------------------------------------------- //

static const luaL_Reg luaXroot_lib[] =
	{
		{ "TasksList_C", TasksList_C },
		{ "StartNewTask_C", StartNewTask_C },
		{ "MakeSyncSafe", LuaMakeSyncSafe },
		{ "LockTaskMutex", LockTaskMutex },
		{ "ReleaseTaskMutex", ReleaseTaskMutex },
		{ "GetTaskStatus", GetTaskStatus },
		{ "SendSignal_C", SendSignal_C },
		{ "CheckSignals_C", CheckSignals_C },
		{ "CompilePostInit_C", CompilePostInit_C },

		{ "TApplication", luaExt_NewTApplication },
		{ "GetTheApp", GetTheApp },

		{ "TObject", luaExt_NewTObject },

		{ "TFile", luaExt_NewTFile },

		{ "TF1", luaExt_NewTF1 },
		{ "THist", luaExt_NewTHist },
		{ "TGraph", luaExt_NewTGraph },
		{ "TCutG", luaExt_NewTCutG },
		{ "TClonesArray", luaExt_NewTClonesArray },

		{ "TTree", luaExt_NewTTree },

		{ "appendtohist", appendtohist },
		{ "saveprompthistory", saveprompthistory },
		{ "trunctehistoryfile", trunctehistoryfile },
		{ "clearprompthistory", clearprompthistory },

		{ "_ctor", luaExt_Ctor },

	// SOCKETS BINDING //

				{ "SysOpen", LuaSysOpen },
				{ "SysClose", LuaSysClose },
				{ "SysUnlink", LuaSysUnlink },
				{ "SysRead", LuaSysRead },
				{ "SysWrite", LuaSysWrite },
				{ "SysDup", LuaSysDup },
				{ "SysDup2", LuaSysDup2 },
				{ "SysFork", LuaSysFork },
				{ "SysExec", LuaSysExecvpe },
				{ "GetEnv", LuaGetEnv },

				{ "MakePipe", MakePipe },
				{ "MakeFiFo", MakeFiFo },
				{ "NewSocket", LuaNewSocket },

				{ "SocketBind", LuaSocketBind },
				{ "SocketConnect", LuaSocketConnect },

				{ "SysSelect", LuaSysSelect },

				{ "SocketListen", LuaSocketListen },
				{ "SocketAccept", LuaSocketAccept },

				{ "SocketReceive", LuaSocketReceive },
				{ "SocketSend", LuaSocketSend },

				{ NULL, NULL } };

extern "C" int luaopen_libLuaXRootlib(lua_State* L);

#endif

