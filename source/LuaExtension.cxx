#include "LuaExtension.h"
#include <llex.h>

lua_State* lua = 0;
int lastCallNReturn = 0;

void InitLuaEnv ( string pathToGDAQ_ )
{
    lua = luaL_newstate();
    luaL_openlibs ( lua );

    lua_register ( lua, "ls", LuaListDirContent );

    string lua_path = "";

    lua_path += "?;?.lua";
    if ( !pathToGDAQ_.empty() )
    {
        lua_path += ";" + pathToGDAQ_ + "/scripts/lua_modules/?;" + pathToGDAQ_ + "/scripts/lua_modules/?.lua;";
        lua_path += pathToGDAQ_ + "/user/lua_scripts/?;" + pathToGDAQ_ + "/user/lua_scripts/?.lua";
    }

    lua_getglobal ( lua, "package" );
    lua_getfield ( lua, -1, "path" );

    if ( lua_type ( lua, -1 ) == LUA_TSTRING ) lua_path = ( string ) lua_tostring ( lua, -1 ) + ";" + lua_path;
    lua_pop ( lua, 1 );

    lua_pushstring ( lua, lua_path.c_str() );

    lua_setfield ( lua, -2, "path" );
    lua_pop ( lua, 1 );

    cout << "Lua Env initialized..." << endl;
}

void LoadLuaFile ( string fname, string modname )
{
    if ( lua == nullptr ) InitLuaEnv();

    int stack_before = lua_gettop ( lua );

    int ret = luaL_loadfile ( lua, fname.c_str() );

    if ( ret != 0 )
    {
        cerr << "ERROR while loading " << fname << endl;
        cerr << lua_tostring ( lua, -1 ) << endl;
        return;
    }

    if ( !modname.empty() )
    {
        lua_newtable ( lua );
        lua_newtable ( lua );
        lua_getglobal ( lua, "_G" );
        lua_setfield ( lua, -2, "__index" );
        lua_setmetatable ( lua, -2 );
        lua_setglobal ( lua, modname.c_str() );

        lua_getglobal ( lua, modname.c_str() );

        lua_setupvalue ( lua, -2, 1 );
    }

    ret = lua_pcall ( lua, 0, LUA_MULTRET, 0 );

    if ( ret != 0 )
    {
        cerr << "ERROR while calling " << fname << endl;
        cerr << lua_tostring ( lua, -1 ) << endl;
        return;
    }

    if ( lua_gettop ( lua ) == stack_before +1 )
    {
        if ( !modname.empty() ) lua_setglobal ( lua, modname.c_str() );
        else
        {
            cerr << "ERROR: if lua script has a return statement, a module name has to be specified..." << endl;
            return;
        }
    }
    else if ( lua_gettop ( lua ) >= stack_before +1 )
    {
        cerr << "ERROR: multiple returns not supported for LoadLuaFile function..." << endl;
        return;
    }
}

void TryGetGlobalField ( lua_State *L, string gfield )
{
    if ( gfield.empty() )
    {
        lua_pushnil ( L );
        return;
    }

    lua_getglobal ( L, "_G" );

    if ( lua_type ( L, -1 ) == LUA_TNIL )
    {
        cerr << "Issue while retrieving global environment... aborting..." << endl;
        lua_pop ( L, 1 );
        return;
    }

    vector<string> chain;

    size_t sepPos = gfield.find_first_of ( "." );

    if ( sepPos == string::npos ) chain.push_back ( gfield );
    else
    {
        chain.push_back ( gfield.substr ( 0, sepPos ) );
        size_t nextSepPos = gfield.find_first_of ( ".", sepPos+1 );

        while ( nextSepPos != string::npos )
        {
            chain.push_back ( gfield.substr ( sepPos+1, nextSepPos-sepPos-1 ) );
            sepPos = nextSepPos;
            nextSepPos = gfield.find_first_of ( ".", sepPos+1 );
        }

        chain.push_back ( gfield.substr ( sepPos+1 ) );
    }

    for ( unsigned int i = 0; i < chain.size(); i++ )
    {
//         cout << "Getting field: " << chain[i] << endl;
        lua_getfield ( L, -1, chain[i].c_str() );
        lua_remove ( L, -2 );
        if ( lua_type ( L, -1 ) == LUA_TNIL )
        {
            return;
        }
//         cout << "Retrieved field: " << chain[i] << endl;
    }
}

bool TrySetGlobalField ( lua_State *L, string gfield )
{
    if ( gfield.empty() ) return false;

    lua_getglobal ( L, "_G" );

    if ( lua_type ( L, -1 ) == LUA_TNIL )
    {
        cerr << "Issue while retrieving global environment... aborting..." << endl;
        lua_pop ( L, 1 );
        return false;
    }

    vector<string> chain;

    size_t sepPos = gfield.find_first_of ( "." );

    if ( sepPos == string::npos ) chain.push_back ( gfield );
    else
    {
        chain.push_back ( gfield.substr ( 0, sepPos ) );
        size_t nextSepPos = gfield.find_first_of ( ".", sepPos+1 );

        while ( nextSepPos != string::npos )
        {
            chain.push_back ( gfield.substr ( sepPos+1, nextSepPos-sepPos-1 ) );
            sepPos = nextSepPos;
            nextSepPos = gfield.find_first_of ( ".", sepPos+1 );
        }

        chain.push_back ( gfield.substr ( sepPos+1 ) );
    }

    for ( unsigned int i = 0; i < chain.size()-1; i++ )
    {
        lua_getfield ( L, -1, chain[i].c_str() );

        if ( lua_type ( L, -1 ) == LUA_TNIL )
        {
            lua_pop ( L, 1 );
//             cout << "TrySetGlobalField: field " << chain[i] << " does not exists. Creating a table for it..." << endl;
            lua_newtable ( L );
            lua_pushvalue ( L, -1 );
            lua_setfield ( L, -3, chain[i].c_str() );
        }
        else if ( lua_type ( L, -1 ) != LUA_TTABLE )
        {
            cerr << "attempt to assign a field to " << chain[i] << ": a " << lua_typename ( L, lua_type ( L, -1 ) ) << endl;
            lua_pop ( L, 1 );
            return false;
        }

        lua_remove ( L, -2 );
    }

    lua_insert ( L, -2 );
//     cout << "TrySetGlobalField: Setting last field " << chain.back() << endl;
    lua_setfield ( L, -2, chain.back().c_str() );
    lua_pop ( L, 1 );

    return true;
}

void DoForEach ( lua_State *L, int index, function<bool ( lua_State *L_ ) > dofn )
{
    if ( index < 0 ) index = lua_gettop ( L ) + index + 1;

    if ( lua_type ( L, index ) == LUA_TTABLE )
    {
        lua_pushnil ( L );

        while ( lua_next ( L, index ) != 0 )
        {
            bool stop = dofn ( L );

            lua_pop ( L, 1 );

            if ( stop )
            {
                lua_pop ( L, 1 );
                return;
            }
        }
    }
    else
    {
        dofn ( L );
        return;
    }
}

void* GetCallResult ( lua_State* L, int index )
{
    index = lua_gettop ( L ) - lastCallNReturn + index;

    if ( index > lua_gettop ( L ) )
    {
        cerr << "Index required (" << index << ") bigger than stack size (" << lua_gettop ( L ) << ")" << endl;
        return nullptr;
    }

    void* result;

    if ( lua_type ( L, index ) == LUA_TUSERDATA ) result = * ( reinterpret_cast<void**> ( lua_touserdata ( L, index ) ) );
    else if ( lua_type ( L, index ) == LUA_TNUMBER && lua_isinteger ( L, index ) )
    {
        int* res = new int;
        *res = lua_tointeger ( L, index );
        result = ( void* ) res;
    }
    else if ( lua_type ( L, index ) == LUA_TNUMBER )
    {
        double* res = new double;
        *res = lua_tonumber ( L, index );
        result = ( void* ) res;
    }
    else if ( lua_type ( L, index ) == LUA_TBOOLEAN )
    {
        bool* res = new bool;
        *res = lua_toboolean ( L, index );
        result = ( void* ) res;
    }
    else if ( lua_type ( L, index ) == LUA_TSTRING )
    {
        char* res = new char[1024];
        sprintf ( res, "%s", lua_tostring ( L, index ) );
        result = ( void* ) res;
    }
    else
    {
        cerr << "Cannot trivialy convert the requested result to C-type variable" << endl;
        return nullptr;
    }

    return result;
}

int LuaListDirContent ( lua_State* L )
{
    DIR* dir;
    dirent* dentry;
    int i;

    if ( lua_type ( L, 1 ) != LUA_TSTRING )
    {
        lua_pushnil ( L );
        lua_pushstring ( L, "Invalid argument" );
        return 2;
    }

    const char* path = lua_tostring ( L, 1 );

    dir = opendir ( path );

    if ( dir == nullptr )
    {
        lua_pushnil ( L );
        lua_pushstring ( L, "no such directory" );
        return 2;
    }

    lua_newtable ( L );
    i = 1;

    while ( ( dentry = readdir ( dir ) ) != nullptr )
    {
        string dtype;

        struct stat fstat;

        stat ( dentry->d_name, &fstat );

        switch ( fstat.st_mode & S_IFMT )
        {
        case S_IFDIR:
            dtype = "dir";
            break;
        case S_IFREG:
            dtype = "file";
            break;
        default:
            dtype = "undetermined";
            break;
        }

        if ( dtype != "undetermined" )
        {
            lua_pushnumber ( L, i );

            lua_newtable ( L );

            lua_pushstring ( L, dentry->d_name );
            lua_setfield ( L, -2, "name" );

            lua_pushstring ( L, dtype.c_str() );
            lua_setfield ( L, -2, "type" );

            lua_settable ( L, -3 );

            // 	    cout << "pushed: [" << i << "] = { type = " << dtype.c_str() << " , name = " << dentry->d_name << " }" << endl;

            i++;
        }
    }

    closedir ( dir );

    return 1;
}
