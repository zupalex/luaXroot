#ifndef __LUASHMEM__
#define __LUASHMEM__

#include <sys/ipc.h>
#include <sys/shm.h>

#include "LuaSystemCalls.h"

struct ShMemInfos {
	int key;
	int id;
	int size;
	char* address;

	ShMemInfos(int key_, int id_, int size_, char* address_)
	{
		key = key_;
		id = id_;
		size = size_;
		address = address_;
	}

	ShMemInfos()
	{
		key = -1;
		id = -1;
		size = 0;
		address = nullptr;
	}
};

extern map<int, ShMemInfos> shmemList;

int LuaRegisterShMemConsts(lua_State* L);

int LuaShmGet(lua_State* L);
int LuaShmAt(lua_State* L);
int LuaShmCtl(lua_State* L);
int LuaShmDt(lua_State* L);

int LuaAssignShmem(lua_State* L);

static const luaL_Reg luaShMem_lib[] =
	{
		{ "ShmGet", LuaShmGet },
		{ "ShmAt", LuaShmAt },
		{ "ShmCtl", LuaShmCtl },
		{ "ShmDt", LuaShmDt },

		{ "AssignShmem", LuaAssignShmem },

		{ NULL, NULL } };

#endif
