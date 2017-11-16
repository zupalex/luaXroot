#include <LuaMMap.h>

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

map<int, MMapInfo> mmapList;

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

	char* mmap_addr = (char*) mmap((caddr_t) 0, size, prot, flags, mapid, offset);

	if (mmap_addr == MAP_FAILED)
	{
		cerr << "Error while retrieving address for memory mapped file: " << errno << endl;
		return 0;
	}

	mmapList[mapid] = MMapInfo(mmap_addr, size, mapid);

	return 0;
}

int LuaAssignMMap(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaAssignMMap argument table",
		{ "mapid", "buffer", "offset" },
		{ LUA_TNUMBER, LUA_TUSERDATA, LUA_TNUMBER },
		{ true, true, false });

	lua_remove(L, 1);

	int mapid = lua_tointeger(L, -3);
	lua_remove(L, -3);

	int offset = lua_tointegerx(L, -1, nullptr);
	if(offset < 0) offset = 0;

	lua_getfield(L, -2, "sizeof");
	int buffer_size = lua_tointeger(L, -1);

	if(mmapList[mapid].address+offset+buffer_size > mmapList[mapid].address+mmapList[mapid].size)
	{
		cerr << "WARNING: assigning memory segment partially or totally outside of the mmap allocated space. Forced it back to last available block" << endl;
		offset = mmapList[mapid].size-buffer_size;
	}

	lua_getfield(L, -3, "type");
	string type = lua_tostring(L, -1);
	lua_pop(L, 2);

	assignUserDataFns[type](L, mmapList[mapid].address+offset);

	lua_pushinteger(L, offset);
	return 1;
}

int LuaMMapRawRead(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaMMapRawRead argument table",
		{ "mapid", "size", "offset" },
		{ LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER },
		{ true, true, false });

	int mapid = lua_tointeger(L, -3);
	int size = lua_tointeger(L, -2);
	int offset = lua_tointegerx(L, -1, nullptr);

	char* rbuf = new char[size];
	rbuf = mmapList[mapid].address + offset;

	lua_pushlstring(L, rbuf, size);

	return 1;
}
