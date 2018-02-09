#include "UserClassBase.h"

void LuaUserClass::SetupLuaUserClassMetatable(lua_State* L)
{
	TryGetGlobalField(L, "_LuaRootObj.Set");
	lua_setfield(L, -2, "Set");

	TryGetGlobalField(L, "_LuaRootObj.Get");
	lua_setfield(L, -2, "Get");

	TryGetGlobalField(L, "_LuaRootObj.Value");
	lua_setfield(L, -2, "Value");

	AddMethod(L, [](lua_State* _lstate)->int
	{
		LuaUserClass* obj_ = GetUserData<LuaUserClass>(_lstate);
		obj_->Reset();
		return 0;
	}, "Reset");

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

int CallMethod(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "CallMethod", LUA_TUSERDATA, LUA_TSTRING)) return 0;

	string method = lua_tostring(L, 2);
	lua_remove(L, 2);

	return methodList[method]();
}
