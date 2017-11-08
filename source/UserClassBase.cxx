#include "UserClassBase.h"

void LuaUserClass::SetupMetatable(lua_State* L)
{
	AddMethod(L, GetMember, "Get");
	AddMethod(L, SetMember, "Set");
	AddMethod(L, GetMemberValue, "Value");
	AddMethod(L, ResetValues, "Reset");
	AddMethod(L, CallMethod, "Call");

	AddNonClassMethods(L);

	lua_newtable(L);
	MakeAccessors(L);
	lua_setfield(L, -2, "members");

	lua_newtable(L);
	for (unsigned int i = 0; i < methods.size(); i++)
	{
		lua_pushstring(L, methods[i].c_str());
		lua_seti(L, -2, i + 1);
	}
	lua_setfield(L, -2, "methods");

	lua_getglobal(L, "MakeEasyMethodCalls");
	lua_pushvalue(L, -2);
	lua_pcall(L, 1, 0, 0);
}

int GetMember(lua_State* L)
{
	string member = lua_tostringx(L, 2);

	if (!member.empty())
	{
		lua_getfield(L, 1, "members");
		lua_getfield(L, -1, member.c_str());
	}

	return 1;
}

int GetMemberValue(lua_State* L)
{
	string member = lua_tostringx(L, 2);

	if (!member.empty())
	{
		lua_getfield(L, 1, "members");
		lua_getfield(L, -1, member.c_str());
		lua_getfield(L, -1, "Get");
		lua_insert(L, -2);
		lua_pcall(L, 1, 1, 0);
	}
	else lua_pushnil(L);

	return 1;
}

int SetMember(lua_State* L)
{
	string member = lua_tostringx(L, 2);
//
	if (!member.empty())
	{
		lua_getfield(L, 1, "members");
		lua_getfield(L, -1, member.c_str());
		lua_remove(L, 1);
		lua_insert(L, 1);
		lua_pop(L, 1);
		luaExt_SetUserDataValue(L);
	}
	else
	{
		luaExt_SetUserDataValue(L);
	}

	return 0;
}

int ResetValues(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "ResetValues", LUA_TUSERDATA)) return 0;

	lua_settop(L, 1);
	lua_getfield(L, -1, "Set");
	lua_insert(L, 1);

	lua_pcall(L, 1, 0, 0);

	return 0;
}

int CallMethod(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "CallMethod", LUA_TUSERDATA, LUA_TSTRING)) return 0;

	string method = lua_tostring(L, 2);
	lua_remove(L, 2);

	return methodList[method]();
}
