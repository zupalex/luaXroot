#ifndef __LUAROOTBINDER__
#define __LUAROOTBINDER__

#include <iostream>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <random>
#include <type_traits>
#include <cxxabi.h>
#include <pthread.h>
#include <dirent.h>
#include <utility>
#include <tuple>
#include <array>
#include <chrono>
#include <ctime>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <list>
#include <functional>
#include <cassert>
#include <algorithm>
#include <type_traits>
#include <mutex>

#include "TSystem.h"
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

#include "LuaExtension.h"
#include <llimits.h>

extern map<TObject*, TCanvas*> canvasTracker;

extern mutex syncSafeGuard;
extern int updateRequestPending;

// ---------------------------------------------------- TApplication Binder ------------------------------------------------------ //

int luaExt_NewTApplication ( lua_State* L );
int luaExt_TApplication_Run ( lua_State* L );
int luaExt_TApplication_Update ( lua_State* L );
int luaExt_TApplication_Terminate ( lua_State* L );

// ------------------------------------------------------- TObject Binder ------------------------------------------------------- //

int luaExt_NewTObject ( lua_State* L );
int luaExt_TObject_Draw ( lua_State* L );
int luaExt_TObject_Update ( lua_State* L );

// ------------------------------------------------------ TF1 Binder -------------------------------------------------------------- //

int luaExt_NewTF1 ( lua_State* L );
int luaExt_TF1_SetParameters ( lua_State* L );
int luaExt_TF1_Eval ( lua_State* L );
int luaExt_TF1_GetPars ( lua_State* L );
int luaExt_TF1_GetChi2 ( lua_State* L );

// -------------------------------------------------- TH[istograms] Binder -------------------------------------------------------- //

int luaExt_NewTHist ( lua_State* L );
int luaExt_THist_Clone ( lua_State* L );
int luaExt_THist_Fill ( lua_State* L );
int luaExt_THist_Add ( lua_State* L );
int luaExt_THist_Scale ( lua_State* L );
int luaExt_THist_SetRangeUser ( lua_State* L );
int luaExt_THist_Fit ( lua_State* L );
int luaExt_THist_Reset ( lua_State* L );

// -------------------------------------------------- TGraphErrors Binder -------------------------------------------------------- //

int luaExt_NewTGraph ( lua_State* L );
int luaExt_TGraph_SetTitle ( lua_State* L );
int luaExt_TGraph_Draw ( lua_State* L );
int luaExt_TGraph_Fit ( lua_State* L );
int luaExt_TGraph_SetPoint ( lua_State* L );
int luaExt_TGraph_GetPoint ( lua_State* L );
int luaExt_TGraph_RemovePoint ( lua_State* L );
int luaExt_TGraph_SetNPoints ( lua_State* L );
int luaExt_TGraph_Eval ( lua_State* L );

// ------------------------------------------------------ TFile Binder ----------------------------------------------------------- //

int luaExt_NewTFile ( lua_State* L );
int luaExt_TFile_Write ( lua_State* L );
int luaExt_TFile_Close ( lua_State* L );

// ------------------------------------------------------ TCutG Binder ----------------------------------------------------------- //

int luaExt_NewTCutG ( lua_State* L );
int luaExt_TCutG_IsInside ( lua_State* L );


// --------------------------------------------------- Lua Library export -------------------------------------------------------- //

class RootAppThreadManager : public TApplication
{
private:

public:
    RootAppThreadManager ( const char *appClassName, Int_t *argc, char **argv, void *options=0, Int_t numOptions=0 );
    virtual ~RootAppThreadManager() {}

    void KillApp()
    {
        cout << "Received signal to kill the app" << endl;
        gSystem->ExitLoop();
    }

    bool shouldStop = false;
    bool safeSync = false;

    ClassDef ( RootAppThreadManager, 1 )
};

extern RootAppThreadManager* theApp;

class LuaEmbeddedCanvas : public TCanvas
{
private:

public:
    LuaEmbeddedCanvas();
    virtual ~LuaEmbeddedCanvas()
    {
        KillRootApp();
    }

    void KillRootApp()
    {
//         cout << "Emitting signal to kill the app" << endl;
        Emit ( "KillRootApp()" );
    }

    ClassDef ( LuaEmbeddedCanvas, 1 )
};

extern map<string, lua_State*> tasksStates;
extern map<lua_State*, string> tasksNames;
extern map<string, string> tasksStatus;
extern map<string, mutex> tasksMutexes;

extern map<lua_State*, vector<string>> tasksPendingSignals;

struct NewTaskArgs {
    string task;
    string packages;
};

int StartNewTask_C ( lua_State* L );

int LockTaskMutex ( lua_State* L );
int ReleaseTaskMutex ( lua_State* L );

int MakeSyncSafe ( lua_State* L );

int GetTaskStatus(lua_State* L);

int SendSignal_C(lua_State* L);
int CheckSignals_C(lua_State* L);

static const luaL_Reg luaXroot_lib [] =
{
    {"StartNewTask_C", StartNewTask_C},
    {"MakeSyncSafe", MakeSyncSafe},
    {"LockTaskMutex", LockTaskMutex},
    {"ReleaseTaskMutex", ReleaseTaskMutex},
    {"GetTaskStatus", GetTaskStatus},
    {"SendSignal_C", SendSignal_C},
    {"CheckSignals_C", CheckSignals_C},

    {"TApplication", luaExt_NewTApplication},

    {"TObject", luaExt_NewTObject},

    {"TFile", luaExt_NewTFile},

    {"TF1", luaExt_NewTF1},
    {"THist", luaExt_NewTHist},
    {"TGraph", luaExt_NewTGraph},
    {"TCutG", luaExt_NewTCutG},

    {"saveprompthistory", saveprompthistory},
    {"wipeprompthistory", wipeprompthistory},

    // SOCKETS BINDING //

    {"SysUnlink", LuaSysUnlink},
    {"SysRead", LuaSysRead},
    {"SysWrite", LuaSysWrite},

    {"NewSocket", LuaNewSocket},

    {"SocketBind", LuaSocketBind},
    {"SocketConnect", LuaSocketConnect},

    {"SocketSelect", LuaSocketSelect},

    {"SocketListen", LuaSocketListen},
    {"SocketAccept", LuaSocketAccept},


    {"SocketReceive", LuaSocketReceive},
    {"SocketSend", LuaSocketSend},
    
    {NULL, NULL}
};

extern "C" int luaopen_libLuaXRootlib ( lua_State* L );

#endif
