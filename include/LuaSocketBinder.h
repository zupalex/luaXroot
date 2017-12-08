#ifndef __LUASOCKET__
#define __LUASOCKET__

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

#include "LuaSystemCalls.h"

// ************************************************************************************************ //
// ***************************************** Sockets Binder *************************************** //
// ************************************************************************************************ //

int LuaRegisterSocketConsts(lua_State* L);

struct SocketInfos {
		int domain;
		int type;
		string address;
		const char* port;

		SocketInfos(int dom_, int typ_, string address_ = "", const char* port_ = "")
		{
			domain = dom_;
			type = typ_;
			address = address_;
			port = port_;
		}

		SocketInfos()
		{
			domain = AF_UNIX;
			type = SOCK_STREAM;
			address = "/tmp/sockets";
			port = "8080";
		}
};

extern map<int, SocketInfos> socketsList;

int LuaNewSocket(lua_State* L);

int LuaSocketBind(lua_State* L);
int LuaSocketConnect(lua_State* L);

int LuaSocketListen(lua_State* L);
int LuaSocketAccept(lua_State* L);

int LuaSocketSend(lua_State* L);
int LuaSocketReceive(lua_State* L);

static const luaL_Reg luaSocket_lib[] =
	{
		{ "NewSocket", LuaNewSocket },

		{ "SocketBind", LuaSocketBind },
		{ "SocketConnect", LuaSocketConnect },

		{ "SocketListen", LuaSocketListen },
		{ "SocketAccept", LuaSocketAccept },

		{ "SocketReceive", LuaSocketReceive },
		{ "SocketSend", LuaSocketSend },

		{ NULL, NULL } };

#endif
