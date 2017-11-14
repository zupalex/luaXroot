#include "LuaMsgq.h"

map<int, MsgqInfos> msgqList;

int LuaRegisterMsgqConsts(lua_State* L)
{

	return 0;
}

int LuaMsgGet(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaMsgGet argument table",
		{ "key", "flags" },
		{ LUA_TNUMBER, LUA_TSTRING },
		{ true, false });

	int msgkey = lua_tointeger(L, -2);
	string msgflag_str = lua_tostringx(L, -1);

	if (msgflag_str.empty()) msgflag_str = "IPC_CREAT | IPC_EXCL | 0666";

	int msgflag = GetFlagsFromOctalString(L, msgflag_str);

	int msgid = msgget(msgkey, msgflag);

	if (msgid == -1)
	{
		cerr << "Error while getting message queue. Check that the flag required is appropriate (by default flag = \"IPC_CREAT | IPC_EXCL | 0666\")" << endl;
		return 0;
	}

	msgqList[msgid] = MsgqInfos(msgkey, msgid, nullptr);

	lua_pushinteger(L, msgid);

	return 1;
}

int LuaMsgSnd(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaAssignMsgq argument table",
		{ "data", "msgqid", "mtype", "flags" },
		{ LUA_TTABLE, LUA_TNUMBER, LUA_TNUMBER, LUA_TSTRING },
		{ true, true, false, false });
	lua_remove(L, 1);

	int mtype = lua_tointegerx(L, -2, nullptr);
	if (mtype <= 0) mtype = 1;

	int msgid = lua_tointeger(L, -3);

	string msgflags_str = lua_tostringx(L, -1);
	lua_pop(L, 3);

	generic_msgbuf msgbuf;
	msgbuf.mtype = mtype;

	lua_getfield(L, 1, "values");
	lua_getfield(L, 1, "format");
	lua_remove(L, 1);

	int blocksize = SetMemoryBlock(L, msgbuf.buffer);

	int msgflags = msgflags_str.empty() ? 0 : GetFlagsFromOctalString(L, msgflags_str);

	int success = msgsnd(msgid, (void*) &msgbuf, blocksize, msgflags);

	if(success == -1)
	{
		cerr << "Failed to send the message..." << endl;
	}

	return 0;
}

int LuaMsgRcv(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaAssignMsgq argument table",
		{ "format", "msgqid", "mtype", "flags" },
		{ LUA_TTABLE, LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER, LUA_TSTRING },
		{ true, true, false, true, false });
	lua_remove(L, 1);

	int mtype = lua_tointegerx(L, -3, nullptr);

	int msgid = lua_tointeger(L, -4);

	string msgflags_str = lua_tostringx(L, -1);
	lua_pop(L, 4);

	generic_msgbuf msgbuf;

	int msgflags = msgflags_str.empty() ? 0 : GetFlagsFromOctalString(L, msgflags_str);

	int success = msgrcv(msgid, (void*) &msgbuf, 4096, mtype, msgflags);

	if(success == -1)
	{
		cerr << "Failed to receive the message..." << endl;
	}

	GetMemoryBlock(L, msgbuf.buffer);
	lua_pushinteger(L, success);

	return 2;
}

int LuaMsgCtl(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaShmCtl argument table",
		{ "msgqid", "cmd" },
		{ LUA_TNUMBER, LUA_TNUMBER },
		{ true, true });

	int msgid = lua_tointeger(L, -2);
	int cmd = lua_tointeger(L, -1);

	msqid_ds msgbuf;

	if (cmd == IPC_STAT)
	{
		msgctl(msgid, IPC_STAT, &msgbuf);

		lua_newtable(L);

		lua_pushinteger(L, msgbuf.msg_qbytes);
		lua_setfield(L, -2, "max_size");

		lua_pushinteger(L, msgbuf.msg_qnum);
		lua_setfield(L, -2, "queue_size");

		lua_pushinteger(L, msgbuf.msg_stime);
		lua_setfield(L, -2, "last_send");

		lua_pushinteger(L, msgbuf.msg_ctime);
		lua_setfield(L, -2, "change_time");

		lua_pushinteger(L, msgbuf.msg_rtime);
		lua_setfield(L, -2, "last_recv");

		lua_pushinteger(L, msgbuf.msg_lspid);
		lua_setfield(L, -2, "lasts_pid");

		lua_pushinteger(L, msgbuf.msg_lrpid);
		lua_setfield(L, -2, "lastr_pid");

		return 1;
	}
	else if (cmd == IPC_RMID)
	{
		msgctl(msgid, IPC_RMID, &msgbuf);
	}

	return 0;
}

