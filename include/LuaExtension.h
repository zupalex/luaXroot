#ifndef __LUAEXT__
#define __LUAEXT__

#include <iostream>
#include <stdlib.h>
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
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <list>
#include <functional>
#include <cassert>
#include <algorithm>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include <readline/history.h>

using namespace std;

extern lua_State* lua;

inline int saveprompthistory ( lua_State* L )
{
    const char* homeDir = getenv ( "HOME" );
    char* histDir = new char[1024];
    sprintf ( histDir, "%s/.luaXrootHist", homeDir );
    write_history ( histDir );
    return 0;
}

inline int wipeprompthistory ( lua_State* L )
{
    const char* homeDir = getenv ( "HOME" );
    char* histDir = new char[1024];
    sprintf ( histDir, "%s/.luaXrootHist", homeDir );
    history_truncate_file ( histDir, 0 );
    clear_history();
    return 0;
}

inline string GetLuaTypename ( int type_ )
{
    if ( type_ == LUA_TNUMBER ) return "number";
    else if ( type_ == LUA_TFUNCTION ) return "function";
    else if ( type_ == LUA_TBOOLEAN ) return "boolean";
    else if ( type_ == LUA_TSTRING ) return "string";
    else if ( type_ == LUA_TUSERDATA ) return "userdata";
    else if ( type_ == LUA_TLIGHTUSERDATA ) return "light userdata";
    else if ( type_ == LUA_TTHREAD ) return "thread";
    else if ( type_ == LUA_TTABLE ) return "table";
    else return "unknown";
}

inline bool CheckLuaArgs ( lua_State* L, int argIdx, bool abortIfError, string funcName )
{
    ( void ) L;
    ( void ) argIdx;
    ( void ) abortIfError;
    ( void ) funcName;
    return true;
}

inline bool CheckLuaArgs ( lua_State* L, int argIdx, bool abortIfError, string funcName, int arg )
{
    if ( lua_type ( L, argIdx ) != arg )
    {
        if ( abortIfError )
        {
            if ( argIdx > 0 )
            {
                cerr << "ERROR in " << funcName << " : argument #" << argIdx << " => " << GetLuaTypename ( arg ) << " expected, got " << lua_typename ( L, lua_type ( L, argIdx ) ) << endl;
            }
            else
            {
                cerr << "ERROR in " << funcName << " => " << GetLuaTypename ( arg ) << " expected, got " << lua_typename ( L, lua_type ( L, argIdx ) ) << endl;
            }
            lua_settop ( L, 0 );
        }
        return false;
    }
    return true;
}

template<typename... Rest> bool CheckLuaArgs ( lua_State* L, int argIdx, bool abortIfError, string funcName, int arg1, Rest... argRest )
{
    if ( lua_type ( L, argIdx ) != arg1 )
    {
        if ( abortIfError )
        {
            if ( argIdx > 0 )
            {
                cerr << "ERROR in " << funcName << " : argument #" << argIdx << " => " << GetLuaTypename ( arg1 ) << " expected, got " << lua_typename ( L, lua_type ( L, argIdx ) ) << endl;
            }
            else
            {
                cerr << "ERROR in " << funcName << " => " << GetLuaTypename ( arg1 ) << " expected, got " << lua_typename ( L, lua_type ( L, argIdx ) ) << endl;
            }
            lua_settop ( L, 0 );
        }
        return false;
    }
    else return CheckLuaArgs ( L, argIdx+1, abortIfError, funcName, argRest... );
}

inline bool lua_checkfield ( lua_State* L, int idx, string field, int type )
{
    lua_getfield ( L, idx, field.c_str() );

    if ( CheckLuaArgs ( L, -1, false, "", type ) )
    {
        return true;
    }
    else
    {
        lua_pop ( L, 1 );
        return false;
    }

}

lua_State* InitLuaEnv ( );

void TryGetGlobalField ( lua_State* L, string gfield );
bool TrySetGlobalField ( lua_State* L, string gfield );

void DoForEach ( lua_State* L, int index, function<bool ( lua_State* L_ ) > dofn );

template<typename T> T* GetLuaField ( lua_State* L, int index = -1, string field = "" )
{
    void* ret_hack;

    if ( field.empty() ) lua_pushvalue ( L, index );
    else lua_getfield ( L, index, field.c_str() );

    if ( lua_type ( L,-1 ) != LUA_TNIL )
    {
        if ( is_same<T,int>::value && lua_type ( L, -1 ) == LUA_TNUMBER )
        {
            int* ret_int = new int ( lua_tointeger ( L, -1 ) );
            ret_hack = ( void* ) ret_int;
        }
        else if ( ( is_same<T,float>::value || is_same<T,double>::value ) && lua_type ( L, -1 ) == LUA_TNUMBER )
        {
            float* ret_float = new float ( lua_tonumber ( L, -1 ) );
            ret_hack = ( void* ) ret_float;
        }
        else if ( is_same<T,string>::value && lua_type ( L, -1 ) == LUA_TSTRING )
        {
            string* ret_str = new string ( lua_tostring ( L, -1 ) );
            ret_hack = ( void* ) ret_str;
        }
        else if ( is_same<T,bool>::value && lua_type ( L, -1 ) == LUA_TBOOLEAN )
        {
            bool* ret_bool = new bool ( lua_toboolean ( L, -1 ) );
            ret_hack = ( void* ) ret_bool;
        }

        lua_pop ( L, 1 );
    }

    return ( ( T* ) ( ret_hack ) );
}

template<typename T> bool TryFindLuaTableValue ( lua_State* L, int index, T value_ )
{
    if ( index < 0 ) index = lua_gettop ( L ) + index + 1;

    if ( lua_type ( L, index ) != LUA_TTABLE )
    {
        cerr << "Called TryFindLuaTableValue on a " << lua_typename ( L, lua_type ( L, index ) ) << endl;
        return false;
    }

    int prevStackSize = lua_gettop ( L );

    DoForEach ( L, index,
                [=] ( lua_State* L_ )
    {
        T* val = GetLuaField<T> ( L_, -1 );

        if ( val != nullptr && *val == value_ )
        {
            lua_pushvalue ( L_, -2 );
            lua_pushvalue ( L_, -2 );
            return true;
        }
        else return false;
    } );

    int newStackSize = lua_gettop ( L );

    if ( newStackSize != prevStackSize && newStackSize != prevStackSize+2 )
    {
        cerr << "Something went wrong while looking for a value in the lua table! Stack balance is " << newStackSize-prevStackSize << endl;
    }

    return newStackSize == prevStackSize+2;
}

inline void PushToLuaStack ( lua_State* L )
{
    ( void ) L;
    return;
}

inline void PushToLuaStack ( lua_State* L, char val )
{
    char* c = new char;
    *c = val;
    lua_pushstring ( L, c );
}

inline void PushToLuaStack ( lua_State* L, const char* val )
{
    lua_pushstring ( L, val );
}

inline void PushToLuaStack ( lua_State* L, char* val )
{
    lua_pushstring ( L, val );
}

inline void PushToLuaStack ( lua_State* L, int val )
{
    lua_pushinteger ( L, val );
}

inline void PushToLuaStack ( lua_State* L, int* val )
{
    lua_pushinteger ( L, *val );
}

inline void PushToLuaStack ( lua_State* L, unsigned int val )
{
    lua_pushnumber ( L, val );
}

inline void PushToLuaStack ( lua_State* L, unsigned int* val )
{
    lua_pushnumber ( L, *val );
}

inline void PushToLuaStack ( lua_State* L, long int val )
{
    lua_pushnumber ( L, val );
}

inline void PushToLuaStack ( lua_State* L, long int* val )
{
    lua_pushnumber ( L, *val );
}

inline void PushToLuaStack ( lua_State* L, unsigned long int val )
{
    lua_pushnumber ( L, val );
}

inline void PushToLuaStack ( lua_State* L, unsigned long int* val )
{
    lua_pushnumber ( L, *val );
}

inline void PushToLuaStack ( lua_State* L, long long int val )
{
    lua_pushnumber ( L, val );
}

inline void PushToLuaStack ( lua_State* L, long long int* val )
{
    lua_pushnumber ( L, *val );
}

inline void PushToLuaStack ( lua_State* L, unsigned long long int val )
{
    lua_pushnumber ( L, val );
}

inline void PushToLuaStack ( lua_State* L, unsigned long long int* val )
{
    lua_pushnumber ( L, *val );
}

inline void PushToLuaStack ( lua_State* L, float val )
{
    lua_pushnumber ( L, val );
}

inline void PushToLuaStack ( lua_State* L, float* val )
{
    lua_pushnumber ( L, *val );
}

inline void PushToLuaStack ( lua_State* L, double val )
{
    lua_pushnumber ( L, val );
}

inline void PushToLuaStack ( lua_State* L, double* val )
{
    lua_pushnumber ( L, *val );
}

inline void PushToLuaStack ( lua_State* L, bool val )
{
    lua_pushboolean ( L, val );
}

inline void PushToLuaStack ( lua_State* L, bool* val )
{
    lua_pushboolean ( L, *val );
}

inline void PushToLuaStack ( lua_State* L, string val )
{
    lua_pushstring ( L, val.c_str() );
}

inline void PushToLuaStack ( lua_State* L, string* val )
{
    lua_pushstring ( L, ( *val ).c_str() );
}

inline void PushToLuaStack ( lua_State* L, nullptr_t val )
{
    ( void ) val;
    lua_pushnil ( L );
}

template<typename T> void PushToLuaStack ( lua_State* L, T val )
{
    void* v_val;
    v_val = ( void* ) ( &val );

    T* udata = reinterpret_cast<T*> ( lua_newuserdata ( L, sizeof ( T ) ) );
    *udata = val;
}

template<typename First, typename... Rest> void PushToLuaStack ( lua_State* L, First fst, Rest... rest )
{
    PushToLuaStack ( L, fst );
    PushToLuaStack ( L, rest... );
}

int LuaListDirContent ( lua_State* L );

// ************************************************************************************************ //
// ***************************************** Sockets Binder *************************************** //
// ************************************************************************************************ //

int LuaRegisterSocketConsts(lua_State* L);

struct SocketInfos
{
    int domain;
    int type;

    SocketInfos(int dom_, int typ_)
    {
        domain = dom_;
        type = typ_;
    }

    SocketInfos()
    {
        domain = AF_UNIX;
        type = SOCK_STREAM;
    }
};

extern map<int, SocketInfos> socketsList;
extern int maxSockFd;

int LuaSysClose(lua_State* L);

int LuaSysUnlink(lua_State* L);
int LuaSysRead(lua_State* L);
int LuaSysWrite(lua_State* L);

int LuaNewSocket(lua_State* L);

int LuaSocketBind(lua_State* L);
int LuaSocketConnect(lua_State* L);

int LuaSocketSelect(lua_State* L);

int LuaSocketListen(lua_State* L);
int LuaSocketAccept(lua_State* L);

int LuaSocketSend(lua_State* L);
int LuaSocketReceive(lua_State* L);

#endif

