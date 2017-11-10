#include "LuaSemaphoresBinder.h"

map<int, SemInfos> semaphoreList;

int LuaRegisterSemaphoresConsts(lua_State* L)
{
	lua_pushinteger(L, IPC_CREAT);
	lua_setglobal(L, "IPC_CREAT");

	lua_pushinteger(L, IPC_EXCL);
	lua_setglobal(L, "IPC_EXCL");

	lua_pushinteger(L, IPC_PRIVATE);
	lua_setglobal(L, "IPC_PRIVATE");

	lua_pushinteger(L, IPC_STAT);
	lua_setglobal(L, "IPC_STAT");

	lua_pushinteger(L, IPC_SET);
	lua_setglobal(L, "IPC_SET");

	lua_pushinteger(L, IPC_RMID);
	lua_setglobal(L, "IPC_RMID");

	lua_pushinteger(L, IPC_INFO);
	lua_setglobal(L, "IPC_INFO");

	lua_pushinteger(L, SEM_INFO);
	lua_setglobal(L, "SEM_INFO ");

	lua_pushinteger(L, SEM_STAT);
	lua_setglobal(L, "SEM_STAT");

	lua_pushinteger(L, GETALL);
	lua_setglobal(L, "GETALL");

	lua_pushinteger(L, GETNCNT);
	lua_setglobal(L, "GETNCNT");

	lua_pushinteger(L, GETPID);
	lua_setglobal(L, "GETPID");

	lua_pushinteger(L, GETVAL);
	lua_setglobal(L, "GETVAL");

	lua_pushinteger(L, GETZCNT);
	lua_setglobal(L, "GETZCNT");

	lua_pushinteger(L, SETALL);
	lua_setglobal(L, "SETALL");

	lua_pushinteger(L, SETVAL);
	lua_setglobal(L, "SETVAL");

	lua_pushinteger(L, IPC_NOWAIT);
	lua_setglobal(L, "IPC_NOWAIT");

	lua_pushinteger(L, SEM_UNDO);
	lua_setglobal(L, "SEM_UNDO");

	return 0;
}

int LuaSemFtok(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaSemFtok argument table",
		{ "pathname", "id" },
		{ LUA_TSTRING, LUA_TNUMBER },
		{ true, false });

	const char* pathname = lua_tostring(L, -2);
	int proj_id = lua_tointegerx(L, -1, nullptr);

	key_t semkey = ftok(pathname, proj_id > 0 ? proj_id : 'Z');

	lua_pushinteger(L, semkey);

	return 1;
}

int LuaSemGet(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaSemGet argument table",
		{ "key", "nsem", "flag" },
		{ LUA_TNUMBER, LUA_TNUMBER, LUA_TSTRING },
		{ true, false, false });

	int semkey = lua_tointeger(L, -3);
	int nsems = lua_tointegerx(L, -2, nullptr);
	string semflag_str = lua_tostringx(L, -1);

	if (semflag_str.empty()) semflag_str = "IPC_CREAT | IPC_EXCL | 0666";

	int semflag = GetFlagsFromOctalString(L, semflag_str);

	int semid = semget(semkey, nsems, semflag);

	if (semid == -1)
	{
		cerr << "Error while getting semaphore. Check that the flag required is appropriate (by default flag = \"IPC_CREAT | IPC_EXCL | 0666\")" << endl;
		return 0;
	}

	semaphoreList[semid] = SemInfos(semkey, nsems);

	lua_pushinteger(L, semid);

	return 1;
}

int LuaSemCtl(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaSemCtl argument table",
		{ "semid", "semnum", "cmd" },
		{ LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER },
		{ true, true, true });

	int semid = lua_tointeger(L, -3);
	int semnum = lua_tointeger(L, -2);
	int cmd = lua_tointeger(L, -1);

	semun sem_un;

	int ret = 0;

	bool nret = 0;

	if (cmd == SETVAL)
	{
		lua_getfield(L, 1, "val");
		sem_un.val = lua_tointegerx(L, -1, nullptr);
		ret = semctl(semid, semnum, cmd, sem_un);
	}
	else if (cmd == SETALL)
	{
		lua_getfield(L, 1, "val");
		vector<unsigned short> vals;
		lua_autosetvector(L, &vals);
		sem_un.array = &vals[0];
		ret = semctl(semid, semnum, cmd, sem_un);
	}
	else if (cmd == GETVAL)
	{
		ret = semctl(semid, semnum, cmd);
		lua_pushinteger(L, ret);
		nret = 1;
	}
	else if (cmd == GETALL)
	{
		vector<int> vals;

		ret = semctl(semid, semnum, cmd, sem_un);

		for (int i = 0; i < semaphoreList[semid].nsems; i++)
			vals[i] = sem_un.array[i];

		lua_autogetvector(L, vals);
		nret = 1;
	}
	else
	{
		lua_pushnil(L);
	}

	if (ret == -1)
	{
		cerr << "Error while performing semctl: " << errno << endl;
		lua_pushinteger(L, errno);
		return 2;
	}

	return nret;
}

int LuaSemOp(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaSemCtl argument table",
		{ "semid", "semnum", "sop" },
		{ LUA_TNUMBER, LUA_TTABLE, LUA_TTABLE },
		{ true, true, true });

	int semid = lua_tointeger(L, -3);

	int nsops = lua_rawlen(L, -2);

	sembuf* sb = new sembuf[nsops];
//	sembuf sb;

	unsigned short* semnum = new unsigned short[nsops];
	short* sop = new short[nsops];

	lua_autosetarray(L, sop, nsops);
	lua_autosetarray(L, semnum, nsops);

	for (int i = 0; i < nsops; i++)
	{
		sb[i].sem_num = semnum[i];
		sb[i].sem_op = sop[i];
	}

//	sb.sem_num = semnum[0];
//	sb.sem_op = sop[0];

	int ret = semop(semid, sb, nsops);
//	int ret = semop(semid, &sb, 1);

	if (ret == -1)
	{
		cerr << "Error while performing semop: " << errno << endl;
		lua_pushinteger(L, errno);
		return 1;
	}

	return 0;
}
