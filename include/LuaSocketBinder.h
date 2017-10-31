#ifndef __LUASOCKET__
#define __LUASOCKET__

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

#include "LuaExtension.h"

// ************************************************************************************************ //
// ***************************************** Sockets Binder *************************************** //
// ************************************************************************************************ //

int MakePipe ( lua_State* L );
int MakeFiFo ( lua_State* L );

int LuaSysOpen ( lua_State* L );
int LuaSysClose ( lua_State* L );

int LuaSysDup ( lua_State* L );
int LuaSysDup2 ( lua_State* L );

int LuaRegisterSysOpenConsts ( lua_State* L );
int LuaRegisterSocketConsts ( lua_State* L );

struct SocketInfos
{
    int domain;
    int type;

    SocketInfos ( int dom_, int typ_ )
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
extern int maxFd;



int LuaSysUnlink ( lua_State* L );
int LuaSysRead ( lua_State* L );
int LuaSysWrite ( lua_State* L );

int LuaNewSocket ( lua_State* L );

int LuaSocketBind ( lua_State* L );
int LuaSocketConnect ( lua_State* L );

int LuaSysSelect ( lua_State* L );

int LuaSocketListen ( lua_State* L );
int LuaSocketAccept ( lua_State* L );

int LuaSocketSend ( lua_State* L );
int LuaSocketReceive ( lua_State* L );

#endif
