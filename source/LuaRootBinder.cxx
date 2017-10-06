#include "LuaRootBinder.h"
#include "TPad.h"
#include <llimits.h>
#include <mutex>
#include <readline/readline.h>

map<TObject*, TCanvas*> canvasTracker;

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
        else lua_getglobal ( lua, "_G" );
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

}

extern "C" int luaopen_libLuaXRootlib ( lua_State* L )
{
    lua_getglobal ( L, "_G" );
    luaL_setfuncs ( L, luaXroot_lib, 0 );
    lua_pop ( L, 1 );

    return 0;
}

// -------------------------------------------------- TApplication Binder -------------------------------------------------------- //

void* StartRootEventProcessor ( void* arg )
{
restartruntask:
    theApp->shouldStop = false;

    rootProcessLoopLock.unlock();

    while ( !theApp->shouldStop )
    {
        theApp->StartIdleing();
        gSystem->InnerLoop();
        theApp->StopIdleing();
    }

    rootProcessLoopLock.lock();

    goto restartruntask;

    return nullptr;
}

int luaExt_NewTApplication ( lua_State* L )
{
    if ( theApp == nullptr )
    {
        //         cout << "theApp == nullptr, initializing it..." << endl;
        lua = L;
        rl_attempted_completion_function = input_completion;

        RootAppThreadManager** tApp = reinterpret_cast<RootAppThreadManager**> ( lua_newuserdata ( L, sizeof ( RootAppThreadManager* ) ) );
        *tApp = new RootAppThreadManager ( "tapp", nullptr, nullptr );

        theApp = *tApp;

        lua_newtable ( L );

        lua_pushvalue ( L, -1 );
        lua_setfield ( L, -2, "__index" );

        lua_pushvalue ( L, -1 );
        lua_setfield ( L, -2, "__newindex" );

        lua_setmetatable ( L, -2 );

        lua_pushcfunction ( L, luaExt_TApplication_Run );
        lua_setfield ( L, -2, "Run" );

        lua_pushcfunction ( L, luaExt_TApplication_Update );
        lua_setfield ( L, -2, "Update" );

        lua_pushcfunction ( L, luaExt_TApplication_Terminate );
        lua_setfield ( L, -2, "Terminate" );

        pthread_t startRootEventProcessrTask;

        pthread_create ( &startRootEventProcessrTask, nullptr, StartRootEventProcessor, nullptr );

        return 1;
    }
    else return 0;
}

int luaExt_TApplication_Run ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TApplication_Run", LUA_TUSERDATA ) ) return 0;

    TApplication* tApp = * ( reinterpret_cast<TApplication**> ( lua_touserdata ( L, 1 ) ) );

    tApp->Run();

    return 0;
}

int luaExt_TApplication_Update ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TApplication_Update", LUA_TUSERDATA ) ) return 0;

    TApplication* tApp = * ( reinterpret_cast<TApplication**> ( lua_touserdata ( L, 1 ) ) );

    tApp->StartIdleing();
    gSystem->InnerLoop();
    tApp->StopIdleing();

    return 0;
}

int luaExt_TApplication_Terminate ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TApplication_Terminate", LUA_TUSERDATA ) ) return 0;

    TApplication* tApp = * ( reinterpret_cast<TApplication**> ( lua_touserdata ( L, 1 ) ) );

    tApp->Terminate();

    return 0;
}

// ------------------------------------------------------- TObject Binder ------------------------------------------------------- //

int luaExt_NewTObject ( lua_State* L )
{
    //     cout << "Tree name / title : " << treeName << " / " << treeTitle << endl;

    TObject** obj = reinterpret_cast<TObject**> ( lua_newuserdata ( L, sizeof ( TObject* ) ) );
    *obj = new TObject ();

    lua_newtable ( L );

    lua_pushvalue ( L, -1 );
    lua_setfield ( L, -2, "__index" );

    lua_pushvalue ( L, -1 );
    lua_setfield ( L, -2, "__newindex" );

    lua_setmetatable ( L, -2 );

    lua_pushcfunction ( L, luaExt_TObject_Draw );
    lua_setfield ( L, -2, "Draw" );

    lua_pushcfunction ( L, luaExt_TObject_Update );
    lua_setfield ( L, -2, "Update" );

    return 1;
}

int luaExt_TObject_Draw ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TObject_Draw", LUA_TUSERDATA ) ) return 0;

    if ( lua_gettop ( L ) == 2 && !CheckLuaArgs ( L, 1, true, "luaExt_TObject_Draw", LUA_TSTRING ) ) return 0;

    //     cout << "Executing luaExt_TObject_Draw" << endl;

    TObject* obj = * ( static_cast<TObject**> ( lua_touserdata ( L, 1 ) ) );

    string opts = "";

    if ( lua_gettop ( L ) == 2 ) opts = lua_tostring ( L, 2 );

    rootProcessLoopLock.lock();

    theApp->shouldStop = true;
    //     cout << "asked to stop" << endl;

    if ( opts.find ( "same" ) == string::npos && opts.find ( "SAME" ) == string::npos )
    {
        LuaEmbeddedCanvas* disp = new LuaEmbeddedCanvas();
        disp->cd();
    }
    obj->Draw ( opts.c_str() );
    canvasTracker[obj] = gPad->GetCanvas();

    rootProcessLoopLock.unlock();

    return 0;
}

int luaExt_TObject_Update ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TObject_Update", LUA_TUSERDATA ) ) return 0;

    TObject* obj = * ( static_cast<TObject**> ( lua_touserdata ( L, 1 ) ) );

    rootProcessLoopLock.lock();

    theApp->shouldStop = true;
    //     cout << "asked to stop" << endl;

    canvasTracker[obj]->Modified();
    canvasTracker[obj]->Update();

    rootProcessLoopLock.unlock();

    return 0;
}

// ------------------------------------------------------ TF1 Binder -------------------------------------------------------------- //

int luaExt_NewTF1 ( lua_State* L )
{
    if ( lua_gettop ( L ) == 0 )
    {
        TF1** func = reinterpret_cast<TF1**> ( lua_newuserdata ( L, sizeof ( TF1* ) ) );
        *func = new TF1 ( );
        return 1;
    }
    else if ( !CheckLuaArgs ( L, 1, true, "luaExt_NewTF1", LUA_TTABLE ) ) return 0;

    lua_getfield ( L, 1, "name" );

    if ( !CheckLuaArgs ( L, -1, true, "luaExt_NewTF1 arguments table: name ", LUA_TSTRING ) ) return 0;

    string fname = lua_tostring ( L, -1 );
    lua_pop ( L, 1 );

    string fformula = "";
    function<double ( double*,double* ) > fn = nullptr;
    double xmin = 0;
    double xmax = 1;
    int npars = 0;
    int ndim = 1;

    if ( lua_checkfield ( L, 1, "formula", LUA_TSTRING ) )
    {
        fformula = lua_tostring ( L, -1 );
        lua_pop ( L, 1 );
    }

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

    if ( lua_checkfield ( L, 1, "ndim", LUA_TNUMBER ) )
    {
        ndim = lua_tointeger ( L, -1 );
        lua_pop ( L, 1 );
    }

    if ( fformula.empty() )
    {
        lua_getfield ( L, 1, "npars" );
        if ( !CheckLuaArgs ( L, -1, true, "luaExt_NewTF1 arguments table: npars required here ", LUA_TNUMBER ) ) return 0;
        npars = lua_tointeger ( L, -1 );
        lua_pop ( L, 1 );

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

    TF1** func = reinterpret_cast<TF1**> ( lua_newuserdata ( L, sizeof ( TF1* ) ) );

    lua_newtable ( L );

    lua_pushvalue ( L, -1 );
    lua_setfield ( L, -2, "__index" );

    lua_pushvalue ( L, -1 );
    lua_setfield ( L, -2, "__newindex" );

    lua_setmetatable ( L, -2 );

    lua_pushcfunction ( L, luaExt_TF1_Eval );
    lua_setfield ( L, -2, "Eval" );

    lua_pushcfunction ( L, luaExt_TF1_SetParameters );
    lua_setfield ( L, -2, "SetParameters" );

    lua_pushcfunction ( L, luaExt_TObject_Draw );
    lua_setfield ( L, -2, "Draw" );

    lua_pushcfunction ( L, luaExt_TObject_Update );
    lua_setfield ( L, -2, "Update" );

    lua_pushcfunction ( L, luaExt_TF1_GetPars );
    lua_setfield ( L, -2, "GetParameters" );

    lua_pushcfunction ( L, luaExt_TF1_GetChi2 );
    lua_setfield ( L, -2, "GetChi2" );


    if ( !fformula.empty() ) *func = new TF1 ( fname.c_str(), fformula.c_str(), xmin, xmax );
    else if ( fn != nullptr ) *func = new TF1 ( fname.c_str(), fn, xmin, xmax, npars, ndim );
    else
    {
        cerr << "Error in luaExt_NewTF1: missing required field in argument table: \"formula\" or \"fn\"" << endl;
        lua_settop ( L, 0 );
        return 0;
    }

    return 1;
}

int luaExt_TF1_SetParameters ( lua_State* L )
{
    if ( lua_gettop ( L ) < 2 || !CheckLuaArgs ( L, 1, true, "luaExt_TF1_SetParameters", LUA_TUSERDATA ) ) return 0;

    TF1* func = * ( reinterpret_cast<TF1**> ( lua_touserdata ( L, 1 ) ) );

    if ( lua_type ( L, 2 ) == LUA_TTABLE )
    {
        DoForEach ( L, 2, [&] ( lua_State* L_ )
        {
            if ( !CheckLuaArgs ( L_, 1, true, "luaExt_TF1_SetParameters: parameters table ", LUA_TNUMBER, LUA_TNUMBER ) ) return true;

            int parIdx = lua_tointeger ( L_, -2 );
            double parVal = lua_tonumber ( L_, -1 );
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
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TF1_Eval", LUA_TUSERDATA, LUA_TNUMBER ) ) return 0;

    TF1* func = * ( reinterpret_cast<TF1**> ( lua_touserdata ( L, 1 ) ) );

    double toEval = lua_tonumber ( L, 2 );

    lua_pushnumber ( L, func->Eval ( toEval ) );

    return 1;
}

int luaExt_TF1_GetPars ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TF1_GetPars", LUA_TUSERDATA ) ) return 0;

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
    else if ( !CheckLuaArgs ( L, 2, true, "luaExt_TF1_GetPars", LUA_TTABLE ) ) return 0;
    else
    {
        lua_newtable ( L );
        DoForEach ( L, 2, [&] ( lua_State* L_ )
        {
            if ( !CheckLuaArgs ( L_, -2, true, "luaExt_TF1_GetPars: parameters list ", LUA_TNUMBER, LUA_TNUMBER ) ) return true;

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
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TF1_GetPars", LUA_TUSERDATA ) ) return 0;

    TF1* func = * ( reinterpret_cast<TF1**> ( lua_touserdata ( L, 1 ) ) );

    lua_pushnumber ( L, func->GetChisquare() );

    return 1;
}

// -------------------------------------------------- TH[istograms] Binder -------------------------------------------------------- //

int luaExt_NewTHist ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_NewTHist", LUA_TTABLE ) ) return 0;

    string name, title;
    double xmin, xmax, nbinsx, ymin = 0, ymax = 0, nbinsy = 0;


    lua_getfield ( L, 1, "name" );
    if ( !CheckLuaArgs ( L, -1, true, "luaExt_NewTHist argument table: name", LUA_TSTRING ) ) return 0;
    name = lua_tostring ( L, -1 );
    lua_pop ( L, 1 );

    lua_getfield ( L, 1, "title" );
    if ( !CheckLuaArgs ( L, -1, true, "luaExt_NewTHist argument table: title", LUA_TSTRING ) ) return 0;
    title = lua_tostring ( L, -1 );
    lua_pop ( L, 1 );

    lua_getfield ( L, 1, "xmin" );
    if ( !CheckLuaArgs ( L, -1, true, "luaExt_NewTHist argument table: xmin", LUA_TNUMBER ) ) return 0;
    xmin = lua_tonumber ( L, -1 );
    lua_pop ( L, 1 );

    lua_getfield ( L, 1, "xmax" );
    if ( !CheckLuaArgs ( L, -1, true, "luaExt_NewTHist argument table: xmax", LUA_TNUMBER ) ) return 0;
    xmax = lua_tonumber ( L, -1 );
    lua_pop ( L, 1 );

    lua_getfield ( L, 1, "nbinsx" );
    if ( !CheckLuaArgs ( L, -1, true, "luaExt_NewTHist argument table: nbinsx", LUA_TNUMBER ) ) return 0;
    nbinsx = lua_tointeger ( L, -1 );
    lua_pop ( L, 1 );

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

    if ( lua_checkfield ( L, 1, "nbinsy", LUA_TNUMBER ) )
    {
        nbinsy = lua_tonumber ( L, -1 );
        lua_pop ( L, 1 );
    }

    if ( nbinsy == 0 && nbinsx > 0 && xmax > xmin )
    {
        TH1D** hist = reinterpret_cast<TH1D**> ( lua_newuserdata ( L, sizeof ( TH1D* ) ) );
        *hist = new TH1D ( name.c_str(), title.c_str(), nbinsx, xmin, xmax );
    }
    else if ( ymax > ymin )
    {
        TH2D** hist = reinterpret_cast<TH2D**> ( lua_newuserdata ( L, sizeof ( TH2D* ) ) );
        *hist = new TH2D ( name.c_str(), title.c_str(), nbinsx, xmin, xmax, nbinsy, ymin, ymax );
    }
    else
    {
        cerr << "Error in luaExt_NewTHist: argument table: invalid boundaries or number of bins" << endl;
        lua_settop ( L, 0 );
        return 0;
    }

    lua_newtable ( L );

    lua_pushvalue ( L, -1 );
    lua_setfield ( L, -2, "__index" );

    lua_pushvalue ( L, -1 );
    lua_setfield ( L, -2, "__newindex" );

    lua_setmetatable ( L, -2 );

    lua_pushcfunction ( L, luaExt_THist_Clone );
    lua_setfield ( L, -2, "Clone" );

    lua_pushcfunction ( L, luaExt_THist_Fill );
    lua_setfield ( L, -2, "Fill" );

    lua_pushcfunction ( L, luaExt_TObject_Draw );
    lua_setfield ( L, -2, "Draw" );

    lua_pushcfunction ( L, luaExt_TObject_Update );
    lua_setfield ( L, -2, "Update" );

    lua_pushcfunction ( L, luaExt_THist_Add );
    lua_setfield ( L, -2, "Add" );

    lua_pushcfunction ( L, luaExt_THist_Scale );
    lua_setfield ( L, -2, "Scale" );

    lua_pushcfunction ( L, luaExt_THist_SetRangeUser );
    lua_setfield ( L, -2, "SetRangeUser" );

    lua_pushcfunction ( L, luaExt_THist_Fit );
    lua_setfield ( L, -2, "Fit" );

    return 1;
}

int luaExt_THist_Clone ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, -1, true, "luaExt_THist_Clone", LUA_TUSERDATA ) ) return 0;

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
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_THist_Fill", LUA_TUSERDATA, LUA_TTABLE ) ) return 0;

    TH1* hist = * ( reinterpret_cast<TH1**> ( lua_touserdata ( L, 1 ) ) );

    double x, y;
    int w = 1;

    lua_getfield ( L, 2, "x" );
    if ( !CheckLuaArgs ( L, -1, true, "luaExt_THist_Fill argument table: x", LUA_TNUMBER ) && !CheckLuaArgs ( L, -1, true, "luaExt_THist_Fill argument table: x", LUA_TNUMBER ) ) return 0;
    x = lua_type ( L, -1 ) == LUA_TNUMBER ? lua_tonumber ( L, -1 ) : lua_toboolean ( L, -1 );
    lua_pop ( L, 1 );

    if ( lua_checkfield ( L, 1, "w", LUA_TNUMBER ) )
    {
        w = lua_tointeger ( L, -1 );
        lua_pop ( L, 1 );
    }

    if ( lua_checkfield ( L, 1, "y", LUA_TNUMBER ) || lua_checkfield ( L, 1, "y", LUA_TBOOLEAN ) )
    {
        y = lua_type ( L, -1 ) == LUA_TNUMBER ? lua_tonumber ( L, -1 ) : lua_toboolean ( L, -1 );
        lua_pop ( L, 1 );
        ( ( TH2D* ) hist )->Fill ( x, y, w );
        if ( gPad != nullptr ) gPad->Update();
        return 0;
    }
    else
    {
        hist->Fill ( x, w );
        if ( gPad != nullptr ) gPad->Update();
        return 0;
    }
}

int luaExt_THist_Add ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, -1, true, "luaExt_THist_Add", LUA_TUSERDATA, LUA_TUSERDATA, LUA_TNUMBER ) ) return 0;

    TH1* hist = * ( reinterpret_cast<TH1**> ( lua_touserdata ( L, 1 ) ) );
    TH1* hist2 = * ( reinterpret_cast<TH1**> ( lua_touserdata ( L, 2 ) ) );

    double scale = 1.0;

    if ( lua_gettop ( L ) == 3 ) scale = lua_tonumber ( L, 3 );

    hist->Add ( hist2, scale );

    return 0;
}

int luaExt_THist_Scale ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, -1, true, "luaExt_THist_Add", LUA_TUSERDATA, LUA_TNUMBER ) ) return 0;

    TH1* hist = * ( reinterpret_cast<TH1**> ( lua_touserdata ( L, 1 ) ) );

    double scale = lua_tonumber ( L, 2 );

    hist->Scale ( scale );

    return 0;
}

int luaExt_THist_SetRangeUser ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, -1, true, "luaExt_THist_SetRangeUser", LUA_TUSERDATA, LUA_TTABLE ) ) return 0;

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
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_THist_Fit", LUA_TUSERDATA, LUA_TTABLE ) ) return 0;

    TH1* hist = * ( reinterpret_cast<TH1**> ( lua_touserdata ( L, 1 ) ) );

    TF1* fitfunc = nullptr;

    lua_getfield ( L, 2, "fn" );

    if ( !CheckLuaArgs ( L, -1, true, "luaExt_THist_Fit argument table: field fn ", LUA_TUSERDATA ) ) return 0;

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
    if ( gPad != nullptr ) gPad->Update();

    return 1;
}

int luaExt_THist_Reset ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_THist_Reset", LUA_TUSERDATA ) ) return 0;

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
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_NewTGraph", LUA_TTABLE ) ) return 0;

    int npoints;
    string name, title;
    double* xs, *ys, *dxs, *dys;

    lua_getfield ( L, 1, "n" );

    if ( !CheckLuaArgs ( L, -1, true, "luaExt_NewTGraph argument table: field n ", LUA_TNUMBER ) ) return 0;

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
            if ( !CheckLuaArgs ( L, -2, true, "luaExt_NewTGraph while setting up points : invalid key for points table ", LUA_TNUMBER ) ) return true;
            if ( !CheckLuaArgs ( L, -1, true, "luaExt_NewTGraph while setting up points : invalid format for points table ", LUA_TTABLE ) ) return true;

            int pindex = lua_tointeger ( L_, -2 )-1;

            lua_getfield ( L, -1, "x" );
            if ( !CheckLuaArgs ( L, -1, true, "luaExt_NewTGraph while setting up points : invalid value for x ", LUA_TNUMBER ) ) return true;
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
            if ( !CheckLuaArgs ( L, -1, true, "luaExt_NewTGraph while setting up points : invalid value for y ", LUA_TNUMBER ) ) return true;
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
                if ( !CheckLuaArgs ( L_, -2, true, "luaExt_NewTGraph: x table", LUA_TNUMBER, LUA_TNUMBER ) ) return true;

                xs[lua_tointeger ( L_, -2 )-1] = lua_tonumber ( L_, -1 );

                return false;
            } );
            lua_pop ( L, 1 );
        }
        else points_specified = false;

        if ( lua_checkfield ( L, 1, "y", LUA_TTABLE ) )
        {
            DoForEach ( L, -1, [&] ( lua_State* L_ )
            {
                if ( !CheckLuaArgs ( L_, -2, true, "luaExt_NewTGraph: y table", LUA_TNUMBER, LUA_TNUMBER ) ) return true;

                ys[lua_tointeger ( L_, -2 )-1] = lua_tonumber ( L_, -1 );

                return false;
            } );
            lua_pop ( L, 1 );
        }
        else points_specified = false;

        if ( lua_checkfield ( L, 1, "dx", LUA_TTABLE ) )
        {
            DoForEach ( L, -1, [&] ( lua_State* L_ )
            {
                if ( !CheckLuaArgs ( L_, -2, true, "luaExt_NewTGraph: dx table", LUA_TNUMBER, LUA_TNUMBER ) ) return true;

                dxs[lua_tointeger ( L_, -2 )-1] = lua_tonumber ( L_, -1 );

                return false;
            } );
            lua_pop ( L, 1 );
        }
        else graph_has_errors = false;

        if ( lua_checkfield ( L, 1, "dy", LUA_TTABLE ) )
        {
            DoForEach ( L, -1, [&] ( lua_State* L_ )
            {
                if ( !CheckLuaArgs ( L_, -2, true, "luaExt_NewTGraph: dy table", LUA_TNUMBER, LUA_TNUMBER ) ) return true;

                dys[lua_tointeger ( L_, -2 )-1] = lua_tonumber ( L_, -1 );

                return false;
            } );
            lua_pop ( L, 1 );
        }
        else graph_has_errors = false;
    }

    if ( npoints <= 0 )
    {
        cerr << "Error in luaExt_NewTGraph: invalid amount of points specified" << endl;
        lua_settop ( L, 0 );
        return 0;
    }

    TGraphErrors** graph = reinterpret_cast<TGraphErrors**> ( lua_newuserdata ( L, sizeof ( TGraphErrors* ) ) );

    if ( !points_specified )
    {
        *graph = new TGraphErrors ( npoints );
    }
    else
    {
        if ( graph_has_errors )
        {
            *graph = new TGraphErrors ( npoints, xs, ys, dxs, dys );
        }
        else
        {
            *graph = new TGraphErrors ( npoints, xs, ys );
        }
    }

    ( *graph )->SetEditable ( false );

    lua_newtable ( L );

    lua_pushvalue ( L, -1 );
    lua_setfield ( L, -2, "__index" );

    lua_pushvalue ( L, -1 );
    lua_setfield ( L, -2, "__newindex" );

    lua_setmetatable ( L, -2 );

    lua_pushcfunction ( L, luaExt_TGraph_SetTitle );
    lua_setfield ( L, -2, "SetTitle" );

    lua_pushcfunction ( L, luaExt_TObject_Draw );
    lua_setfield ( L, -2, "Draw" );

    lua_pushcfunction ( L, luaExt_TObject_Update );
    lua_setfield ( L, -2, "Update" );

    lua_pushcfunction ( L, luaExt_TGraph_Fit );
    lua_setfield ( L, -2, "Fit" );

    lua_pushcfunction ( L, luaExt_TGraph_SetPoint );
    lua_setfield ( L, -2, "SetPoint" );

    lua_pushcfunction ( L, luaExt_TGraph_GetPoint );
    lua_setfield ( L, -2, "GetPoint" );

    lua_pushcfunction ( L, luaExt_TGraph_RemovePoint );
    lua_setfield ( L, -2, "RemovePoint" );

    lua_pushcfunction ( L, luaExt_TGraph_SetNPoints );
    lua_setfield ( L, -2, "SetNPoints" );

    lua_pushcfunction ( L, luaExt_TGraph_Eval );
    lua_setfield ( L, -2, "SetNPoints" );

    return 1;
}

int luaExt_TGraph_SetTitle ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TGraph_SetTitle", LUA_TUSERDATA, LUA_TSTRING ) ) return 0;

    TGraphErrors* graph = * ( reinterpret_cast<TGraphErrors**> ( lua_touserdata ( L, 1 ) ) );
    string title = lua_tostring ( L, 2 );

    graph->SetTitle ( title.c_str() );

    return 0;
}

int luaExt_TGraph_Draw ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TGraph_Draw", LUA_TUSERDATA ) ) return 0;
    if ( lua_gettop ( L ) == 2 && !CheckLuaArgs ( L, 2, true, "luaExt_TGraph_Draw", LUA_TSTRING ) ) return 0;

    TGraphErrors* graph = * ( reinterpret_cast<TGraphErrors**> ( lua_touserdata ( L, 1 ) ) );

    string opts = "";

    if ( lua_gettop ( L ) == 2 ) opts = lua_tostring ( L, 2 );

    graph->Draw ( opts.c_str() );

    return 0;
}

int luaExt_TGraph_Fit ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TGraph_Fit", LUA_TUSERDATA, LUA_TTABLE ) ) return 0;

    TGraphErrors* graph = * ( reinterpret_cast<TGraphErrors**> ( lua_touserdata ( L, 1 ) ) );

    TF1* fitfunc = nullptr;

    lua_getfield ( L, 2, "fn" );

    if ( !CheckLuaArgs ( L, -1, true, "luaExt_TGraph_Fit argument table: field fn ", LUA_TUSERDATA ) ) return 0;

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
    if ( gPad != nullptr ) gPad->Update();

    return 0;
}

int luaExt_TGraph_SetPoint ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TGraph_SetPoint", LUA_TUSERDATA, LUA_TTABLE ) ) return 0;

    TGraphErrors* graph = * ( reinterpret_cast<TGraphErrors**> ( lua_touserdata ( L, 1 ) ) );

    lua_getfield ( L, 2, "i" );
    if ( !CheckLuaArgs ( L, -1, true, "luaExt_TGraph_SetPoint argument table: i ", LUA_TNUMBER ) ) return 0;
    int idx = lua_tointeger ( L, -1 );
    lua_pop ( L, 1 );

    lua_getfield ( L, 2, "x" );
    if ( !CheckLuaArgs ( L, -1, true, "luaExt_TGraph_SetPoint argument table: x ", LUA_TNUMBER ) ) return 0;
    double x = lua_tonumber ( L, -1 );
    lua_pop ( L, 1 );

    lua_getfield ( L, 2, "y" );
    if ( !CheckLuaArgs ( L, -1, true, "luaExt_TGraph_SetPoint argument table: y ", LUA_TNUMBER ) ) return 0;
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

    if ( dx > 0 || dy > 0 ) graph->SetPointError ( idx, dx ,dy );

    return 0;
}

int luaExt_TGraph_GetPoint ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TGraph_GetPoint", LUA_TUSERDATA, LUA_TNUMBER ) ) return 0;

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
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TGraph_RemovePoint", LUA_TUSERDATA, LUA_TNUMBER ) ) return 0;

    TGraphErrors* graph = * ( reinterpret_cast<TGraphErrors**> ( lua_touserdata ( L, 1 ) ) );

    int idx = lua_tointeger ( L, 2 );

    graph->RemovePoint ( idx );

    return 0;
}

int luaExt_TGraph_SetNPoints ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TGraph_SetNPoints", LUA_TUSERDATA, LUA_TNUMBER ) ) return 0;

    TGraphErrors* graph = * ( reinterpret_cast<TGraphErrors**> ( lua_touserdata ( L, 1 ) ) );

    int npoints = lua_tointeger ( L, 2 );

    graph->Set ( npoints );

    return 0;
}

int luaExt_TGraph_Eval ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TGraph_Eval", LUA_TUSERDATA, LUA_TNUMBER ) ) return 0;

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

    TFile** file = reinterpret_cast<TFile**> ( lua_newuserdata ( L, sizeof ( TFile* ) ) );
    *file = new TFile ( fName.c_str(), openOpts.c_str() );

    lua_newtable ( L );

    lua_pushvalue ( L, -1 );
    lua_setfield ( L, -2, "__index" );

    lua_pushvalue ( L, -1 );
    lua_setfield ( L, -2, "__newindex" );

    lua_setmetatable ( L, -2 );

    lua_pushcfunction ( L, luaExt_TFile_Write );
    lua_setfield ( L, -2, "Write" );

    lua_pushcfunction ( L, luaExt_TFile_Close );
    lua_setfield ( L, -2, "Close" );

    return 1;
}

int luaExt_TFile_Write ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TFile_Write", LUA_TUSERDATA ) ) return 0;

    TFile* file = * ( reinterpret_cast<TFile**> ( lua_touserdata ( L, 1 ) ) );
    file->Write();

    return 0;
}

int luaExt_TFile_Close ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_TFile_Close", LUA_TUSERDATA ) ) return 0;

    TFile* file = * ( reinterpret_cast<TFile**> ( lua_touserdata ( L, 1 ) ) );
    file->Close();

    return 0;
}

// ------------------------------------------------------ TCutG Binder ----------------------------------------------------------- //

int luaExt_NewTCutG ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "luaExt_NewTCutG", LUA_TTABLE ) ) return 0;

    lua_getfield ( L, 1, "name" );
    if ( !CheckLuaArgs ( L, -1, true, "luaExt_NewTCutG argument table: name ", LUA_TSTRING ) ) return 0;
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
                if ( !CheckLuaArgs ( L_, -2, true, "luaExt_NewTCutG: x table", LUA_TNUMBER, LUA_TNUMBER ) ) return true;

                xs[lua_tointeger ( L_, -2 )-1] = lua_tonumber ( L_, -1 );

                return false;
            } );
        }
        else validPTable = false;

        if ( lua_checkfield ( L, 1, "y", LUA_TTABLE ) )
        {
            DoForEach ( L, -1, [&] ( lua_State* L_ )
            {
                if ( !CheckLuaArgs ( L_, -2, true, "luaExt_NewTCutG: y table", LUA_TNUMBER, LUA_TNUMBER ) ) return true;

                ys[lua_tointeger ( L_, -2 )-1] = lua_tonumber ( L_, -1 );

                return false;
            } );
        }
        else validPTable = false;
    }

    TCutG** cut = reinterpret_cast<TCutG**> ( lua_newuserdata ( L, sizeof ( TCutG* ) ) );

    if ( validPTable ) *cut = new TCutG ( cutName.c_str(), npts, xs, ys );
    else *cut = new TCutG ( cutName.c_str(), 0 );

    lua_newtable ( L );

    lua_pushvalue ( L, -1 );
    lua_setfield ( L, -2, "__index" );

    lua_pushvalue ( L, -1 );
    lua_setfield ( L, -2, "__newindex" );

    lua_setmetatable ( L, -2 );

    lua_pushcfunction ( L, luaExt_TCutG_IsInside );
    lua_setfield ( L, -2, "IsInside" );

    return 1;
}

int luaExt_TCutG_IsInside ( lua_State* L )
{
    return 1;
}

