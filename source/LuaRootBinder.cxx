#include "LuaRootBinder.h"
#include "TPad.h"
#include <llimits.h>
#include <csignal>
#include <readline/readline.h>

#include "TUnixSystem.h"
#include "TSysEvtHandler.h"

map<TObject*, TCanvas*> canvasTracker;
mutex syncSafeGuard;
int updateRequestPending;

void singtstp_handler_stop ( int i )
{
    if ( i == SIGTSTP )
    {
        signal ( SIGTSTP, singtstp_handler_pause );

        for ( auto itr = tasksNames.begin(); itr != tasksNames.end(); itr++ )
        {
            PushSignal ( itr->second, "stop" );
        }

        cout << endl;
    }
}

void singtstp_handler_pause ( int i )
{
    if ( i == SIGTSTP )
    {
        signal ( SIGTSTP, singtstp_handler_stop );

        for ( auto itr = tasksNames.begin(); itr != tasksNames.end(); itr++ )
        {
            PushSignal ( itr->second, "wait" );
        }

        cout << endl;
    }
}

vector<string> getpossiblefields ( string field )
{
    vector<string> autocompletes;

    if ( field.empty() )
    {
        return autocompletes;
    }

    bool searchFn = false;
    string rootfield = "";

    size_t functionSep = field.find ( ":" );

    if ( functionSep != string::npos )
    {
        rootfield = field.substr ( 0, functionSep );
        TryGetGlobalField ( lua, rootfield );
        rootfield += ":";
        searchFn = true;
    }
    else
    {
        size_t lastDot = field.find_last_of ( "." );

        if ( lastDot != string::npos )
        {
            rootfield = field.substr ( 0, lastDot );
            //         cout << "Checking field " << rootfield  << endl;

            TryGetGlobalField ( lua, rootfield );

            rootfield += ".";
        }
        else
        {
            lua_getglobal ( lua, "_G" );
        }
    }

    if ( lua_type ( lua, -1 ) != LUA_TTABLE )
    {
        if ( lua_type ( lua, -1 ) != LUA_TNIL )
        {
            lua_getmetatable ( lua, -1 );
            lua_remove ( lua, -2 );
        }

        if ( lua_type ( lua, -1 ) != LUA_TTABLE )
        {
            lua_pop ( lua, 1 );
            return autocompletes;
        }
    }

    //     cout << "**********************************Trying to check all the possibilities*****************************" << endl;

    lua_pushnil ( lua );

    while ( lua_next ( lua, -2 ) != 0 )
    {
        if ( lua_type ( lua, -2 ) == LUA_TSTRING )
        {
            if ( !searchFn || lua_type ( lua, -1 ) == LUA_TFUNCTION )
            {
                //             cout << "Possibility: " << lua_tostring ( lua, -2 ) << endl;
                autocompletes.push_back ( rootfield + ( ( string ) lua_tostring ( lua, -2 ) ) );
            }
        }

        lua_pop ( lua, 1 );
    }

    lua_pop ( lua, 1 );

    return autocompletes;
}

char* possibilities_generator ( const char *text, int state )
{
    static int list_index, len;
    char *name;

    if ( !state )
    {
        list_index = 0;
        len = strlen ( text );
    }

    vector<string> autocompletes = getpossiblefields ( text );

    char** possibilities = new char*[autocompletes.size() +1];

    for ( unsigned int i = 0; i < autocompletes.size(); i++ )
    {
        possibilities[i] = new char[1024];
        sprintf ( possibilities[i], "%s", autocompletes[i].c_str() );
    }

    possibilities[autocompletes.size()] = NULL;

    while ( ( name = possibilities[list_index++] ) )
    {
        if ( strncmp ( name, text, len ) == 0 )
        {
            //             cout << "Possibility: " << name << endl;
            return strdup ( name );
        }
    }

    return NULL;
}

char** input_completion ( const char *text, int start, int end )
{
    rl_completion_append_character = '\0';
    return rl_completion_matches ( text, possibilities_generator );
}

mutex rootProcessLoopLock;

RootAppThreadManager* theApp = 0;

RootAppThreadManager::RootAppThreadManager ( const char *appClassName, Int_t *argc, char **argv, void *options, Int_t numOptions ) : TApplication ( appClassName, argc, argv, options, numOptions )
{

}

LuaEmbeddedCanvas::LuaEmbeddedCanvas() : TCanvas()
{
//     TQObject::Connect ( this, "HasBeenKilled()", "RootAppThreadManager", theApp, "OnCanvasKilled()" );
}

extern "C" int luaopen_libLuaXRootlib ( lua_State* L )
{
    luaL_requiref ( L, "_G", luaopen_base, 1 );
    lua_pop ( L, 1 );

    lua_getglobal ( L, "_G" );
    luaL_setfuncs ( L, luaXroot_lib, 0 );
    lua_pop ( L, 1 );

    lua_getglobal ( L, "_G" );
    luaL_setfuncs ( L, luaTTreeBranchFns, 0 );
    lua_pop ( L, 1 );

    InitializeBranchesFuncs();

    LuaRegisterSocketConsts ( L );
    LuaRegisterSysOpenConsts ( L );

    return 0;
}

int CompilePostInit_C ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, false, "CompilePostInit_C", LUA_TTABLE ) ) return 0;

    lua_getfield ( L, 1, "script" );

    if ( !CheckLuaArgs ( L, 1, false, "CompilePostInit_C missing or invalid argument \"script\":", LUA_TTABLE ) ) return 0;

    const char* scriptname = lua_tostring ( L, -1 );
    const char* targetLanguage = "++";

    if ( lua_checkfield ( L, 1, "target", LUA_TSTRING ) )
    {
        string targetstr = lua_tostring ( L, -1 );

        if ( targetstr == "C" ) targetLanguage = "";
        else if ( targetstr == "C++" ) targetLanguage = "++";
    }

    char* linecall = new char[2048];

    sprintf ( linecall, ".L %s%s", scriptname, targetLanguage );

    gROOT->ProcessLine ( linecall );

    return 0;
}

map<string, lua_State*> tasksStates;
map<lua_State*, string> tasksNames;
map<string, string> tasksStatus;
map<string, mutex> tasksMutexes;

map<lua_State*, vector<string>> tasksPendingSignals;

int TasksList_C ( lua_State* L )
{
    lua_newtable ( L );

    for ( auto itr = tasksNames.begin(); itr != tasksNames.end(); itr++ )
    {
        string taskname = itr->second;
        lua_pushstring ( L, tasksStatus[taskname].c_str() );
        lua_setfield ( L, -2, taskname.c_str() );
    }

    return 1;
}

int LockTaskMutex ( lua_State* L )
{
    string mutexName;

    if ( CheckLuaArgs ( L, 1, false, "", LUA_TSTRING ) )
    {
        mutexName = lua_tostring ( L, 1 );
    }
    else
    {
        lua_getglobal ( L, "taskName" );

        if ( CheckLuaArgs ( L, -1, true, "LockTaskMutex", LUA_TSTRING ) ) mutexName = lua_tostring ( L, -1 );
    }

    cout << "Trying to acquire lock on mutex " << mutexName << endl;

    tasksMutexes[mutexName].lock();

    cout << "Locked mutex " << mutexName << endl;

    return 0;
}

int ReleaseTaskMutex ( lua_State* L )
{
    string mutexName;

    if ( CheckLuaArgs ( L, 1, false, "", LUA_TSTRING ) )
    {
        mutexName = lua_tostring ( L, 1 );
    }
    else
    {
        lua_getglobal ( L, "taskName" );
        if ( CheckLuaArgs ( L, -1, true, "ReleaseTaskMutex", LUA_TSTRING ) ) mutexName = lua_tostring ( L, -1 );
    }

    cout << "Releasing mutex " << mutexName << endl;

    tasksMutexes[mutexName].unlock();

    return 0;
}

void* NewTaskFn ( void* arg )
{
    //     cout << "Test task setup..." << endl;
    NewTaskArgs* args = ( NewTaskArgs* ) arg;

    lua_State* L = luaL_newstate();
    luaL_openlibs ( L );

    const char* luaXrootPath = std::getenv ( "LUAXROOTLIBPATH" );
    lua_pushstring ( L, luaXrootPath );
    lua_setglobal ( L, "LUAXROOTLIBPATH" );

    string logonFile = ( string ) luaXrootPath + "/../scripts/luaXrootlogon.lua";
    luaL_dofile ( L, logonFile.c_str() );

    //     luaopen_libLuaXRootlib ( L );
    if ( !args->packages.empty() )
    {
        //         cout << "Some additional packages need to be loaded" << endl;

        lua_getglobal ( L, "LoadAdditionnalPackages" );

        luaL_loadstring ( L, ( args->packages ).c_str() );

        lua_pcall ( L, 0, LUA_MULTRET, 0 );

        if ( !CheckLuaArgs ( L, -1, true, "NewTaskFn packages table:", LUA_TTABLE ) )
        {
            return 0;
        }

        lua_pcall ( L, 1, LUA_MULTRET, 0 );
    }

    //     cout << "Will retrieve arguments..." << endl;

    luaL_loadstring ( L, ( args->task ).c_str() );

    lua_pcall ( L, 0, LUA_MULTRET, 0 );

    if ( !CheckLuaArgs ( L, -1, true, "NewTaskFn", LUA_TTABLE ) )
    {
        return 0;
    }

    lua_getfield ( L, -1, "name" );
    string taskname = lua_tostring ( L, -1 );
    lua_pop ( L, 1 );

    lua_pushstring ( L, taskname.c_str() );
    lua_setglobal ( L, "taskName" );

    tasksStates[taskname] = L;
    tasksNames[L] = taskname;
    tasksStatus[taskname] = "waiting";

    lua_getfield ( L, -1, "taskfn" );

    if ( lua_type ( L, -1 ) == LUA_TSTRING )
    {
        const char* taskfn_name = lua_tostring ( L, -1 );
        lua_pop ( L, 1 );
        //         cout << "Task given as string: " << taskfn_name << endl;
        TryGetGlobalField ( L, taskfn_name );
    }

    if ( !CheckLuaArgs ( L, -1, true, "NewTaskFn", LUA_TFUNCTION ) )
    {
        return 0;
    }

    //     cout << "Task is valid. Getting arguments..." << endl;

    lua_getfield ( L, -2, "args" );

    if ( !CheckLuaArgs ( L, -1, true, "NewTaskFn", LUA_TTABLE ) )
    {
        return 0;
    }

    int nargs = lua_rawlen ( L, -1 );

    int argsStackPos = lua_gettop ( L );

    for ( int i = 0; i < nargs; i++ )
    {
        lua_geti ( L, argsStackPos, i+1 );
    }

    lua_remove ( L, argsStackPos );

    tasksMutexes[taskname].lock();

    tasksStatus[taskname] = "running";

    lua_pcall ( L, nargs, LUA_MULTRET, 0 );

    tasksMutexes[taskname].unlock();

    tasksStatus[taskname] = "done";

    //     cout << "Task complete" << endl;

    return nullptr;
}

int StartNewTask_C ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "StartNewTask_C", LUA_TSTRING ) )
    {
        return 0;
    }

    if ( lua_gettop ( L ) > 1 && !CheckLuaArgs ( L, 2, true, "StartNewTask_C", LUA_TSTRING ) )
    {
        return 0;
    }

    NewTaskArgs* args = new NewTaskArgs;

    args->task = lua_tostring ( L, 1 );
    args->packages = "";

    if ( lua_gettop ( L ) > 1 )
    {
        args->packages = lua_tostring ( L, 2 );
    }

    pthread_t newTask;

    pthread_create ( &newTask, nullptr, NewTaskFn, ( void* ) args );

    return 0;
}

int MakeSyncSafe ( lua_State* L )
{
    bool apply = true;

    if ( CheckLuaArgs ( L, 1, false, "", LUA_TBOOLEAN ) )
    {
        apply = lua_toboolean ( L, 1 );
    }

    if ( !apply )
    {
        updateRequestPending = 0;
        //         cout << "Reset updateRequestpending: " << updateRequestPending << endl;
    }

    if ( theApp->safeSync ) syncSafeGuard.unlock();

    if ( apply )
    {
        theApp->shouldStop = true;
        syncSafeGuard.lock();
        theApp->safeSync = true;
        //         cout << "Setting up sync safetey mutex..." << endl;
    }

    return 0;
}

int GetTaskStatus ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "GetTaskStatus", LUA_TSTRING ) )
    {
        return 0;
    }

    string taskname = lua_tostring ( L, 1 );

    lua_pushstring ( L, tasksStatus[taskname].c_str() );

    return 1;
}

void PushSignal ( string taskname, string signal )
{
    vector<string>* sigs = &tasksPendingSignals[tasksStates[taskname]];

    //     cout << "Sending signal " << signal << " to " << taskname << endl;

    if ( signal == "stop" || signal == "wait" || signal == "resume" ) sigs->clear();

    sigs->push_back ( signal );
}

int SendSignal_C ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "SendSignal_C", LUA_TSTRING, LUA_TSTRING ) )
    {
        return 0;
    }

    string taskname = lua_tostring ( L, 1 );
    string signal = lua_tostring ( L, 2 );

    PushSignal ( taskname, signal );

    lua_pushboolean ( L, true );

    return 1;
}

int CheckSignals_C ( lua_State* L )
{
    vector<string>* sigs = &tasksPendingSignals[L];
    unsigned int nsigs = sigs->size();

    if ( nsigs > 0 )
    {
//         cout << nsigs << " pending signals for " << tasksNames[L] <<" ..." << endl;
        for ( unsigned int i = 0; i < nsigs; i++ )
        {
            //             cout << "Treating signal ";
            if ( i == 0 && sigs->at ( i ) == "stop" )
            {
//                 cout << "stop" << endl;
                sigs->clear();
                return 0;
            }
            if ( i == 0 && sigs->at ( i ) == "wait" )
            {
//                 cout << "wait" << endl;
                tasksStatus[tasksNames[L]] = "suspended";

                while ( sigs->at ( i ) == "wait" )
                {
                    sleep ( 1 );
                }

                return CheckSignals_C ( L );
            }
            if ( i == 0 && sigs->at ( i ) == "resume" )
            {
                tasksStatus[tasksNames[L]] = "running";
                sigs->clear();
                signal ( SIGTSTP, singtstp_handler_pause );
                return 1;
            }
            else
            {
//                 cout << sigs->at ( i ) << endl;
                lua_getglobal ( L, "ProcessSignal" );
                lua_pushstring ( L, sigs->at ( i ).c_str() );
                lua_pcall ( L, 1, LUA_MULTRET, 0 );
            }
        }
    }
//     else cout << "No pending signals" << endl;

    sigs->clear();
    lua_pushboolean ( L, 1 );

    return 1;
}

// -------------------------------------------------- TApplication Binder -------------------------------------------------------- //

void* StartRootEventProcessor ( void* arg )
{
    updateRequestPending = 0;

    theApp->SetupUpdateSignalReceiver();

restartruntask:

    //     if ( theApp->shouldStop ) rootProcessLoopLock.unlock();

    theApp->shouldStop = false;

    if ( theApp->safeSync )
    {
        //         cout << "trying to acquire lock on sync mutex" << endl;
        syncSafeGuard.lock();
        //         cout << "lock acquired and now releasing it..." << endl;
        syncSafeGuard.unlock();
        theApp->safeSync = false;
    }

    rootProcessLoopLock.lock();

    while ( !theApp->shouldStop )
    {
        //         cout << "Restarting InnerLoop" << endl;
        theApp->StartIdleing();
        gSystem->InnerLoop();
//         cout << "Exiting InnerLoop" << endl;
        theApp->StopIdleing();
    }

//     cout << "Exiting main loop..." << endl;

    rootProcessLoopLock.unlock();

    while ( updateRequestPending > 0 )
    {
        //         cout << "Waiting for updates: " << updateRequestPending << endl;
        gSystem->Sleep ( 50 );
    }

    //     cout << "All Updates have been treated" << endl;

    //     rootProcessLoopLock.lock();

    //     cout << "theApp acquired master lock" << endl;

    gSystem->Sleep ( 50 );

    goto restartruntask;

    return nullptr;
}

int luaExt_NewTApplication ( lua_State* L )
{
    if ( theApp == nullptr )
    {
        //         cout << "theApp == nullptr, initializing it..." << endl;

        signal ( SIGTSTP, singtstp_handler_pause );

        lua = L;
        rl_attempted_completion_function = input_completion;

        RootAppThreadManager** tApp = NewUserData<RootAppThreadManager> ( L, "tapp", nullptr, nullptr );

        theApp = *tApp;

        theApp->SetupUpdateSignalSender();

        theApp->SetupRootAppMetatable ( L );

        gSystem->AddDynamicPath ( "${LUAXROOTLIBPATH}" );
        gSystem->AddIncludePath ( "-I/usr/include/readline" );
        gSystem->AddIncludePath ( "-I$LUAXROOTLIBPATH/../include" );
        gSystem->AddIncludePath ( "-I$LUAXROOTLIBPATH/../lua" );
        gSystem->Load ( "libguilereadline-v-18.so" );
        gSystem->Load ( "liblualib.so" );
        gSystem->Load ( "libLuaXRootlib.so" );

        pthread_t startRootEventProcessrTask;

        pthread_create ( &startRootEventProcessrTask, nullptr, StartRootEventProcessor, nullptr );

        theApp->WaitForUpdateReceiver();

        return 1;
    }
    else
    {
        return 0;
    }
}

int luaExt_TApplication_Run ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TApplication_Run", LUA_TUSERDATA ) )
    {
        return 0;
    }

    TApplication* tApp = * ( reinterpret_cast<TApplication**> ( lua_touserdata ( L, 1 ) ) );

    tApp->Run();

    return 0;
}

int luaExt_TApplication_Update ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TApplication_Update", LUA_TUSERDATA ) )
    {
        return 0;
    }

    TApplication* tApp = * ( reinterpret_cast<TApplication**> ( lua_touserdata ( L, 1 ) ) );

    while ( updateRequestPending > 0 )
    {
        //         cout << "Waiting for updates: " << updateRequestPending << endl;
        gSystem->Sleep ( 50 );
    }

    theApp->NotifyUpdatePending();

    TTimer* innerloop_timer = new TTimer ( 2000 );
    gSystem->AddTimer ( innerloop_timer );

    //     cout << "Force update theApp" << endl;
    tApp->StartIdleing();
    gSystem->InnerLoop();
    tApp->StopIdleing();
    //     cout << "Force update theApp done" << endl;

    gSystem->RemoveTimer ( innerloop_timer );

    theApp->NotifyUpdateDone();

    return 0;
}

int luaExt_TApplication_Terminate ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TApplication_Terminate", LUA_TUSERDATA ) )
    {
        return 0;
    }

    for ( auto itr = canvasTracker.begin(); itr != canvasTracker.end(); itr++ ) delete itr->second;

    TApplication* tApp = * ( reinterpret_cast<TApplication**> ( lua_touserdata ( L, 1 ) ) );

    tApp->Terminate();

    return 0;
}

// ------------------------------------------------------- TObject Binder ------------------------------------------------------- //

int luaExt_NewTObject ( lua_State* L )
{
    NewUserData<TObject> ( L );

    SetupTObjectMetatable ( L );

    return 1;
}

int luaExt_TObject_Write ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TObject_Write", LUA_TUSERDATA ) ) return 0;

    TObject* obj = GetUserData<TObject> ( L );
    obj->Write();

    return 0;
}

int luaExt_TObject_Draw ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TObject_Draw", LUA_TUSERDATA ) )
    {
        return 0;
    }

    if ( lua_gettop ( L ) == 2 && !CheckLuaArgs ( L, 2, true, "luaExt_TObject_Draw", LUA_TSTRING ) )
    {
        return 0;
    }

    //     cout << "Executing luaExt_TObject_Draw" << endl;

    TObject* obj = * ( static_cast<TObject**> ( lua_touserdata ( L, 1 ) ) );

    string opts = "";

    if ( lua_gettop ( L ) == 2 )
    {
        opts = lua_tostring ( L, 2 );
    }

    theApp->NotifyUpdatePending();

    if ( canvasTracker[obj] == nullptr )
    {
        if ( opts.find ( "same" ) == string::npos && opts.find ( "SAME" ) == string::npos )
        {
            LuaEmbeddedCanvas* disp = new LuaEmbeddedCanvas();
            disp->cd();
            canvasTracker[obj] = ( TCanvas* ) disp;
        }
        else
        {
            canvasTracker[obj] = gPad->GetCanvas();
        }

        obj->Draw ( opts.c_str() );
    }
    else
    {
        if ( dynamic_cast<TH1*> ( obj ) != nullptr ) ( ( TH1* ) obj )->Rebuild();
        canvasTracker[obj]->Modified();
        canvasTracker[obj]->Update();
    }

    theApp->NotifyUpdateDone();

    return 0;
}

int luaExt_TObject_Update ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TObject_Update", LUA_TUSERDATA ) )
    {
        return 0;
    }

    TObject* obj = * ( static_cast<TObject**> ( lua_touserdata ( L, 1 ) ) );

    string opts = "";

    if ( lua_gettop ( L ) == 2 )
    {
        opts = lua_tostring ( L, 2 );
    }

    theApp->NotifyUpdatePending();

    if ( canvasTracker[obj] != nullptr )
    {
        //         cout << "Processing Update" << endl;
        canvasTracker[obj]->Modified();
        canvasTracker[obj]->Update();
    }

    theApp->NotifyUpdateDone();

    return 0;
}

int luaExt_TObject_GetName ( lua_State* L )
{
    TObject* obj = GetUserData<TObject> ( L, 1, "luaExt_TObject_GetName" );

    lua_pushstring ( L, obj->GetName() );

    return 1;
}

int luaExt_TObject_GetTitle ( lua_State* L )
{
    TObject* obj = GetUserData<TObject> ( L, 1, "luaExt_TObject_GetName" );

    lua_pushstring ( L, obj->GetTitle() );

    return 1;
}

// ------------------------------------------------------ TF1 Binder -------------------------------------------------------------- //

int luaExt_NewTF1 ( lua_State* L )
{
    if ( lua_gettop ( L ) == 0 )
    {
        NewUserData<TF1> ( L );
        return 1;
    }
    else if ( !CheckLuaArgs ( L, 1, true, "luaExt_NewTF1", LUA_TTABLE ) ) return 0;

    lua_unpackarguments ( L, 1, "luaExt_NewTF1 argument table",
    {"name", "formula", "xmin", "xmax", "ndim", "npars"},
    {LUA_TSTRING, LUA_TSTRING, LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER},
    {true, false, false, false ,false, false} );

    string fname = lua_tostring ( L, -6 );

    size_t formula_length;

    const char* fformula = lua_tolstring ( L, -5, &formula_length );
    function<double ( double*,double* ) > fn = nullptr;
    double xmin = lua_tonumberx ( L, -4, nullptr );
    double xmax = lua_tonumberx ( L, -3, nullptr );
    int ndim = lua_tonumberx ( L, -2, nullptr );
    int npars = lua_tonumberx ( L, -1, nullptr );

    if ( xmin == xmax ) xmax = xmin+1;
    if ( ndim <= 0 ) ndim = 1;

    if ( formula_length == 0 )
    {
        if ( npars == 0 )
        {
            cerr << "luaExt_NewTF1 arguments table: npars required if no string formula is given" << endl;
            return 0;
        }

        lua_getfield ( L, 1, "fn" );
        if ( lua_type ( L, -1 ) == LUA_TFUNCTION )
        {
            //             cout << "We got a function instead of a formula" << endl;
            string fn_gfield = ( "TF1fns."+fname );
            bool success = TrySetGlobalField ( L, fn_gfield );

            if ( !success )
            {
                cerr << "Error in luaExt_NewTF1: failed to push fn as global field" << endl;
                lua_settop ( L, 0 );
                return 0;
            }
            //             lua_setglobal ( L, fn_gfield.c_str() );
            cout << "Fn set to global field " << fn_gfield << endl;

            //             lua_newtable ( L );
            // 	    lua_insert(L, -2);
            // 	    lua_setfield(L, -2, fname.c_str());
            //             lua_setglobal ( L, "TF1fns" );

            fn = [=] ( double* x, double* p )
            {
                TryGetGlobalField ( L, fn_gfield );

                if ( lua_type ( L, -1 ) != LUA_TFUNCTION )
                {
                    cerr << "Error in luaExt_NewTF1: failed to retrieve the fn field for the TF1" << endl;
                    lua_settop ( L, 0 );
                    return 0.0;
                }

                //                 lua_getglobal ( L, fn_gfield.c_str() );
                lua_pushnumber ( L, x[0] );

                for ( int i = 0; i < npars; i++ )
                {
                    lua_pushnumber ( L, p[i] );
                }

                lua_pcall ( L, npars+1, 1, 0 );

                double res = lua_tonumber ( L, -1 );

                return res;
            };
        }
        lua_pop ( L, 1 );
    }

    if ( formula_length > 0 ) NewUserData<TF1> ( L, fname.c_str(), fformula, xmin, xmax );
    else if ( fn != nullptr ) NewUserData<TF1> ( L, fname.c_str(), fn, xmin, xmax, npars, ndim );
    else
    {
        cerr << "Error in luaExt_NewTF1: missing required field in argument table: \"formula\" or \"fn\"" << endl;
        lua_settop ( L, 0 );
        return 0;
    }

    SetupTObjectMetatable ( L );

    AddMethod ( L, luaExt_TF1_Eval, "Eval" );
    AddMethod ( L, luaExt_TF1_SetParameters, "SetParameters" );
    AddMethod ( L, luaExt_TF1_GetPars, "GetParameters" );
    AddMethod ( L, luaExt_TF1_GetChi2, "GetChi2" );

    return 1;
}

int luaExt_TF1_SetParameters ( lua_State* L )
{
    if ( lua_gettop ( L ) < 2 || !CheckLuaArgs ( L, 1, true, "luaExt_TF1_SetParameters", LUA_TUSERDATA ) )
    {
        return 0;
    }

    TF1* func = * ( reinterpret_cast<TF1**> ( lua_touserdata ( L, 1 ) ) );

    if ( lua_type ( L, 2 ) == LUA_TTABLE )
    {
        DoForEach ( L, 2, [&] ( lua_State* L_ )
        {
            if ( !CheckLuaArgs ( L_, -2, true, "luaExt_TF1_SetParameters: parameters table ", LUA_TNUMBER, LUA_TNUMBER ) )
            {
                return true;
            }

            int parIdx = lua_tointeger ( L_, -2 );
            double parVal = lua_tonumber ( L_, -1 );
            cout << "Setting parameter " << parIdx << " : " << parVal << endl;
            func->SetParameter ( parIdx, parVal );
            return false;
        } );
    }
    else
    {
        for ( int i = 0; i < func->GetNpar(); i++ )
        {
            if ( lua_type ( L, i+2 ) == LUA_TNUMBER )
            {
                cout << "Setting parameter " << i << " : " << lua_tonumber ( L, i+2 ) << endl;
                func->SetParameter ( i, lua_tonumber ( L, i+2 ) );
            }
        }
    }

    return 0;
}

int luaExt_TF1_Eval ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TF1_Eval", LUA_TUSERDATA, LUA_TNUMBER ) )
    {
        return 0;
    }

    TF1* func = * ( reinterpret_cast<TF1**> ( lua_touserdata ( L, 1 ) ) );

    double toEval = lua_tonumber ( L, 2 );

    lua_pushnumber ( L, func->Eval ( toEval ) );

    return 1;
}

int luaExt_TF1_GetPars ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TF1_GetPars", LUA_TUSERDATA ) )
    {
        return 0;
    }

    TF1* func = * ( reinterpret_cast<TF1**> ( lua_touserdata ( L, 1 ) ) );

    int nres = 0;

    if ( lua_gettop ( L ) == 1 )
    {
        for ( int i = 0; i < func->GetNpar(); i++ )
        {
            lua_pushnumber ( L, func->GetParameter ( i ) );
            nres++;
        }
    }
    else if ( lua_type ( L, 2 ) == LUA_TNUMBER )
    {
        lua_pushnumber ( L, func->GetParameter ( lua_tointeger ( L, 2 ) ) );
        nres = 1;
    }
    else if ( !CheckLuaArgs ( L, 2, true, "luaExt_TF1_GetPars", LUA_TTABLE ) )
    {
        return 0;
    }
    else
    {
        lua_newtable ( L );
        DoForEach ( L, 2, [&] ( lua_State* L_ )
        {
            if ( !CheckLuaArgs ( L_, -2, true, "luaExt_TF1_GetPars: parameters list ", LUA_TNUMBER, LUA_TNUMBER ) )
            {
                return true;
            }

            int parnum = lua_tointeger ( L_, -1 );
            double fpar = func->GetParameter ( parnum );
            lua_pushnumber ( L_, fpar );

            lua_setfield ( L_, -4, to_string ( parnum ).c_str() );
            return false;
        } );

        nres = 1;
    }

    return nres;
}

int luaExt_TF1_GetChi2 ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TF1_GetPars", LUA_TUSERDATA ) )
    {
        return 0;
    }

    TF1* func = * ( reinterpret_cast<TF1**> ( lua_touserdata ( L, 1 ) ) );

    lua_pushnumber ( L, func->GetChisquare() );

    return 1;
}

// -------------------------------------------------- TH[istograms] Binder -------------------------------------------------------- //

int luaExt_NewTHist ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_NewTHist", LUA_TTABLE ) )
    {
        return 0;
    }

    string name, title;
    double xmin, xmax, nbinsx, ymin, ymax, nbinsy;

    lua_unpackarguments ( L, 1, "luaExt_NewTHist argument table",
    {"name", "title", "xmin", "xmax", "nbinsx", "ymin", "ymax", "nbinsy"},
    {LUA_TSTRING, LUA_TSTRING, LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER},
    {true, true, true ,true, true, false, false, false} );

    name = lua_tostring ( L, -8 );
    title = lua_tostring ( L, -7 );
    xmin = lua_tointeger ( L, -6 );
    xmax = lua_tointeger ( L, -5 );
    nbinsx = lua_tointeger ( L, -4 );

    ymin = lua_tointegerx ( L, -3, nullptr );
    ymax = lua_tointegerx ( L, -2, nullptr );
    nbinsy = lua_tointegerx ( L, -1, nullptr );

    if ( nbinsy == 0 && nbinsx > 0 && xmax > xmin ) NewUserData<TH1D> ( L, name.c_str(), title.c_str(), nbinsx, xmin, xmax );
    else if ( ymax > ymin ) NewUserData<TH2D> ( L, name.c_str(), title.c_str(), nbinsx, xmin, xmax, nbinsy, ymin, ymax );
    else
    {
        cerr << "Error in luaExt_NewTHist: argument table: invalid boundaries or number of bins" << endl;
        lua_settop ( L, 0 );
        return 0;
    }

    SetupTObjectMetatable ( L );

    AddMethod ( L, luaExt_THist_Clone, "Clone" );
    AddMethod ( L, luaExt_THist_Fill, "Fill" );
    AddMethod ( L, luaExt_THist_Reset, "Reset" );
    AddMethod ( L, luaExt_THist_Reset, "Add" );
    AddMethod ( L, luaExt_THist_Scale, "Scale" );
    AddMethod ( L, luaExt_THist_SetRangeUser, "SetRangeUser" );
    AddMethod ( L, luaExt_THist_Fit, "Fit" );

    return 1;
}

int luaExt_THist_Clone ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, -1, true, "luaExt_THist_Clone", LUA_TUSERDATA ) )
    {
        return 0;
    }

    TH1* hist = * ( reinterpret_cast<TH1**> ( lua_touserdata ( L, 1 ) ) );

    lua_getmetatable ( L, -1 );

    if ( dynamic_cast<TH2D*> ( hist ) != nullptr )
    {
        TH2D** clone =  reinterpret_cast<TH2D**> ( lua_newuserdata ( L, sizeof ( TH2D* ) ) );
        *clone = ( TH2D* ) hist->Clone();
    }
    else
    {
        TH1D** clone =  reinterpret_cast<TH1D**> ( lua_newuserdata ( L, sizeof ( TH1D* ) ) );
        *clone = ( TH1D* ) hist->Clone();
    }

    lua_insert ( L, -2 );
    lua_setmetatable ( L, -2 );

    return 1;
}

int luaExt_THist_Fill ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_THist_Fill", LUA_TUSERDATA, LUA_TTABLE ) )
    {
        return 0;
    }

    TH1* hist = * ( reinterpret_cast<TH1**> ( lua_touserdata ( L, 1 ) ) );

    double x, y;
    int w = 1;

    lua_getfield ( L, 2, "x" );
    if ( !CheckLuaArgs ( L, -1, true, "luaExt_THist_Fill argument table: x", LUA_TNUMBER ) && !CheckLuaArgs ( L, -1, true, "luaExt_THist_Fill argument table: x", LUA_TNUMBER ) )
    {
        return 0;
    }
    x = lua_type ( L, -1 ) == LUA_TNUMBER ? lua_tonumber ( L, -1 ) : lua_toboolean ( L, -1 );
    lua_pop ( L, 1 );

    if ( lua_checkfield ( L, 2, "w", LUA_TNUMBER ) )
    {
        w = lua_tointeger ( L, -1 );
        lua_pop ( L, 1 );
    }

    if ( lua_checkfield ( L, 2, "y", LUA_TNUMBER ) || lua_checkfield ( L, 2, "y", LUA_TBOOLEAN ) )
    {
        y = lua_type ( L, -1 ) == LUA_TNUMBER ? lua_tonumber ( L, -1 ) : lua_toboolean ( L, -1 );
        lua_pop ( L, 1 );
        ( ( TH2D* ) hist )->Fill ( x, y, w );
        return 0;
    }
    else
    {
        hist->Fill ( x, w );
        return 0;
    }
}

int luaExt_THist_Add ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, -1, true, "luaExt_THist_Add", LUA_TUSERDATA, LUA_TUSERDATA, LUA_TNUMBER ) )
    {
        return 0;
    }

    TH1* hist = * ( reinterpret_cast<TH1**> ( lua_touserdata ( L, 1 ) ) );
    TH1* hist2 = * ( reinterpret_cast<TH1**> ( lua_touserdata ( L, 2 ) ) );

    double scale = 1.0;

    if ( lua_gettop ( L ) == 3 )
    {
        scale = lua_tonumber ( L, 3 );
    }

    hist->Add ( hist2, scale );

    return 0;
}

int luaExt_THist_Scale ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, -1, true, "luaExt_THist_Add", LUA_TUSERDATA, LUA_TNUMBER ) )
    {
        return 0;
    }

    TH1* hist = * ( reinterpret_cast<TH1**> ( lua_touserdata ( L, 1 ) ) );

    double scale = lua_tonumber ( L, 2 );

    hist->Scale ( scale );

    return 0;
}

int luaExt_THist_SetRangeUser ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, -1, true, "luaExt_THist_SetRangeUser", LUA_TUSERDATA, LUA_TTABLE ) )
    {
        return 0;
    }

    TH1* hist = * ( reinterpret_cast<TH1**> ( lua_touserdata ( L, 1 ) ) );

    TAxis* xAxis = hist->GetXaxis();
    TAxis* yAxis = hist->GetYaxis();

    double xmin, xmax, ymin, ymax;

    xmin = xAxis->GetXmin();
    xmax = xAxis->GetXmax();

    ymin = yAxis->GetXmin();
    ymax = yAxis->GetXmax();

    if ( lua_checkfield ( L, 1, "xmin", LUA_TNUMBER ) )
    {
        xmin = lua_tonumber ( L, -1 );
        lua_pop ( L, 1 );
    }

    if ( lua_checkfield ( L, 1, "xmax", LUA_TNUMBER ) )
    {
        xmax = lua_tonumber ( L, -1 );
        lua_pop ( L, 1 );
    }

    if ( lua_checkfield ( L, 1, "ymin", LUA_TNUMBER ) )
    {
        ymin = lua_tonumber ( L, -1 );
        lua_pop ( L, 1 );
    }

    if ( lua_checkfield ( L, 1, "ymax", LUA_TNUMBER ) )
    {
        ymax = lua_tonumber ( L, -1 );
        lua_pop ( L, 1 );
    }

    if ( xmax <= xmin || ymax <= ymin )
    {
        cerr << "Error in luaExt_THist_SetRange: invalid boundaries" << endl;
        lua_settop ( L, 0 );
        return 0;
    }

    xAxis->SetRangeUser ( xmin, xmax );
    yAxis->SetRangeUser ( ymin, ymax );

    return 0;
}

int luaExt_THist_Fit ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_THist_Fit", LUA_TUSERDATA, LUA_TTABLE ) )
    {
        return 0;
    }

    TH1* hist = * ( reinterpret_cast<TH1**> ( lua_touserdata ( L, 1 ) ) );

    TF1* fitfunc = nullptr;

    lua_getfield ( L, 2, "fn" );

    if ( !CheckLuaArgs ( L, -1, true, "luaExt_THist_Fit argument table: field fn ", LUA_TUSERDATA ) )
    {
        return 0;
    }

    fitfunc = * ( reinterpret_cast<TF1**> ( lua_touserdata ( L, -1 ) ) );
    lua_pop ( L, 1 );

    TAxis* xAxis = hist->GetXaxis();

    double xmin, xmax;
    string opts = "";

    xmin = xAxis->GetXmin();
    xmax = xAxis->GetXmax();

    if ( lua_checkfield ( L, 1, "xmin", LUA_TNUMBER ) )
    {
        xmin = lua_tonumber ( L, -1 );
        lua_pop ( L, 1 );
    }

    if ( lua_checkfield ( L, 1, "xmax", LUA_TNUMBER ) )
    {
        xmax = lua_tonumber ( L, -1 );
        lua_pop ( L, 1 );
    }

    if ( lua_checkfield ( L, 1, "opts", LUA_TNUMBER ) )
    {
        opts = lua_tostring ( L, -1 );
        lua_pop ( L, 1 );
    }

    hist->Fit ( fitfunc, opts.c_str(), "", xmin, xmax );
    if ( gPad != nullptr )
    {
        gPad->Update();
    }

    return 1;
}

int luaExt_THist_Reset ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_THist_Reset", LUA_TUSERDATA ) )
    {
        return 0;
    }

    TH1* hist = * ( reinterpret_cast<TH1**> ( lua_touserdata ( L, 1 ) ) );

    rootProcessLoopLock.lock();

    theApp->shouldStop = true;
    //     cout << "asked to stop" << endl;

    hist->Reset();
    canvasTracker[ ( TObject* ) hist]->Modified();
    canvasTracker[ ( TObject* ) hist]->Update();

    rootProcessLoopLock.unlock();

    return 1;
}

// -------------------------------------------------- TGraphErrors Binder -------------------------------------------------------- //

int luaExt_NewTGraph ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_NewTGraph", LUA_TTABLE ) )
    {
        return 0;
    }

    int npoints;
    string name, title;
    double* xs, *ys, *dxs, *dys;

    lua_getfield ( L, 1, "n" );

    if ( !CheckLuaArgs ( L, -1, true, "luaExt_NewTGraph argument table: field n ", LUA_TNUMBER ) )
    {
        return 0;
    }

    npoints = lua_tointeger ( L, -1 );
    lua_pop ( L, 1 );

    xs = new double[npoints];
    ys = new double[npoints];

    dxs = new double[npoints];
    dys = new double[npoints];

    bool graph_has_errors = true;

    if ( lua_checkfield ( L, 1, "name", LUA_TSTRING ) )
    {
        name = lua_tostring ( L, -1 );
        lua_pop ( L, 1 );
    }

    if ( lua_checkfield ( L, 1, "title", LUA_TSTRING ) )
    {
        title = lua_tostring ( L, -1 );
        lua_pop ( L, 1 );
    }


    bool points_specified = false;

    lua_getfield ( L, 1, "points" );
    if ( lua_type ( L, -1 ) == LUA_TTABLE )
    {
        points_specified = true;
        DoForEach ( L, -1, [&] ( lua_State* L_ )
        {
            if ( !CheckLuaArgs ( L, -2, true, "luaExt_NewTGraph while setting up points : invalid key for points table ", LUA_TNUMBER ) )
            {
                return true;
            }
            if ( !CheckLuaArgs ( L, -1, true, "luaExt_NewTGraph while setting up points : invalid format for points table ", LUA_TTABLE ) )
            {
                return true;
            }

            int pindex = lua_tointeger ( L_, -2 )-1;

            lua_getfield ( L, -1, "x" );
            if ( !CheckLuaArgs ( L, -1, true, "luaExt_NewTGraph while setting up points : invalid value for x ", LUA_TNUMBER ) )
            {
                return true;
            }
            xs[pindex] = lua_tonumber ( L_, -1 );
            lua_pop ( L_, 1 );

            if ( graph_has_errors )
            {
                if ( lua_checkfield ( L, -1, "dx", LUA_TNUMBER ) )
                {
                    dxs[pindex] = lua_tonumber ( L_, -1 );
                    lua_pop ( L_, 1 );
                }
                else
                {
                    graph_has_errors = false;
                }
            }

            lua_getfield ( L, -1, "y" );
            if ( !CheckLuaArgs ( L, -1, true, "luaExt_NewTGraph while setting up points : invalid value for y ", LUA_TNUMBER ) )
            {
                return true;
            }
            ys[pindex] = lua_tonumber ( L_, -1 );
            lua_pop ( L_, 1 );

            if ( graph_has_errors )
            {
                if ( lua_checkfield ( L, -1, "dy", LUA_TNUMBER ) )
                {
                    dys[pindex] = lua_tonumber ( L_, -1 );
                    lua_pop ( L_, 1 );
                }
                else
                {
                    graph_has_errors = false;
                }
            }

            return false;
        } );

        lua_pop ( L, 1 );
    }
    else
    {
        lua_pop ( L, 1 );
        if ( lua_checkfield ( L, 1, "x", LUA_TTABLE ) )
        {
            points_specified = true;
            DoForEach ( L, -1, [&] ( lua_State* L_ )
            {
                if ( !CheckLuaArgs ( L_, -2, true, "luaExt_NewTGraph: x table", LUA_TNUMBER, LUA_TNUMBER ) )
                {
                    return true;
                }

                xs[lua_tointeger ( L_, -2 )-1] = lua_tonumber ( L_, -1 );

                return false;
            } );
            lua_pop ( L, 1 );
        }
        else
        {
            points_specified = false;
        }

        if ( lua_checkfield ( L, 1, "y", LUA_TTABLE ) )
        {
            DoForEach ( L, -1, [&] ( lua_State* L_ )
            {
                if ( !CheckLuaArgs ( L_, -2, true, "luaExt_NewTGraph: y table", LUA_TNUMBER, LUA_TNUMBER ) )
                {
                    return true;
                }

                ys[lua_tointeger ( L_, -2 )-1] = lua_tonumber ( L_, -1 );

                return false;
            } );
            lua_pop ( L, 1 );
        }
        else
        {
            points_specified = false;
        }

        if ( lua_checkfield ( L, 1, "dx", LUA_TTABLE ) )
        {
            DoForEach ( L, -1, [&] ( lua_State* L_ )
            {
                if ( !CheckLuaArgs ( L_, -2, true, "luaExt_NewTGraph: dx table", LUA_TNUMBER, LUA_TNUMBER ) )
                {
                    return true;
                }

                dxs[lua_tointeger ( L_, -2 )-1] = lua_tonumber ( L_, -1 );

                return false;
            } );
            lua_pop ( L, 1 );
        }
        else
        {
            graph_has_errors = false;
        }

        if ( lua_checkfield ( L, 1, "dy", LUA_TTABLE ) )
        {
            DoForEach ( L, -1, [&] ( lua_State* L_ )
            {
                if ( !CheckLuaArgs ( L_, -2, true, "luaExt_NewTGraph: dy table", LUA_TNUMBER, LUA_TNUMBER ) )
                {
                    return true;
                }

                dys[lua_tointeger ( L_, -2 )-1] = lua_tonumber ( L_, -1 );

                return false;
            } );
            lua_pop ( L, 1 );
        }
        else
        {
            graph_has_errors = false;
        }
    }

    if ( npoints <= 0 )
    {
        cerr << "Error in luaExt_NewTGraph: invalid amount of points specified" << endl;
        lua_settop ( L, 0 );
        return 0;
    }

    TGraphErrors** graph;

    if ( !points_specified ) graph = NewUserData<TGraphErrors> ( L, npoints );
    else
    {
        if ( graph_has_errors ) graph = NewUserData<TGraphErrors> ( L, npoints, xs, ys, dxs, dys );
        else graph = NewUserData<TGraphErrors> ( L, npoints, xs, ys );
    }

    ( *graph )->SetEditable ( false );

    SetupTObjectMetatable ( L );

    AddMethod ( L, luaExt_TGraph_SetTitle, "SetTitle" );
    AddMethod ( L, luaExt_TGraph_Fit, "Fit" );
    AddMethod ( L, luaExt_TGraph_SetPoint, "SetPoint" );
    AddMethod ( L, luaExt_TGraph_GetPoint, "GetPoint" );
    AddMethod ( L, luaExt_TGraph_RemovePoint, "RemovePoint" );
    AddMethod ( L, luaExt_TGraph_SetNPoints, "SetNPoints" );
    AddMethod ( L, luaExt_TGraph_Eval, "Eval" );

    return 1;
}

int luaExt_TGraph_SetTitle ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TGraph_SetTitle", LUA_TUSERDATA, LUA_TSTRING ) )
    {
        return 0;
    }

    TGraphErrors* graph = * ( reinterpret_cast<TGraphErrors**> ( lua_touserdata ( L, 1 ) ) );
    string title = lua_tostring ( L, 2 );

    graph->SetTitle ( title.c_str() );

    return 0;
}

int luaExt_TGraph_Fit ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TGraph_Fit", LUA_TUSERDATA, LUA_TTABLE ) )
    {
        return 0;
    }

    TGraphErrors* graph = * ( reinterpret_cast<TGraphErrors**> ( lua_touserdata ( L, 1 ) ) );

    TF1* fitfunc = nullptr;

    lua_getfield ( L, 2, "fn" );

    if ( !CheckLuaArgs ( L, -1, true, "luaExt_TGraph_Fit argument table: field fn ", LUA_TUSERDATA ) )
    {
        return 0;
    }

    fitfunc = * ( reinterpret_cast<TF1**> ( lua_touserdata ( L, -1 ) ) );
    lua_pop ( L, 1 );

    TAxis* xAxis = graph->GetXaxis();

    double xmin, xmax;
    string opts = "";

    xmin = xAxis->GetXmin();
    xmax = xAxis->GetXmax();

    if ( lua_checkfield ( L, 1, "xmin", LUA_TNUMBER ) )
    {
        xmin = lua_tonumber ( L, -1 );
        lua_pop ( L, 1 );
    }

    if ( lua_checkfield ( L, 1, "xmax", LUA_TNUMBER ) )
    {
        xmax = lua_tonumber ( L, -1 );
        lua_pop ( L, 1 );
    }

    if ( lua_checkfield ( L, 1, "opts", LUA_TSTRING ) )
    {
        opts = lua_tostring ( L, -1 );
        lua_pop ( L, 1 );
    }

    graph->Fit ( fitfunc, opts.c_str(), "", xmin, xmax );
    if ( gPad != nullptr )
    {
        gPad->Update();
    }

    return 0;
}

int luaExt_TGraph_SetPoint ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TGraph_SetPoint", LUA_TUSERDATA, LUA_TTABLE ) )
    {
        return 0;
    }

    TGraphErrors* graph = * ( reinterpret_cast<TGraphErrors**> ( lua_touserdata ( L, 1 ) ) );

    lua_getfield ( L, 2, "i" );
    if ( !CheckLuaArgs ( L, -1, true, "luaExt_TGraph_SetPoint argument table: i ", LUA_TNUMBER ) )
    {
        return 0;
    }
    int idx = lua_tointeger ( L, -1 );
    lua_pop ( L, 1 );

    lua_getfield ( L, 2, "x" );
    if ( !CheckLuaArgs ( L, -1, true, "luaExt_TGraph_SetPoint argument table: x ", LUA_TNUMBER ) )
    {
        return 0;
    }
    double x = lua_tonumber ( L, -1 );
    lua_pop ( L, 1 );

    lua_getfield ( L, 2, "y" );
    if ( !CheckLuaArgs ( L, -1, true, "luaExt_TGraph_SetPoint argument table: y ", LUA_TNUMBER ) )
    {
        return 0;
    }
    double y = lua_tonumber ( L, -1 );
    lua_pop ( L, 1 );

    lua_getfield ( L, 2, "dx" );
    double dx = 0;
    if ( lua_checkfield ( L, 1, "dx", LUA_TNUMBER ) )
    {
        dx = lua_tonumber ( L, -1 );
        lua_pop ( L, 1 );
    }

    lua_getfield ( L, 2, "dy" );
    double dy = 0;
    if ( lua_checkfield ( L, 1, "dy", LUA_TNUMBER ) )
    {
        dy = lua_tonumber ( L, -1 );
        lua_pop ( L, 1 );
    }

    graph->SetPoint ( idx, x ,y );

    if ( dx > 0 || dy > 0 )
    {
        graph->SetPointError ( idx, dx ,dy );
    }

    return 0;
}

int luaExt_TGraph_GetPoint ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TGraph_GetPoint", LUA_TUSERDATA, LUA_TNUMBER ) )
    {
        return 0;
    }

    TGraphErrors* graph = * ( reinterpret_cast<TGraphErrors**> ( lua_touserdata ( L, 1 ) ) );

    int idx = lua_tointeger ( L, 2 );

    double x, y;
    graph->GetPoint ( idx, x, y );

    lua_pushnumber ( L, x );
    lua_pushnumber ( L, y );

    return 2;
}

int luaExt_TGraph_RemovePoint ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TGraph_RemovePoint", LUA_TUSERDATA, LUA_TNUMBER ) )
    {
        return 0;
    }

    TGraphErrors* graph = * ( reinterpret_cast<TGraphErrors**> ( lua_touserdata ( L, 1 ) ) );

    int idx = lua_tointeger ( L, 2 );

    graph->RemovePoint ( idx );

    return 0;
}

int luaExt_TGraph_SetNPoints ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TGraph_SetNPoints", LUA_TUSERDATA, LUA_TNUMBER ) )
    {
        return 0;
    }

    TGraphErrors* graph = * ( reinterpret_cast<TGraphErrors**> ( lua_touserdata ( L, 1 ) ) );

    int npoints = lua_tointeger ( L, 2 );

    graph->Set ( npoints );

    return 0;
}

int luaExt_TGraph_Eval ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TGraph_Eval", LUA_TUSERDATA, LUA_TNUMBER ) )
    {
        return 0;
    }

    TGraphErrors* graph = * ( reinterpret_cast<TGraphErrors**> ( lua_touserdata ( L, 1 ) ) );

    double toEval = lua_tonumber ( L, 2 );

    lua_pushnumber ( L, graph->Eval ( toEval ) );

    return 1;
}

// ------------------------------------------------------ TFile Binder ----------------------------------------------------------- //

int luaExt_NewTFile ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_NewTFile", LUA_TTABLE ) ) return 0;

    lua_getfield ( L, 1, "name" );
    
    if ( !CheckLuaArgs ( L, -1, true, "luaExt_NewTFile argument table: name ", LUA_TSTRING ) ) return 0;

    string fName = lua_tostring ( L, -1 );
    lua_pop ( L, 1 );

    string openOpts = "";

    if ( lua_checkfield ( L, 1, "opts", LUA_TSTRING ) )
    {
        openOpts = lua_tostring ( L, 2 );
        lua_pop ( L, 1 );
    }

    NewUserData<TFile> ( L, fName.c_str(), openOpts.c_str() );

    SetupTObjectMetatable ( L );

    AddMethod ( L, luaExt_TFile_Close, "Close" );
    AddMethod ( L, luaExt_TFile_cd, "cd" );
    AddMethod ( L, luaExt_TFile_ls, "ls" );

    return 1;
}

int luaExt_TFile_Close ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TFile_Close", LUA_TUSERDATA ) )
    {
        return 0;
    }

    TFile* file = GetUserData<TFile> ( L );
    file->Close();

    return 0;
}

int luaExt_TFile_cd ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TFile_cd", LUA_TUSERDATA ) )
    {
        return 0;
    }

    TFile* file = GetUserData<TFile> ( L );
    file->cd();

    return 0;
}

int luaExt_TFile_ls ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TFile_ls", LUA_TUSERDATA ) )
    {
        return 0;
    }

    TFile* file = GetUserData<TFile> ( L );
    file->ls();

    return 0;
}

// ------------------------------------------------------ TCutG Binder ----------------------------------------------------------- //

int luaExt_NewTCutG ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_NewTCutG", LUA_TTABLE ) )
    {
        return 0;
    }

    lua_getfield ( L, 1, "name" );
    if ( !CheckLuaArgs ( L, -1, true, "luaExt_NewTCutG argument table: name ", LUA_TSTRING ) )
    {
        return 0;
    }
    string cutName = lua_tostring ( L, -1 );
    lua_pop ( L, 1 );

    int npts = 0;

    if ( lua_checkfield ( L, 1, "n", LUA_TNUMBER ) )
    {
        npts = lua_tointeger ( L, -1 );
        lua_pop ( L, 1 );
    }

    double* xs;
    double* ys;

    bool validPTable = false;

    if ( npts > 0 )
    {
        validPTable = true;
        xs = new double[npts];
        ys = new double[npts];

        if ( lua_checkfield ( L, 1, "x", LUA_TTABLE ) )
        {
            DoForEach ( L, -1, [&] ( lua_State* L_ )
            {
                if ( !CheckLuaArgs ( L_, -2, true, "luaExt_NewTCutG: x table", LUA_TNUMBER, LUA_TNUMBER ) )
                {
                    return true;
                }

                xs[lua_tointeger ( L_, -2 )-1] = lua_tonumber ( L_, -1 );

                return false;
            } );
        }
        else
        {
            validPTable = false;
        }

        if ( lua_checkfield ( L, 1, "y", LUA_TTABLE ) )
        {
            DoForEach ( L, -1, [&] ( lua_State* L_ )
            {
                if ( !CheckLuaArgs ( L_, -2, true, "luaExt_NewTCutG: y table", LUA_TNUMBER, LUA_TNUMBER ) )
                {
                    return true;
                }

                ys[lua_tointeger ( L_, -2 )-1] = lua_tonumber ( L_, -1 );

                return false;
            } );
        }
        else
        {
            validPTable = false;
        }
    }

    if ( validPTable ) NewUserData<TCutG> ( L, cutName.c_str(), npts, xs, ys );
    else NewUserData<TCutG> ( L, cutName.c_str(), 0 );

    SetupTObjectMetatable ( L );

    AddMethod ( L, luaExt_TCutG_IsInside, "IsInside" );

    return 1;
}

int luaExt_TCutG_IsInside ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TCutG_IsInside", LUA_TUSERDATA, LUA_TNUMBER, LUA_TNUMBER ) )
    {
        return 0;
    }

    TCutG* cutg = * ( reinterpret_cast<TCutG**> ( lua_touserdata ( L, 1 ) ) );

    double x = lua_tonumber ( L, 2 );
    double y = lua_tonumber ( L, 3 );

    lua_pushboolean ( L, cutg->IsInside ( x, y ) );

    return 1;
}

// ------------------------------------------------------ TClonesArray Interface ----------------------------------------------------------- //

int luaExt_NewTClonesArray ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 2, true, "luaExt_NewTClonesArray", LUA_TTABLE ) ) return 0;

    lua_unpackarguments ( L, 1, "luaExt_NewTClonesArray argument table",
    {"class", "size",},
    {LUA_TSTRING, LUA_TNUMBER},
    {true, true} );

    const char* classname = lua_tostring ( L, -2 );
    int size = lua_tointeger ( L, -1 );

    NewUserData<TClonesArray> ( L, classname, size );

    SetupTObjectMetatable ( L );

    AddMethod ( L, luaExt_TClonesArray_ConstructedAt, "At" );

    return 1;
}

int luaExt_TClonesArray_ConstructedAt ( lua_State* L )
{
    TClonesArray* clones = GetUserData<TClonesArray> ( L );

    lua_unpackarguments ( L, 1, "luaExt_TClonesArray_ConstructedAt argument table",
    {"idx",},
    {LUA_TNUMBER},
    {true} );

    int idx = lua_tointeger ( L, -1 );

    clones->ConstructedAt ( idx );

    return 1;
}

// ------------------------------------------------------ TTree Binder ----------------------------------------------------------- //

map<string, function<void ( lua_State*, TTree*, const char* ) >> newBranchFns;

int luaExt_NewTTree ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_NewTTree", LUA_TTABLE ) )
    {
        return 0;
    }

    lua_unpackarguments ( L, 1, "luaExt_NewTTree argument table",
    {"name", "title",},
    {LUA_TSTRING, LUA_TSTRING},
    {true, true} );

    const char* name = lua_tostring ( L, -2 );
    const char* title = lua_tostring ( L, -1 );

    NewUserData<TTree> ( L, name, title );

    SetupTObjectMetatable ( L );

    AddMethod ( L, luaExt_TTree_Fill, "Fill" );
    AddMethod ( L, luaExt_TTree_GetEntries, "GetEntries" );
    AddMethod ( L, luaExt_TTree_Draw, "Draw" );
    AddMethod ( L, luaExt_TTree_GetEntry, "GetEntry" );

    AddMethod ( L, luaExt_TTree_NewBranch_Interface, "NewBranch" );
    AddMethod ( L, luaExt_TTree_GetBranch_Interface, "GetBranch" );

    lua_newtable ( L );
    lua_setfield ( L, -2, "branches_list" );

    return 1;
}

int luaExt_TTree_Fill ( lua_State* L )
{
    TTree* tree = GetUserData<TTree> ( L, 1, "luaExt_TTree_NewBranch" );

    tree->Fill();

    return 0;
}

int luaExt_TTree_GetEntries ( lua_State* L )
{
    TTree* tree = GetUserData<TTree> ( L, 1, "luaExt_TTree_GetEntries" );

    lua_pushinteger ( L, tree->GetEntries() );

    return 1;
}

int luaExt_TTree_Draw ( lua_State* L )
{
    TTree* tree = GetUserData<TTree> ( L, 1, "luaExt_TTree_Draw" );

    lua_unpackarguments ( L, 2, "luaExt_TTree_Draw argument table",
    {"exp", "cond", "opts"},
    {LUA_TSTRING, LUA_TSTRING, LUA_TSTRING},
    {true, false, false} );

    const char* exp = lua_tostring ( L, -3 );
    const char* cond = lua_tostringx ( L, -2 );
    string opts = lua_tostringx ( L, -1 );

    theApp->NotifyUpdatePending();

    if ( opts.find ( "same" ) == string::npos && opts.find ( "SAME" ) == string::npos )
    {
        LuaEmbeddedCanvas* disp = new LuaEmbeddedCanvas();
        disp->cd();
        canvasTracker[ ( TObject* ) tree] = ( TCanvas* ) disp;
    }
    else
    {
        canvasTracker[ ( TObject* ) tree] = gPad->GetCanvas();
    }

    long long entries = tree->Draw ( exp, cond, opts.c_str() );
    lua_pushinteger ( L, entries );

    canvasTracker[ ( TObject* ) tree]->Modified();
    canvasTracker[ ( TObject* ) tree]->Update();

    theApp->NotifyUpdateDone();

    return 1;
}


int luaExt_TTree_GetEntry ( lua_State* L )
{
    TTree* tree = GetUserData<TTree> ( L, 1, "luaExt_TTree_GetEntry" );

    lua_unpackarguments ( L, 2, "luaExt_TTree_Draw argument table",
    {"entry", "getall"},
    {LUA_TNUMBER, LUA_TBOOLEAN},
    {true, false} );

    long long int entry = lua_tointeger ( L, -2 );
    int getall = lua_tointegerx ( L, -1, nullptr );

    tree->GetEntry ( entry, getall );

    return 0;
}

// ------------------------------------------------------ TTree Branch Interface ----------------------------------------------------------- //

const char* GetROOTLeafId ( string btype )
{
    const char* leafTypeId = "";

    if ( btype == "bool" ) leafTypeId = "O";
    else if ( btype == "unsigned short" ) leafTypeId = "s";
    else if ( btype == "short" ) leafTypeId = "S";
    else if ( btype == "unsigned int" ) leafTypeId = "i";
    else if ( btype == "int" ) leafTypeId = "I";
    else if ( btype == "unsigned long" ) leafTypeId = "i";
    else if ( btype == "long" ) leafTypeId = "I";
    else if ( btype == "unsigned long long" ) leafTypeId = "l";
    else if ( btype == "long long" ) leafTypeId = "L";
    else if ( btype == "float" ) leafTypeId = "F";
    else if ( btype == "double" ) leafTypeId = "D";

    return leafTypeId;
}

template<typename T> void MakeNewBranchFuncs ( string type )
{
    string finalType = type;

    newBranchFns[finalType] = [=] ( lua_State* L, TTree* tree, const char* bname )
    {
        luaExt_NewUserData ( L );
        T* branch_ptr = GetUserData<T> ( L, -1, "luaExt_TTree_NewBranch" );
        tree->Branch ( bname, branch_ptr );
    };

    finalType = "vector<" + type + ">";

    newBranchFns[finalType] = [=] ( lua_State* L, TTree* tree, const char* bname )
    {
        luaExt_NewUserData ( L );
        vector<T>* branch_ptr = GetUserData<vector<T>> ( L, -1, "luaExt_TTree_NewBranch" );
        tree->Branch ( bname, branch_ptr );
    };

    finalType = "vector<vector<" + type + ">>";

    newBranchFns[finalType] = [=] ( lua_State* L, TTree* tree, const char* bname )
    {
        luaExt_NewUserData ( L );
        vector<vector<T>>* branch_ptr = GetUserData<vector<vector<T>>> ( L, -1, "luaExt_TTree_NewBranch" );
        tree->Branch ( bname, branch_ptr );
    };

    finalType = type + "[]";

    newBranchFns[finalType] = [=] ( lua_State* L, TTree* tree, const char* bname )
    {
        luaExt_NewUserData ( L );
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

template<typename T> void SetupBranchFuncs ( string type )
{
    MakeNewBranchFuncs<T> ( type );

    MakeAccessorsUserDataFuncs<T> ( type );
}

void InitializeBranchesFuncs()
{
    SetupBranchFuncs<bool> ( "bool" );
    SetupBranchFuncs<short> ( "short" );
    SetupBranchFuncs<unsigned short> ( "unsigned short" );
    SetupBranchFuncs<int> ( "int" );
    SetupBranchFuncs<unsigned int> ( "unsigned int" );
    SetupBranchFuncs<long> ( "long" );
    SetupBranchFuncs<unsigned long> ( "unsigned long" );
    SetupBranchFuncs<long long> ( "long long" );
    SetupBranchFuncs<unsigned long long> ( "unsigned long long" );
    SetupBranchFuncs<float> ( "float" );
    SetupBranchFuncs<double> ( "double" );
}

int luaExt_TTree_NewBranch_Interface ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TTree_NewBranch_Interface", LUA_TUSERDATA, LUA_TTABLE ) ) return 0;

    lua_unpackarguments ( L, 2, "luaExt_TTree_NewBranch_Interface argument table",
    {"type", "name"},
    {LUA_TSTRING, LUA_TSTRING},
    {true, true} );

    lua_remove ( L, 2 );

    const char* bname = lua_tostring ( L, -1 );
    lua_pop ( L, 1 );

    string btype = lua_tostring ( L, -1 );
    string adjust_type = btype;

    if ( btype.find ( "[" ) != string::npos ) adjust_type = btype.substr ( 0, btype.find ( "[" ) ) + "[]";

    TTree* tree = GetUserData<TTree> ( L, 1, "luaExt_TTree_NewBranch" );

    lua_getfield ( L, 1, "branches_list" );
    lua_remove ( L, 1 );

    newBranchFns[adjust_type] ( L, tree, bname );

    lua_pushvalue ( L, -1 );
    lua_setfield ( L, -3, bname );

    return 1;
}

int luaExt_TTree_GetBranch_Interface ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TTree_GetBranch_Interface", LUA_TUSERDATA, LUA_TTABLE ) ) return 0;

    lua_unpackarguments ( L, 2, "luaExt_TTree_GetBranch_Interface argument table",
    {"name"},
    {LUA_TSTRING},
    {true} );

    const char* bname = lua_tostring ( L, -1 );

    lua_getfield ( L, 1, "branches_list" );
    lua_getfield ( L, -1, bname );

    if ( lua_type ( L, -1 ) == LUA_TNIL )
    {
        cerr << "Trying to retrieve branch " << bname << " but it does not exists..." << endl;
        lua_pushnil ( L );
        return 1;
    }

    return 1;
}
















