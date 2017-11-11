#ifndef __LUASHMEM__
#define __LUASHMEM__

#include "LuaSystemCalls.h"

int LuaRegisterShMemConsts(lua_State* L);

static const luaL_Reg luaShMem_lib[] =
	{

		{ NULL, NULL } };

#endif
