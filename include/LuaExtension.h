#ifndef __LUAEXT__
#define __LUAEXT__

#include <iostream>
#include <stdlib.h>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <queue>
#include <random>
#include <type_traits>
#include <pthread.h>
#include <dirent.h>
#include <utility>
#include <tuple>
#include <array>
#include <chrono>
#include <ctime>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <list>
#include <functional>
#include <cassert>
#include <algorithm>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include <readline/history.h>

using namespace std;

class LuaUserClass;

extern lua_State* lua;

int LuaGetEnv(lua_State* L);

int LuaSysFork(lua_State* L);
int LuaSysExecvpe(lua_State* L);

inline int appendtohist(lua_State* L)
{
	append_history(1, histPath);
	return 0;
}

inline int trunctehistoryfile(lua_State* L)
{
	if (lua_type(L, 1) != LUA_TNUMBER) return 0;

	int maxHistLength = lua_tointeger(L, 1);

	history_truncate_file(histPath, maxHistLength);
	return 0;
}

inline int saveprompthistory(lua_State* L)
{
	HIST_ENTRY* hentry = current_history();
	if (hentry != nullptr && (((string) hentry->line) == "q()" || ((string) hentry->line) == "exit()")) remove_history(where_history());

	write_history(histPath);
	trunctehistoryfile(L);
	return 0;
}

inline int clearprompthistory(lua_State* L)
{
	clear_history();
	history_truncate_file(histPath, 0);
	return 0;
}

inline string GetLuaTypename(int type_)
{
	if (type_ == LUA_TNUMBER) return "number";
	else if (type_ == LUA_TFUNCTION) return "function";
	else if (type_ == LUA_TBOOLEAN) return "boolean";
	else if (type_ == LUA_TSTRING) return "string";
	else if (type_ == LUA_TUSERDATA) return "userdata";
	else if (type_ == LUA_TLIGHTUSERDATA) return "light userdata";
	else if (type_ == LUA_TTHREAD) return "thread";
	else if (type_ == LUA_TTABLE) return "table";
	else if (type_ == LUA_TNIL) return "nil";
	else return "unknown";
}

inline void DumpLuaStack(lua_State* L)
{
	cout << ":::::::::: Lua Stack Dump :::::::::::::" << endl;

	for (int i = 0; i < lua_gettop(L); i++)
	{
		cout << "Stack pos " << i + 1 << " : type = " << GetLuaTypename(lua_type(L, i + 1)) << " / value = ";
		lua_getglobal(L, "print");
		lua_pushvalue(L, i + 1);
		lua_pcall(L, 1, 0, 0);
	}
}

inline bool CheckLuaArgs(lua_State* L, int argIdx, bool abortIfError, string funcName)
{
	(void) L;
	(void) argIdx;
	(void) abortIfError;
	(void) funcName;
	return true;
}

inline bool CheckLuaArgs(lua_State* L, int argIdx, bool abortIfError, string funcName, int arg)
{
	if (lua_type(L, argIdx) != arg)
	{
		if (abortIfError)
		{
			if (argIdx > 0)
			{
				cerr << "ERROR in " << funcName << " : argument #" << argIdx << " => " << GetLuaTypename(arg) << " expected, got " << lua_typename(L, lua_type(L, argIdx)) << endl;
			}
			else
			{
				cerr << "ERROR in " << funcName << " : index " << argIdx << " => " << GetLuaTypename(arg) << " expected, got " << lua_typename(L, lua_type(L, argIdx)) << endl;
			}
			lua_settop(L, 0);
		}
		return false;
	}
	return true;
}

template<typename ... Rest> bool CheckLuaArgs(lua_State* L, int argIdx, bool abortIfError, string funcName, int arg1, Rest ... argRest)
{
	if (lua_type(L, argIdx) != arg1)
	{
		if (abortIfError)
		{
			if (argIdx > 0)
			{
				cerr << "ERROR in " << funcName << " : argument #" << argIdx << " => " << GetLuaTypename(arg1) << " expected, got " << lua_typename(L, lua_type(L, argIdx)) << endl;
			}
			else
			{
				argIdx = lua_gettop(L) + argIdx + 1;
				cerr << "ERROR in " << funcName << " : index " << argIdx << " => " << GetLuaTypename(arg1) << " expected, got " << lua_typename(L, lua_type(L, argIdx)) << endl;
			}
			lua_settop(L, 0);
		}
		return false;
	}
	else return CheckLuaArgs(L, argIdx + 1, abortIfError, funcName, argRest...);
}

inline bool lua_checkfield(lua_State* L, int idx, string field, int type)
{
	lua_getfield(L, idx, field.c_str());

	if (CheckLuaArgs(L, -1, false, "", type))
	{
		return true;
	}
	else
	{
		lua_pop(L, 1);
		return false;
	}

}

inline bool lua_unpackarguments(lua_State* L, int index, string errmsg, vector<const char*> arg_names, vector<int> arg_types, vector<bool> arg_required)
{
	if (arg_names.size() != arg_types.size() || arg_names.size() != arg_required.size())
	{
		cerr << "Error in arguments vectors in lua_unpackarguments..." << endl;
		return false;
	}

	int const_idx = index;

	if (const_idx < 0) const_idx = lua_gettop(L) + const_idx + 1;

	for (unsigned int i = 0; i < arg_names.size(); i++)
	{
		lua_getfield(L, const_idx, arg_names[i]);
		if (arg_required[i] && !CheckLuaArgs(L, -1, true, errmsg, arg_types[i])) return false;
	}

	return true;
}

lua_State* InitLuaEnv();

void TryGetGlobalField(lua_State* L, string gfield);
bool TrySetGlobalField(lua_State* L, string gfield);

void DoForEach(lua_State* L, int index, function<bool(lua_State* L_)> dofn);

template<typename T> T* GetLuaField(lua_State* L, int index = -1, string field = "")
{
	void* ret_hack;

	if (field.empty()) lua_pushvalue(L, index);
	else lua_getfield(L, index, field.c_str());

	if (lua_type(L, -1) != LUA_TNIL)
	{
		if (is_same<T, int>::value && lua_type(L, -1) == LUA_TNUMBER)
		{
			int* ret_int = new int(lua_tointeger(L, -1));
			ret_hack = (void*) ret_int;
		}
		else if ((is_same<T, float>::value || is_same<T, double>::value) && lua_type(L, -1) == LUA_TNUMBER)
		{
			float* ret_float = new float(lua_tonumber(L, -1));
			ret_hack = (void*) ret_float;
		}
		else if (is_same<T, string>::value && lua_type(L, -1) == LUA_TSTRING)
		{
			string* ret_str = new string(lua_tostring(L, -1));
			ret_hack = (void*) ret_str;
		}
		else if (is_same<T, bool>::value && lua_type(L, -1) == LUA_TBOOLEAN)
		{
			bool* ret_bool = new bool(lua_toboolean(L, -1));
			ret_hack = (void*) ret_bool;
		}

		lua_pop(L, 1);
	}

	return ((T*) (ret_hack));
}

template<typename T> bool TryFindLuaTableValue(lua_State* L, int index, T value_)
{
	if (index < 0) index = lua_gettop(L) + index + 1;

	if (lua_type(L, index) != LUA_TTABLE)
	{
		cerr << "Called TryFindLuaTableValue on a " << lua_typename(L, lua_type(L, index)) << endl;
		return false;
	}

	int prevStackSize = lua_gettop(L);

	DoForEach(L, index, [=] ( lua_State* L_ )
	{
		T* val = GetLuaField<T> ( L_, -1 );

		if ( val != nullptr && *val == value_ )
		{
			lua_pushvalue ( L_, -2 );
			lua_pushvalue ( L_, -2 );
			return true;
		}
		else return false;
	});

	int newStackSize = lua_gettop(L);

	if (newStackSize != prevStackSize && newStackSize != prevStackSize + 2)
	{
		cerr << "Something went wrong while looking for a value in the lua table! Stack balance is " << newStackSize - prevStackSize << endl;
	}

	return newStackSize == prevStackSize + 2;
}

inline const char* lua_tostringx(lua_State* L, int index)
{
	if (lua_type(L, index) == LUA_TSTRING) return lua_tostring(L, index);
	else return "";
}

// ************************************************************************************************ //
// *************************************** API Helper Funcs *************************************** //
// ************************************************************************************************ //

int LuaListDirContent(lua_State* L);

template<typename T> T** NewUserData(lua_State* L)
{
	T** obj = reinterpret_cast<T**>(lua_newuserdata(L, sizeof(T*)));
	*obj = new T();

	return obj;
}

template<typename T> T** NewUserData(lua_State* L, T* init)
{
	T** obj = reinterpret_cast<T**>(lua_newuserdata(L, sizeof(T*)));
	*obj = init;

	return obj;
}

template<typename T> T** NewUserDataArray(lua_State* L, int size)
{
	T** obj = reinterpret_cast<T**>(lua_newuserdata(L, sizeof(T*)));
	*obj = new T[size];

	return obj;
}

inline void MakeMetatable(lua_State* L)
{
	lua_newtable(L);

	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__newindex");

	lua_setmetatable(L, -2);
}

inline void AddMethod(lua_State* L, lua_CFunction func, const char* funcname)
{
	lua_pushcfunction(L, func);
	lua_setfield(L, -2, funcname);
}

int luaExt_SetUserDataValue(lua_State* L);
int luaExt_GetUserDataValue(lua_State* L);
int luaExt_PushBackUserDataValue(lua_State* L);

template<typename >
struct is_std_vector: std::false_type {
};

template<typename T, typename A>
struct is_std_vector<vector<T, A>> : std::true_type {
};

template<typename >
struct is_std_tuple: std::false_type {
};

template<typename ... Args>
struct is_std_tuple<tuple<Args...>> : std::true_type {
};

template<typename T, typename Arg> typename enable_if<!is_same<T*, Arg>::value, T**>::type NewUserData(lua_State* L, Arg initializer)
{
	T** obj = reinterpret_cast<T**>(lua_newuserdata(L, sizeof(T*)));
	*obj = new T(initializer);

	return obj;
}

template<typename T, typename ... Args> typename enable_if<sizeof...(Args) >= 2, T**>::type NewUserData(lua_State* L, Args ... initializers)
{
	T** obj = reinterpret_cast<T**>(lua_newuserdata(L, sizeof(T*)));
	*obj = new T(initializers...);

	return obj;
}

template<typename T> T* GetUserData(lua_State* L, int idx = 1, string errmsg = "Error in GetUserData:")
{
	if (!CheckLuaArgs(L, idx, true, errmsg, LUA_TUSERDATA))
	{
		return nullptr;
	}

	T* obj = *(static_cast<T**>(lua_touserdata(L, idx)));

	return obj;
}

template<typename T> T** GetUserDataPtr(lua_State* L, int idx = 1, string errmsg = "Error in GetUserData:")
{
	if (!CheckLuaArgs(L, idx, true, errmsg, LUA_TUSERDATA))
	{
		return nullptr;
	}

	T** obj = static_cast<T**>(lua_touserdata(L, idx));

	return obj;
}

template<typename T> typename enable_if<is_base_of<LuaUserClass, T>::value>::type SetupMetatable(lua_State* L)
{
	T* obj = GetUserData<T>(L, -1);
	obj->SetupMetatable(L);
}

template<typename T> typename enable_if<!is_base_of<LuaUserClass, T>::value>::type SetupMetatable(lua_State* L)
{
	AddMethod(L, luaExt_SetUserDataValue, "Set");
	AddMethod(L, luaExt_GetUserDataValue, "Get");
}

// ************************************************************************************************ //
// ********************************** Auto Set and Get Functions ********************************** //
// ************************************************************************************************ //

template<typename T> typename enable_if<is_convertible<T, bool>::value && is_convertible<T, int>::value>::type lua_trygetboolean(lua_State* L, int index, T* dest)
{
	if (lua_type(L, index) == LUA_TNIL) *dest = 0;
	else if (lua_type(L, index) == LUA_TUSERDATA) *dest = *GetUserData<T>(L, index);
	else *dest = lua_toboolean(L, index);
}

template<typename T> typename enable_if<!is_convertible<T, bool>::value || !is_convertible<T, int>::value>::type lua_trygetboolean(lua_State* L, int index, T* dest)
{
	return;
}

template<typename T> typename enable_if<is_integral<T>::value>::type lua_trygetinteger(lua_State* L, int index, T* dest)
{
	if (lua_type(L, index) == LUA_TNIL) *dest = 0;
	else if (lua_type(L, index) == LUA_TUSERDATA) *dest = *GetUserData<T>(L, index);
	else *dest = lua_tointeger(L, index);
}

template<typename T> typename enable_if<!is_integral<T>::value>::type lua_trygetinteger(lua_State* L, int index, T* dest)
{
	return;
}

template<typename T> typename enable_if<is_convertible<T, double>::value>::type lua_trygetnumber(lua_State* L, int index, T* dest)
{
	if (lua_type(L, index) == LUA_TNIL) *dest = 0;
	else if (lua_type(L, index) == LUA_TUSERDATA) *dest = *GetUserData<T>(L, index);
	else *dest = lua_tonumber(L, index);
}

template<typename T> typename enable_if<!is_convertible<T, double>::value>::type lua_trygetnumber(lua_State* L, int index, T* dest)
{
	return;
}

template<typename T> typename enable_if<is_convertible<T, string>::value>::type lua_trygetstring(lua_State* L, int index, T* dest)
{
	if (lua_type(L, index) == LUA_TNIL) *dest = "";
	else if (lua_type(L, index) == LUA_TUSERDATA) *dest = *GetUserData<T>(L, index);
	else *dest = lua_tostring(L, index);
}

template<typename T> typename enable_if<!is_convertible<T, string>::value>::type lua_trygetstring(lua_State* L, int index, T* dest)
{
	return;
}

template<typename T> typename enable_if<!is_pointer<T>::value && is_base_of<LuaUserClass, T>::value>::type lua_trygetuserdata(lua_State* L, int index, T* dest)
{
	if (lua_type(L, index) == LUA_TNIL) return;
	else *dest = *GetUserData<T>(L, index);
}

template<typename T> typename enable_if<!is_pointer<T>::value && !is_base_of<LuaUserClass, T>::value>::type lua_trygetuserdata(lua_State* L, int index, T* dest)
{
	if (lua_type(L, index) == LUA_TNIL) return;
	else *dest = *GetUserData<T>(L, index);
	return;
}

template<typename T> typename enable_if<is_pointer<T>::value && is_base_of<LuaUserClass, T>::value>::type lua_trygetuserdata(lua_State* L, int index, T* dest)
{
	if (lua_type(L, index) == LUA_TNIL) return;
	else *dest = GetUserData<T>(L, index);
}

template<typename T> typename enable_if<is_pointer<T>::value && !is_base_of<LuaUserClass, T>::value>::type lua_trygetuserdata(lua_State* L, int index, T* dest)
{
	if (lua_type(L, index) == LUA_TNIL) return;
	else *dest = *GetUserData<T>(L, index);
	return;
}

template<typename T> int lua_autosetvalue(lua_State* L, T* dest, int type = -1, int index = -1, string errmsg = "Bad setter value")
{
	if (lua_gettop(L) == 1) lua_pushnil(L);

	if (type >= 0 && !CheckLuaArgs(L, index, true, errmsg, type)) return 0;

	if (type == LUA_TBOOLEAN) lua_trygetboolean(L, index, dest);
	else if (type == LUA_TNUMBER) lua_trygetnumber(L, index, dest);
	else if (type == LUA_TSTRING) lua_trygetstring(L, index, dest);
	else if (type == LUA_TTABLE) lua_autosetvector(L, dest, type, index);
	else if (type == LUA_TUSERDATA) lua_trygetuserdata(L, index, dest);
	else if (type == -1)
	{
		if (lua_type(L, index) == LUA_TTABLE) lua_autosetvector(L, dest, type);
		else if (is_same<T, bool>::value) lua_trygetnumber(L, index, dest);
		else if (is_integral<T>::value) lua_trygetinteger(L, index, dest);
		else if (is_convertible<T, double>::value) lua_trygetnumber(L, index, dest);
		else if (is_convertible<T, string>::value) lua_trygetstring(L, index, dest);
		else lua_trygetuserdata(L, index, dest);
	}

	lua_remove(L, index);

	return 0;
}

template<typename R, typename ... Args> int lua_autosetvalue(lua_State* L, function<R(Args...)>* dest, int type = -1, int index = -1, string errmsg = "Bad setter value")
{
	if (type == -1 || type == LUA_TFUNCTION)
	{
		if (lua_type(L, index) == LUA_TNIL) return 0;

		TryGetGlobalField(L, "_stdfunctions.unnamed");
		int func_nbr = lua_tointeger(L, -1) + 1;
		lua_pop(L, 1);
		lua_pushinteger(L, func_nbr);
		TrySetGlobalField(L, "_stdfunctions.unnamed");

		lua_pushvalue(L, index);

		string fn_gfield = ("_stdfunctions.func" + to_string(func_nbr));
		bool success = TrySetGlobalField(L, fn_gfield);

		if (!success)
		{
			cerr << "Error in lua_autosetvalue with std::function: failed to push fn as global field" << endl;
			lua_remove(L, index);
			return 0;
		}

//		cout << "Fn set to global field " << fn_gfield << endl;

		*dest = [=] ( Args... args ) -> R
		{
			TryGetGlobalField ( L, fn_gfield );

			LuaMultPush(L, args...);

			lua_pcall(L, sizeof...(Args), LUA_MULTRET, 0);

			R* res = new R();
			lua_autosetvalue ( L, res, -1 );

			return *res;
		};

		lua_remove(L, index);
	}

	return 0;
}

inline int lua_autosetvalue(lua_State* L, char* dest, int type = -1, int index = -1, string errmsg = "Bad setter value")
{
	if (lua_gettop(L) == 1) lua_pushnil(L);

	dest = new char[1024];
	sprintf(dest, "%s", lua_tostringx(L, index));

	lua_remove(L, index);

	return 0;
}

template<typename T> int lua_autopushback(lua_State* L, vector<T>* dest, int type = -1, int index = -1, string errmsg = "Bad setter push_back vector")
{
	T* new_elem = new T;

	lua_autosetvalue(L, new_elem, type, index, errmsg);

	dest->push_back(*new_elem);

	return 0;
}

template<typename T> int lua_autosetvector(lua_State* L, vector<T>* dest, int type = -1, int index = -1, string errmsg = "Bad setter vector")
{
	if (lua_gettop(L) == 1)
	{
		dest->clear();
		return 0;
	}

	if (!CheckLuaArgs(L, index, false, errmsg, LUA_TTABLE)) return lua_autopushback(L, dest, type, index, errmsg);

	unsigned int length = lua_rawlen(L, index);

	T* new_elem = new T;

	dest->clear();

	for (unsigned int i = 0; i < length; i++)
	{
		lua_geti(L, index, i + 1);

		lua_autosetvalue(L, new_elem, type, -1, errmsg);

		dest->push_back(*new_elem);
	}

	lua_remove(L, index);

	return 0;
}

template<typename T> int lua_autosetvector(lua_State* L, T* dest, int type = -1, int index = -1, string errmsg = "Bad setter vector")
{
	return 0;
}

template<typename T> int lua_autosetarray(lua_State* L, T* dest, unsigned int size, int type = -1, int index = -1, string errmsg = "Bad setter array")
{
	if (lua_gettop(L) == 1)
	{
		T* new_elem = new T;

		for (unsigned int i = 0; i < size; i++)
		{
			lua_pushinteger(L, 0);
			lua_autosetvalue(L, new_elem, type, -1, errmsg);
			dest[i] = *new_elem;
		}
		return 0;
	}
	else if (!CheckLuaArgs(L, index, false, errmsg, LUA_TTABLE))
	{
		if (!CheckLuaArgs(L, index, false, errmsg, LUA_TNUMBER)) return 0;

		unsigned int add_at = lua_tointeger(L, -1);

		if (add_at > size)
		{
			cerr << "Trying to assign value outside of array range..." << endl;
			return 0;
		}

		lua_pop(L, 1);

		T* new_elem = new T;
		lua_autosetvalue(L, new_elem, type, -1, errmsg);

		dest[add_at - 1] = *new_elem;
	}
	else
	{
		unsigned int length = lua_rawlen(L, index);
		T* new_elem = new T;

		for (unsigned int i = 0; i < min(length, size); i++)
		{
			lua_geti(L, index, i + 1);
			lua_autosetvalue(L, new_elem, type, -1, errmsg);
			dest[i] = *new_elem;
		}

		for (unsigned int i = min(length, size); i < size; i++)
		{
			lua_pushinteger(L, 0);
			lua_autosetvalue(L, new_elem, type, -1, errmsg);
			dest[i] = *new_elem;
		}

		lua_remove(L, index);
	}
	return 0;
}

template<typename T> typename enable_if<is_convertible<T, bool>::value>::type lua_trypushboolean(lua_State* L, T src)
{
	lua_pushboolean(L, (bool) src);
}

template<typename T> typename enable_if<!is_convertible<T, bool>::value>::type lua_trypushboolean(lua_State* L, T src)
{
	return;
}

template<typename T> typename enable_if<is_integral<T>::value>::type lua_trypushnumber(lua_State* L, T src)
{
	lua_pushinteger(L, src);
}

template<typename T> typename enable_if<!is_integral<T>::value && is_convertible<T, double>::value>::type lua_trypushnumber(lua_State* L, T src)
{
	lua_pushnumber(L, src);
}

template<typename T> typename enable_if<!is_convertible<T, double>::value>::type lua_trypushnumber(lua_State* L, T src)
{
	return;
}

template<typename T> typename enable_if<is_convertible<T, string>::value>::type lua_trypushstring(lua_State* L, T src)
{
	string src_str = (string) src;
	lua_pushstring(L, src_str.c_str());
}

template<typename T> typename enable_if<!is_convertible<T, string>::value>::type lua_trypushstring(lua_State* L, T src)
{
	return;
}

template<typename T> typename enable_if<is_base_of<LuaUserClass, T>::value && is_pointer<T>::value>::type lua_trypushuserdata(lua_State* L, T src)
{
	T* obj = *(NewUserData<T>(L));
	*obj = src;

	obj->SetupMetatable(L);
	obj->MakeAccessors(L);
}

template<typename T> typename enable_if<is_base_of<LuaUserClass, T>::value && !is_pointer<T>::value>::type lua_trypushuserdata(lua_State* L, T src)
{
	T* obj = *(NewUserData<T>(L));
	obj = &src;

	obj->SetupMetatable(L);
	obj->MakeAccessors(L);
}

template<typename T> typename enable_if<!is_base_of<LuaUserClass, T>::value && is_pointer<T>::value>::type lua_trypushuserdata(lua_State* L, T src)
{
	T* obj = reinterpret_cast<T*>(lua_newuserdata(L, sizeof(T)));
	*obj = src;

	MakeMetatable(L);
	AddMethod(L, luaExt_SetUserDataValue, "Set");
	AddMethod(L, luaExt_GetUserDataValue, "Get");
}

template<typename T> typename enable_if<!is_base_of<LuaUserClass, T>::value && !is_pointer<T>::value>::type lua_trypushuserdata(lua_State* L, T& src)
{
	T** obj = reinterpret_cast<T**>(lua_newuserdata(L, sizeof(T*)));
	*obj = &src;

	MakeMetatable(L);
	AddMethod(L, luaExt_SetUserDataValue, "Set");
	AddMethod(L, luaExt_GetUserDataValue, "Get");
}

inline void lua_trypushuserdata(lua_State* L, const char* src)
{
	char** obj = reinterpret_cast<char**>(lua_newuserdata(L, 1024));
	sprintf(*obj, "%s", src);

	MakeMetatable(L);
	AddMethod(L, luaExt_SetUserDataValue, "Set");
	AddMethod(L, luaExt_GetUserDataValue, "Get");
}

template<typename T> typename enable_if<!is_std_vector<T>::value, int>::type lua_autogetvalue(lua_State* L, T src, int type = -1, string errmsg = "Bad getter value")
{
	if (type == LUA_TBOOLEAN) lua_trypushboolean(L, src);
	else if (type == LUA_TNUMBER) lua_trypushnumber(L, src);
	else if (type == LUA_TSTRING) lua_trypushstring(L, src);
	else if (type == LUA_TUSERDATA) lua_trypushuserdata(L, src);
	else if (type == LUA_TTABLE) lua_trypushtable(L, src);
	else if (type == -1)
	{
		if (is_std_tuple<T>::value) lua_trypushtable(L, src);
		else if (is_same<T, bool>::value) lua_trypushboolean(L, src);
		else if (is_convertible<T, double>::value) lua_trypushnumber(L, src);
		else if (is_convertible<T, string>::value) lua_trypushstring(L, src);
		else lua_trypushuserdata(L, src);
	}

	return 1;
}

template<typename T> typename enable_if<is_std_vector<T>::value, int>::type lua_autogetvalue(lua_State* L, T src, int type = -1, string errmsg = "Bad getter value")
{
	return lua_autogetvector(L, src, type, errmsg);
}

inline int lua_autogetvalue(lua_State* L, void* src, int type = -1, string errmsg = "Bad getter value")
{
	return 0;
}

template<typename T> int lua_autogetvector(lua_State* L, vector<T> src, int type = -1, string errmsg = "Bad getter vector")
{
	lua_newtable(L);

	for (unsigned int i = 0; i < src.size(); i++)
	{
		lua_autogetvalue(L, src.at(i), type, errmsg);
		lua_seti(L, -2, i + 1);
	}

	return 1;
}

template<typename T> int lua_autogetarray(lua_State* L, T* src, unsigned int size, int type = -1, string errmsg = "Bad getter array")
{
	lua_newtable(L);

	for (unsigned int i = 0; i < size; i++)
	{
		lua_autogetvalue(L, src[i], type, errmsg);
		lua_seti(L, -2, i + 1);
	}

	return 1;
}

int luaExt_GetVectorSize(lua_State* L);

// --------------------------------------------------------------------------------------------------------- //
// ----------------------- Multi-return/push from Lua and function call ------------------------------------ //
// --------------------------------------------------------------------------------------------------------- //

extern map<string, function<int()>> methodList;
extern map<string, map<int, function<void(int)>>> constructorList;

template<int...> struct seq
{};

template<int N, int ...S> struct gens: gens<N - 1, N - 1, S...> {
};

template<int ...S> struct gens<0, S...> {
		typedef seq<S...> type;
};

template<size_t, typename R, typename ... Args>
struct StoreArgsAndCallFn {
		R (*fptr)(Args...);
		tuple<Args...> params;

		R DoCallFn()
		{
			return CallFn(typename gens<sizeof...(Args)>::type());
		}

		template<int ...S>
		R CallFn(seq<S...>)
		{
			return (*fptr)(std::get<S>(params) ...);
		}
};

template<typename R, typename ... Args>
struct StoreArgsAndCallFn<0, R, Args...> {
		R (*fptr)(Args...);
		void* params;

		R DoCallFn()
		{
			return (*fptr)();
		}
};

// ------------------ Push -------------------- //

inline void LuaMultPush(lua_State* L)
{
	return;
}

inline void LuaMultPush(lua_State* L1, lua_State* L2)  // Ugly but required due to current designed (see below). Try to change it t some point?
{
	return;
}

template<typename T> void LuaMultPush(lua_State* L, T val)
{
	lua_autogetvalue(L, val, -1);
}

template<typename First, typename ... Rest> void LuaMultPush(lua_State* L, First fst, Rest ... rest)
{
	lua_autogetvalue(L, fst, -1);
	LuaMultPush<Rest...>(L, rest...);
}

template<typename ... Ts> void lua_trypushtable(lua_State* L, tuple<Ts...> src)
{
	const int nargs = sizeof...(Ts);

	lua_newtable(L);
	int table_pos = lua_gettop(L);
	StoreArgsAndCallFn<nargs, void, lua_State*, Ts...> retrieved =  // LuaMultPush takes as first argument a lua_State* so we need to insert it between void and Ts...
				{ LuaMultPush, tuple_cat(make_tuple(L), src) };  // But then it tries to unpack src as tuple<lua_State*, Ts...> so we artificially add one at the beginning...
	retrieved.DoCallFn();

	for (int i = 0; i < nargs; i++)
		lua_seti(L, table_pos, nargs - i);
}

template<typename T> typename enable_if<!is_std_tuple<T>::value>::type lua_trypushtable(lua_State* L, T src)
{
	return;
}

// ------------------ Pop and Call ----------------- //

template<typename T, typename R, typename ... Args> union LuaMemFuncCaller {
		R (T::*mfptr)(Args...);
		R (T::*const_mfptr)(Args...) const;
};

template<size_t, typename T, typename R, typename ... Args>
struct StoreArgsAndCallMemberFn {
		LuaMemFuncCaller<T, R, Args...> func;
		tuple<Args...> params;
		T* obj;

		R DoCallMemFn()
		{
			return CallMemFn(typename gens<sizeof...(Args)>::type());
		}

		template<int ...S>
		R CallMemFn(seq<S...>)
		{
			return (obj->*(func.mfptr))(std::get<S>(params) ...);
		}

		R DoCallConstMemFn()
		{
			return CallConstMemFn(typename gens<sizeof...(Args)>::type());
		}

		template<int ...S>
		R CallConstMemFn(seq<S...>)
		{
			return (obj->*(func.const_mfptr))(std::get<S>(params) ...);
		}
};

template<typename T, typename R, typename ... Args>
struct StoreArgsAndCallMemberFn<0, T, R, Args...> {
		LuaMemFuncCaller<T, R, Args...> func;
		void* params;
		T* obj;

		R DoCallMemFn()
		{
			return (obj->*(func.mfptr))();
		}

		R DoCallMemFnConst()
		{
			return (obj->*(func.const_mfptr))();
		}

		R DoCallFn()
		{
			return ((func.fptr))();
		}
};

template<size_t, typename ... Ts>
struct LuaPopHelper {
		typedef std::tuple<Ts...> type;

		template<typename T>
		static tuple<T> LuaStackToTuple(lua_State* L, const int index)
		{
			T* stack_element = new T();
			if (is_std_vector<T>::value) lua_autosetvector(L, stack_element, -1, index);
			else lua_autosetvalue(L, stack_element, -1, index);
			return make_tuple(*stack_element);
		}

		// inductive case
		template<typename T1, typename T2, typename ... Rest>
		static tuple<T1, T2, Rest...> LuaStackToTuple(lua_State* L, const int index)
		{
			T1* stack_element = new T1();
			if (is_std_vector<T1>::value) lua_autosetvector(L, stack_element, -1, index);
			else lua_autosetvalue(L, stack_element, -1, index);
			tuple<T1> head = make_tuple(*stack_element);
			return tuple_cat(head, LuaStackToTuple<T2, Rest...>(L, index));
		}

		static std::tuple<Ts...> DoPop(lua_State* L, int index = 1)
		{
			auto ret = LuaStackToTuple<Ts...>(L, index + 1);
			return ret;
		}
};

template<typename ... Ts>
struct LuaPopHelper<0, Ts...> {
		typedef void* type;

		static void* DoPop(lua_State* L, int index)
		{
			return nullptr;
		}
};

template<typename T, typename R, typename ... Args> typename LuaPopHelper<sizeof...(Args), Args...>::type LuaMultPop(lua_State* L, int index, R (T::*method)(Args...))
{
	return LuaPopHelper<sizeof...(Args), Args...>::DoPop(L, index);
}

template<typename T, typename R, typename ... Args> typename LuaPopHelper<sizeof...(Args), Args...>::type LuaMultPop(lua_State* L, int index, R (T::*method)(Args...) const)
{
	return LuaPopHelper<sizeof...(Args), Args...>::DoPop(L, index);
}

template<typename T, typename R, typename ... Args> R CallFuncsWithArgs(lua_State* L, int index, R (*func)(Args...))
{
	auto args = LuaPopHelper<sizeof...(Args), Args...>::DoPop(L, index);

	StoreArgsAndCallFn<sizeof...(Args), R, Args...> retrieved =
		{ func, args };
	return retrieved.DoCallFn();
}

template<typename T, typename ... Args> void MakeDefaultConstructor(lua_State* L, string name)
{
	auto ctor = [=](int index)
	{
		NewUserData<T>(L);

		MakeMetatable ( L );
		SetupMetatable<T> ( L );

		return;
	};

	constructorList[name][0] = ctor;

	lua_getglobal(L, "MakeEasyConstructors");
	lua_pushstring(L, name.c_str());
	lua_pcall(L, 1, 0, 0);
}

template<typename T, typename ... Args> void AddObjectConstructor(lua_State* L, string name)
{
	auto ctor = [=](int index)
	{
		auto args = LuaPopHelper<sizeof...(Args), Args...>::DoPop(L, index);

		auto final_args = tuple_cat(make_tuple(L), args);
		StoreArgsAndCallFn<sizeof...(Args), T**, lua_State*, Args...> retrieved =
		{	NewUserData<T>, final_args};

		T** obj = reinterpret_cast<T**>(lua_newuserdata(L, sizeof(T*)));
		obj = retrieved.DoCallFn();

		MakeMetatable ( L );
		SetupMetatable<T> ( L );

		return;
	};

	constructorList[name][sizeof...(Args)] = ctor;
}

inline int LuaCtor(lua_State* L, int index = 1)
{
	string classname = lua_tostring(L, index);

	if (index < 0) index = lua_gettop(L) + index + 1;

	int nargs = lua_gettop(L) - index;

	int arraySize = 0;
	size_t findIfArray = classname.find("[");

	if (findIfArray != string::npos)
	{
		size_t endArraySize = classname.find("]");
		arraySize = stoi(classname.substr(findIfArray + 1, endArraySize - findIfArray - 1));
		classname = classname.substr(0, findIfArray) + "[]";
	}

	(constructorList[classname][nargs])(index);

	lua_pushstring(L, classname.c_str());
	lua_setfield(L, -2, "type");

	if (findIfArray != string::npos)
	{
		lua_pushinteger(L, arraySize);
		lua_setfield(L, -2, "array_size");
		AddMethod(L, luaExt_GetVectorSize, "GetSize");

		lua_getfield(L, -1, "Set");
		lua_pushvalue(L, -2);
		lua_pcall(L, 1, 0, 0);
	}

	if (classname.find("vector") != string::npos)
	{
		AddMethod(L, luaExt_PushBackUserDataValue, "PushBack");
		AddMethod(L, luaExt_GetVectorSize, "GetSize");
	}

	return 1;
}

inline int luaExt_Ctor(lua_State* L)
{
	return LuaCtor(L);
}

// --------------------------------------------------------------------------------------------------------- //
// -------------------------------- Accessor Helper functions ---------------------------------------------- //
// --------------------------------------------------------------------------------------------------------- //

extern map<string, function<void(lua_State*)>> newUserDataFns;
extern map<string, function<void(lua_State*)>> setUserDataFns;
extern map<string, function<void(lua_State*)>> getUserDataFns;

template<typename T> void MakeAccessorsUserDataFuncs(lua_State* _lstate, string type)
{
	string finalType = type;

	MakeDefaultConstructor<T>(_lstate, finalType);
	AddObjectConstructor<T, T>(_lstate, finalType);

	setUserDataFns[finalType] = [=] ( lua_State* L )
	{
		T* ud = GetUserData<T> ( L, 1, "setUserDataFns" );
		lua_autosetvalue ( L, ud, -1 );
	};

	getUserDataFns[finalType] = [=] ( lua_State* L )
	{
		T* ud = GetUserData<T> ( L, 1, "getUserDataFns" );
		lua_autogetvalue ( L, *ud, -1 );
	};

	finalType = "vector<" + type + ">";

	MakeDefaultConstructor<vector<T>>(_lstate, finalType);
	AddObjectConstructor<vector<T>, vector<T>>(_lstate, finalType);

	setUserDataFns[finalType] = [=] ( lua_State* L )
	{
		vector<T>* ud = GetUserData<vector<T>> ( L, 1, "setUserDataFns" );
		lua_autosetvector ( L, ud, -1 );
	};

	getUserDataFns[finalType] = [=] ( lua_State* L )
	{
		vector<T>* ud = GetUserData<vector<T>> ( L, 1, "getUserDataFns" );
		lua_autogetvalue ( L, *ud, -1 );
	};

	finalType = "vector<vector<" + type + ">>";

	MakeDefaultConstructor<vector<vector<T>>>(_lstate, finalType);

	setUserDataFns[finalType] = [=] ( lua_State* L )
	{
		vector<vector<T>>* ud = GetUserData<vector<vector<T>>> ( L, 1, "setUserDataFns" );
		lua_autosetvector ( L, ud, -1 );
	};

	getUserDataFns[finalType] = [=] ( lua_State* L )
	{
		vector<vector<T>>* ud = GetUserData<vector<vector<T>>> ( L, 1, "getUserDataFns" );
		lua_autogetvalue ( L, *ud, -1 );
	};

	finalType = type + "[]";

	MakeDefaultConstructor<T>(_lstate, finalType);
	AddObjectConstructor<T, T>(_lstate, finalType);

	setUserDataFns[finalType] = [=] ( lua_State* L )
	{
		T* ud = GetUserData<T> ( L, 1, "setUserDataFns" );
		lua_getfield ( L, 1, "array_size" );
		int array_size = lua_tointeger ( L, -1 );
		lua_pop ( L, 1 );
		lua_autosetarray ( L, ud, array_size, -1 );
	};

	getUserDataFns[finalType] = [=] ( lua_State* L )
	{
		T* ud = GetUserData<T> ( L, 1, "getUserDataFns" );
		lua_getfield ( L, 1, "array_size" );
		int array_size = lua_tointeger ( L, -1 );
		lua_pop ( L, 1 );
		lua_autogetarray ( L, ud, array_size, -1 );
	};
}

#endif

