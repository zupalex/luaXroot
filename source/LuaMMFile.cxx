#include "LuaMMFile.h"

int LuaRegisterMMFileConsts(lua_State* L)
{
	lua_pushinteger(L, PROT_EXEC);
	lua_setglobal(L, "PROT_EXEC");

	lua_pushinteger(L, PROT_READ);
	lua_setglobal(L, "PROT_READ");

	lua_pushinteger(L, PROT_WRITE);
	lua_setglobal(L, "PROT_WRITE");

	lua_pushinteger(L, PROT_NONE);
	lua_setglobal(L, "PROT_NONE");

	lua_pushinteger(L, MAP_SHARED);
	lua_setglobal(L, "MAP_SHARED");

	lua_pushinteger(L, MAP_PRIVATE);
	lua_setglobal(L, "MAP_PRIVATE");

	lua_pushinteger(L, MAP_32BIT);
	lua_setglobal(L, "MAP_32BIT");

	lua_pushinteger(L, MAP_ANONYMOUS);
	lua_setglobal(L, "MAP_ANONYMOUS");

	lua_pushinteger(L, MAP_FIXED);
	lua_setglobal(L, "MAP_FIXED");

	lua_pushinteger(L, MAP_GROWSDOWN);
	lua_setglobal(L, "MAP_GROWSDOWN");

	lua_pushinteger(L, MAP_HUGETLB);
	lua_setglobal(L, "MAP_HUGETLB");

	lua_pushinteger(L, MAP_HUGE_SHIFT);
	lua_setglobal(L, "MAP_HUGE_SHIFT");

	lua_pushinteger(L, MAP_LOCKED);
	lua_setglobal(L, "MAP_LOCKED");

	lua_pushinteger(L, MAP_NONBLOCK);
	lua_setglobal(L, "MAP_NONBLOCK");

	lua_pushinteger(L, MAP_NORESERVE);
	lua_setglobal(L, "MAP_NORESERVE");

	lua_pushinteger(L, MAP_POPULATE);
	lua_setglobal(L, "MAP_POPULATE");

	lua_pushinteger(L, MAP_STACK);
	lua_setglobal(L, "MAP_STACK");

	return 0;
}

map<int, void*> mmapList;

int LuaNewMMap(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaNewMMap argument table",
		{ "mapid", "flags", "prot", "size", "offset" },
		{ LUA_TNUMBER, LUA_TSTRING, LUA_TSTRING, LUA_TNUMBER, LUA_TNUMBER },
		{ true, false, false, true, false });

	int mapid, size, offset;

	mapid = lua_tointeger(L, -5);
	size = lua_tonumber(L, -2);
	offset = lua_tonumberx(L, -1, nullptr);

	string flags_str, prot_str;
	flags_str = lua_tostringx(L, -4);
	prot_str = lua_tostringx(L, -3);

	if (flags_str.empty()) flags_str = "MAP_SHARED";
	if (prot_str.empty()) prot_str = "PROT_READ";

	int flags = GetFlagsFromOctalString(L, flags_str);
	int prot = GetFlagsFromOctalString(L, prot_str);

	void* mmap_addr = mmap((caddr_t) 0, size, prot, flags, mapid, offset);

	if (mmap_addr == MAP_FAILED)
	{
		cerr << "Error while retrieving address for memory mapped file: " << errno << endl;
		return 0;
	}

	mmapList[mapid] = mmap_addr;

	return 0;
}

int LuaAssignMMap(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaAssignMMap argument table",
		{ "mapid", "buffer" },
		{ LUA_TNUMBER, LUA_TUSERDATA },
		{ true, true });

	lua_remove(L, 1);

	int mapid = lua_tointeger(L, -2);
	lua_remove(L, -2);

	lua_getfield(L, -1, "type");
	string type = lua_tostring(L, -1);
	lua_pop(L, 1);

	assignUserDataFns[type](L, mmapList[mapid]);

	return 0;
}

