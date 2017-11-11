#ifndef __LUASEMAPHORESBINDER__
#define __LUASEMAPHORESBINDER__

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>

#include "LuaSystemCalls.h"

union semun {
		int val; /* Value for SETVAL */
		struct semid_ds *buf; /* Buffer for IPC_STAT, IPC_SET */
		unsigned short *array; /* Array for GETALL, SETALL */
		struct seminfo *__buf; /* Buffer for IPC_INFO
		 (Linux-specific) */
};

struct SemInfos {
		int key;
		int nsems;

		SemInfos(int key_, int nsems_)
		{
			key = key_;
			nsems = nsems_;
		}

		SemInfos()
		{
			key = 0;
			nsems = 0;
		}
};

extern map<int, SemInfos> semaphoreList;

int LuaRegisterSemaphoresConsts(lua_State* L);

int LuaSemGet(lua_State* L);
int LuaSemCtl(lua_State* L);
int LuaSemOp(lua_State* L);

static const luaL_Reg luaSem_lib[] =
	{
		{ "SemGet", LuaSemGet },
		{ "SemCtl", LuaSemCtl },
		{ "SemOp", LuaSemOp },

		{ NULL, NULL } };

#endif
