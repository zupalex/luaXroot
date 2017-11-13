#ifndef __LUAMSGQ__
#define __LUAMSGQ__

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "LuaSystemCalls.h"

struct MsgqInfos {
	int key;
	int id;
	char* address;

	MsgqInfos(int key_, int id_, char* address_)
	{
		key = key_;
		id = id_;
		address = address_;
	}

	MsgqInfos()
	{
		key = -1;
		id = -1;
		address = nullptr;
	}
};

struct generic_msgbuf {
	long mtype;
	char buffer[2048];
};

extern map<int, MsgqInfos> msgqList;

int LuaRegisterMsgqConsts(lua_State* L);

int LuaMsgGet(lua_State* L);
int LuaMsgSnd(lua_State* L);
int LuaMsgRcv(lua_State* L);
int LuaMsgCtl(lua_State* L);

static const luaL_Reg luaMsgq_lib[] =
	{
		{ "MsgGet", LuaMsgGet },
		{ "MsgSnd", LuaMsgSnd },
		{ "MsgRcv", LuaMsgRcv },
		{ "MsgCtl", LuaMsgCtl },

		{ NULL, NULL } };

#endif
