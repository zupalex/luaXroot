#ifndef __LUASYSTEMCALLS__
#define __LUASYSTEMCALLS__

#include <sys/types.h>
#include <sys/ipc.h>
#include <fcntl.h>

#include "LuaExtension.h"

extern int maxFd;

int LuaRegisterSysOpenConsts(lua_State* L);

int GetFlagsFromOctalString(lua_State* L, string str);

int LuaGetEnv(lua_State* L);
int LuaSysFork(lua_State* L);
int LuaSysExecvpe(lua_State* L);

int LuaSysOpen(lua_State* L);
int LuaSysFtruncate(lua_State* L);
int LuaSysClose(lua_State* L);
int MakePipe(lua_State* L);
int MakeFiFo(lua_State* L);
int LuaSysDup(lua_State* L);
int LuaSysDup2(lua_State* L);

int LuaSysUnlink(lua_State* L);
int LuaSysRead(lua_State* L);
int LuaSysWrite(lua_State* L);

int LuaSysSelect(lua_State* L);

int LuaSysFtok(lua_State* L);

static const luaL_Reg luaSysCall_lib[] =
	{
		{ "SysOpen", LuaSysOpen },
		{ "SysFtruncate", LuaSysFtruncate },
		{ "SysClose", LuaSysClose },
		{ "SysUnlink", LuaSysUnlink },
		{ "SysRead", LuaSysRead },
		{ "SysWrite", LuaSysWrite },
		{ "SysDup", LuaSysDup },
		{ "SysDup2", LuaSysDup2 },
		{ "SysFork", LuaSysFork },
		{ "SysExec", LuaSysExecvpe },
		{ "GetEnv", LuaGetEnv },

		{ "MakePipe", MakePipe },
		{ "MakeFiFo", MakeFiFo },

		{ "SysSelect", LuaSysSelect },

		{ "SysFtok", LuaSysFtok },

		{ NULL, NULL } };

#endif
