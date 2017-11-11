#ifndef __LUAMMFILE__
#define __LUAMMFILE__

#include <sys/mman.h>

#include "LuaSystemCalls.h"

struct MMapInfo {
		char* address;
		size_t size;
		int fd;

		MMapInfo(char* address_, size_t size_, int fd_)
		{
			address = address_;
			size = size_;
			fd = fd_;
		}

		MMapInfo()
		{
			address = nullptr;
			size = 0;
			fd = -1;
		}
};


extern map<int, MMapInfo> mmapList;

int LuaRegisterMMFileConsts(lua_State* L);

int LuaNewMMap(lua_State* L);

int LuaAssignMMap(lua_State* L);

static const luaL_Reg luaMMap_lib[] =
	{
		{ "NewMMap", LuaNewMMap },
		{ "AssignMMap", LuaAssignMMap },

		{ NULL, NULL } };

#endif
