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

	if (nsems == 0)
	{
		semid_ds semstat;
		semun sem_un;
		sem_un.buf = &semstat;
		int success = semctl(semid, 0, IPC_STAT, sem_un);

		if (success == -1)
		{
			cerr << "Error while retrieving semaphore info: " << errno << endl;
			return 0;
		}

		nsems = semstat.sem_nsems;
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
		ret = semctl(semid, semnum, SETVAL, sem_un);
	}
	else if (cmd == SETALL)
	{
		lua_getfield(L, 1, "val");
		sem_un.array = new unsigned short[semaphoreList[semid].nsems];
		lua_autosetarray(L, sem_un.array, semaphoreList[semid].nsems);
		ret = semctl(semid, semnum, SETALL, sem_un);
	}
	else if (cmd == GETVAL)
	{
		ret = semctl(semid, semnum, GETVAL);
		lua_pushinteger(L, ret);
		nret = 1;
	}
	else if (cmd == GETALL)
	{
		sem_un.array = new unsigned short[semaphoreList[semid].nsems];

		ret = semctl(semid, semnum, GETALL, sem_un);

		lua_autogetarray(L, sem_un.array, semaphoreList[semid].nsems);
		nret = 1;
	}
	else if (cmd == IPC_STAT)
	{
		semid_ds semstat;
		sem_un.buf = &semstat;
		int success = semctl(semid, semnum, IPC_STAT, sem_un);

		if (success == -1)
		{
			cerr << "Error while retrieving semaphore info: " << errno << endl;
			return 0;
		}

		lua_newtable(L);
		lua_pushinteger(L, semstat.sem_nsems);
		lua_pushinteger(L, semstat.sem_ctime);
		lua_pushinteger(L, semstat.sem_otime);
		lua_setfield(L, -4, "semop_time");
		lua_setfield(L, -3, "lastchange_time");
		lua_setfield(L, -2, "nsem");
		return 1;
	}
	else if (cmd == GETNCNT)
	{
		int ncnt = semctl(semid, semnum, GETNCNT);
		lua_pushinteger(L, ncnt);
		return 1;
	}
	else if (cmd == GETZCNT)
	{
		int zcnt = semctl(semid, semnum, GETZCNT);
		lua_pushinteger(L, zcnt);
		return 1;
	}
	else
	{
		semctl(semid, semnum, cmd);
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

	unsigned short* semnum = new unsigned short[nsops];
	short* sop = new short[nsops];

	lua_autosetarray(L, sop, nsops);
	lua_autosetarray(L, semnum, nsops);

	for (int i = 0; i < nsops; i++)
	{
		sb[i].sem_num = semnum[i];
		sb[i].sem_op = sop[i];
	}

	int ret = semop(semid, sb, nsops);

	if (ret == -1)
	{
		cerr << "Error while performing semop: " << errno << endl;
		lua_pushinteger(L, errno);
		return 1;
	}

	return 0;
}
