#ifndef __LUAMMFILE__
#define __LUAMMFILE__

#include <sys/mman.h>

#include "LuaSystemCalls.h"

extern map<int, void*> mmapList;

int LuaRegisterMMFileConsts(lua_State* L);

int LuaNewMMap(lua_State* L);

int LuaAssignMMap(lua_State* L);

static const luaL_Reg luaMMap_lib[] =
	{
		{ "NewMMap", LuaNewMMap },
		{ "AssignMMap", LuaAssignMMap },

		{ NULL, NULL } };

#endif
