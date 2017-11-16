#include "LuaShMem.h"

map<int, ShMemInfos> shmemList;

int LuaRegisterShMemConsts(lua_State* L)
{
	lua_pushinteger(L, SHM_HUGETLB);
	lua_setglobal(L, "SHM_HUGETLB");

	lua_pushinteger(L, SHM_NORESERVE);
	lua_setglobal(L, "SHM_NORESERVE");

	lua_pushinteger(L, SHM_EXEC);
	lua_setglobal(L, "SHM_EXEC");

	lua_pushinteger(L, SHM_RDONLY);
	lua_setglobal(L, "SHM_RDONLY");

	lua_pushinteger(L, SHM_REMAP);
	lua_setglobal(L, "SHM_REMAP");

	lua_pushinteger(L, SHM_RND);
	lua_setglobal(L, "SHM_RND");

	lua_pushinteger(L, SHMLBA);
	lua_setglobal(L, "SHMLBA");

	return 0;
}

int LuaShmGet(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaShmGet argument table",
		{ "key", "size", "flags" },
		{ LUA_TNUMBER, LUA_TNUMBER, LUA_TSTRING },
		{ true, true, false });

	int shmkey = lua_tointeger(L, -3);
	int shmsize = lua_tointeger(L, -2);
	string shmflag_str = lua_tostringx(L, -1);

	if (shmflag_str.empty()) shmflag_str = "IPC_CREAT | IPC_EXCL | 0666";

	int shmflag = GetFlagsFromOctalString(L, shmflag_str);

	int shmid = shmget(shmkey, shmsize, shmflag);

	if (shmid == -1)
	{
		cerr << "Error while getting shared memory segment. Check that the flag required is appropriate (by default flag = \"IPC_CREAT | IPC_EXCL | 0666\")" << endl;
		return 0;
	}

	shmemList[shmid] = ShMemInfos(shmkey, shmid, shmsize, nullptr);

	lua_pushinteger(L, shmid);

	return 1;
}

int LuaShmAt(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaShmAt argument table",
		{ "buffer", "shmid", "flags" },
		{ LUA_TUSERDATA, LUA_TNUMBER, LUA_TSTRING },
		{ false, true, false });

	lua_remove(L, 1);

	int shmid = lua_tointeger(L, -2);
	string shmflag_str = lua_tostringx(L, -1);

	int shmflag = shmflag_str.empty() ? 0 : GetFlagsFromOctalString(L, shmflag_str);

	char* shmaddr = (char*) shmat(shmid, (void*) 0, shmflag);

	if (shmaddr == (void*) -1)
	{
		cerr << "Error while retrieving address for shared memory segment: " << errno << endl;
		return 0;
	}

	shmemList[shmid].address = shmaddr;

	if (lua_type(L, -3) == LUA_TUSERDATA)
	{
		lua_pop(L, 2);

		lua_getfield(L, 1, "type");
		string type = lua_tostring(L, -1);
		lua_pop(L, 1);

		assignUserDataFns[type](L, shmemList[shmid].address);
	}

	return 0;
}

int LuaShmCtl(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaShmCtl argument table",
		{ "shmid", "cmd" },
		{ LUA_TNUMBER, LUA_TNUMBER },
		{ true, true });

	int shmid = lua_tointeger(L, -2);
	int cmd = lua_tointeger(L, -1);

	shmid_ds shmbuf;

	if (cmd == IPC_STAT)
	{
		shmctl(shmid, IPC_STAT, &shmbuf);

		lua_newtable(L);

		lua_pushinteger(L, shmbuf.shm_segsz);
		lua_setfield(L, -2, "size");

		lua_pushinteger(L, shmbuf.shm_atime);
		lua_setfield(L, -2, "attach_time");

		lua_pushinteger(L, shmbuf.shm_dtime);
		lua_setfield(L, -2, "detach_time");

		lua_pushinteger(L, shmbuf.shm_ctime);
		lua_setfield(L, -2, "change_time");

		lua_pushinteger(L, shmbuf.shm_cpid);
		lua_setfield(L, -2, "creator_pid");

		lua_pushinteger(L, shmbuf.shm_lpid);
		lua_setfield(L, -2, "last_pid");

		lua_pushinteger(L, shmbuf.shm_nattch);
		lua_setfield(L, -2, "nattach");

		return 1;
	}
	else if (cmd == IPC_RMID)
	{
		shmctl(shmid, IPC_RMID, &shmbuf);
	}

	return 0;
}

int LuaShmDt(lua_State* L)
{
	int shmid = lua_tointegerx(L, 1, nullptr);

	if (shmid > 0) shmdt(shmemList[shmid].address);

	return 0;
}

int LuaAssignShmem(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaAssignShmem argument table",
		{ "buffer", "shmid", "offset" },
		{ LUA_TUSERDATA, LUA_TNUMBER },
		{ true, true, false });
	lua_remove(L, 1);

	int shmid = lua_tointeger(L, -2);
	int offset = lua_tointegerx(L, -1, nullptr);
	lua_pop(L, 2);

	lua_getfield(L, 1, "type");
	string btype = lua_tostring(L, -1);
	lua_pop(L, 1);

	assignUserDataFns[btype](L, shmemList[shmid].address + offset);

	return 0;
}

int LuaShmSetMem(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaSgmSetMem argument table",
		{ "shmid", "input", "format" },
		{ LUA_TNUMBER, LUA_TTABLE, LUA_TTABLE },
		{ true, true, true });

	int shmid = lua_tointeger(L, -3);
	lua_remove(L, 1);
	lua_remove(L, 1);

	char* shmem_address = shmemList[shmid].address;

	SetMemoryBlock(L, shmem_address);

	return 0;
}

int LuaShmGetMem(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaSgmSetMem argument table",
		{ "shmid", "output" },
		{ LUA_TNUMBER, LUA_TTABLE },
		{ true, true });

	int shmid = lua_tointeger(L, -2);
	lua_remove(L, 1);
	lua_remove(L, 1);

	char* shmem_address = shmemList[shmid].address;

	GetMemoryBlock(L, shmem_address);

	return 1;
}

int LuaShmRawRead(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaShmRawRead argument table",
		{ "shmid", "size", "offset" },
		{ LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER },
		{ true, true, false });

	int shmid = lua_tointeger(L, -3);
	int size = lua_tointeger(L, -2);
	int offset = lua_tointegerx(L, -1, nullptr);

	char* rbuf = new char[size];
	rbuf = shmemList[shmid].address + offset;

	lua_pushlstring(L, rbuf, size);

	return 1;
}
