#ifndef __LUASOCKET__
#define __LUASOCKET__

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>

#include "LuaExtension.h"

// ************************************************************************************************ //
// ***************************************** Sockets Binder *************************************** //
// ************************************************************************************************ //

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
extern int maxSockFd;

int LuaSysUnlink ( lua_State* L );
int LuaSysRead ( lua_State* L );
int LuaSysWrite ( lua_State* L );

int LuaNewSocket ( lua_State* L );

int LuaSocketBind ( lua_State* L );
int LuaSocketConnect ( lua_State* L );

int LuaSocketSelect ( lua_State* L );

int LuaSocketListen ( lua_State* L );
int LuaSocketAccept ( lua_State* L );

int LuaSocketSend ( lua_State* L );
int LuaSocketReceive ( lua_State* L );

#endif