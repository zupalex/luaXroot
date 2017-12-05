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

namespace LuaExt {
template<typename T> string to_string(const T& x)
{
	return "to_string not implemented";
}
}

using namespace LuaExt;

class LuaUserClass;

extern lua_State* lua;

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

int LuaGetEnv(lua_State* L);

int appendtohist(lua_State* L);
int trunctehistoryfile(lua_State* L);
int saveprompthistory(lua_State* L);
int clearprompthistory(lua_State* L);

int luaExt_gettime(lua_State* L);

string GetLuaTypename(int type_);

void DumpLuaStack(lua_State* L);

template<typename T> void LuaPopValue(lua_State* L, T* dest, int index = -1)
{
	*dest = **(static_cast<T**>(lua_touserdata(L, index)));
	lua_remove(L, index);
}

template<> void LuaPopValue<bool>(lua_State* L, bool* dest, int index);
template<> void LuaPopValue<char>(lua_State* L, char* dest, int index);
template<> void LuaPopValue<short>(lua_State* L, short* dest, int index);
template<> void LuaPopValue<unsigned short>(lua_State* L, unsigned short* dest, int index);
template<> void LuaPopValue<int>(lua_State* L, int* dest, int index);
template<> void LuaPopValue<unsigned int>(lua_State* L, unsigned int* dest, int index);
template<> void LuaPopValue<long>(lua_State* L, long* dest, int index);
template<> void LuaPopValue<unsigned long>(lua_State* L, unsigned long* dest, int index);
template<> void LuaPopValue<long long>(lua_State* L, long long* dest, int index);
template<> void LuaPopValue<unsigned long long>(lua_State* L, unsigned long long* dest, int index);
template<> void LuaPopValue<float>(lua_State* L, float* dest, int index);
template<> void LuaPopValue<double>(lua_State* L, double* dest, int index);
template<> void LuaPopValue<string>(lua_State* L, string* dest, int index);

void MakeMetatable(lua_State* L);
void AddMethod(lua_State* L, lua_CFunction func, const char* funcname);
void RegisterMethodInTable(lua_State* L, lua_CFunction func, const char* funcname, const char* dest_table);

bool CheckLuaArgs(lua_State* L, int argIdx, bool abortIfError, string funcName);

bool CheckLuaArgs(lua_State* L, int argIdx, bool abortIfError, string funcName, int arg);

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

bool lua_unpackarguments(lua_State* L, int index, string errmsg, vector<const char*> arg_names, vector<int> arg_types, vector<bool> arg_required);

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

int luaExt_SetUserDataValue(lua_State* L);
int luaExt_GetUserDataValue(lua_State* L);
int luaExt_PushBackUserDataValue(lua_State* L);

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
	T* obj = *(static_cast<T**>(lua_touserdata(L, idx)));
	return obj;
}

template<typename T> T** GetUserDataPtr(lua_State* L, int idx = 1, string errmsg = "Error in GetUserData:")
{
	T** obj = static_cast<T**>(lua_touserdata(L, idx));
	return obj;
}

template<typename T> int luaExt_PushBackUserDataValue(lua_State* L)
{
	T* ud = GetUserData<T>(L, 1, "luaExt_PushBackUserDataValue");
	lua_autopushback(L, ud);
	return 0;
}

// ************************************************************************************************ //
// ********************************** Auto Set and Get Functions ********************************** //
// ************************************************************************************************ //

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

//      cout << "Fn set to global field " << fn_gfield << endl;

		*dest = [=] ( Args... args ) -> R
		{
			TryGetGlobalField ( L, fn_gfield );

			LuaMultPush(L, args...);

			lua_pcall(L, sizeof...(Args), LUA_MULTRET, 0);

			R res;
			LuaPopValue<R>(L, &res);

			return res;
		};

		lua_remove(L, index);
	}

	return 0;
}

template<typename T> int lua_autopushback(lua_State* L, T* dest, int type = -1, int index = -1, string errmsg = "Bad setter push_back vector")
{
	return 0;
}

template<typename T> int lua_autopushback(lua_State* L, vector<T>* dest, int type = -1, int index = -1, string errmsg = "Bad setter push_back vector")
{
	T new_elem = T();
	LuaPopValue<T>(L, &new_elem);

	dest->push_back(new_elem);

	return 0;
}

template<typename T> int lua_autopushback(lua_State* L, vector<vector<T>>* dest, int type = -1, int index = -1, string errmsg = "Bad setter push_back vector")
{
	if (lua_type(L, index) != LUA_TTABLE)
	{
		unsigned int pushat = lua_tointeger(L, index - 1) - 1;
		if (dest->size() <= pushat) return 0;

		T new_elem;
		LuaPopValue<T>(L, &new_elem);
		dest->at(pushat).push_back(new_elem);
		return 0;
	}

	vector<T> new_elem;

	lua_autosetvector(L, &new_elem, type, index, errmsg);

	dest->push_back(new_elem);

	return 0;
}

template<typename T> int lua_autosetvector(lua_State* L, T* dest, int type = -1, int index = -1, string errmsg = "Bad setter push_back vector")
{
	return 0;
}

template<typename T> int lua_autosetvector(lua_State* L, vector<T>* dest, int type = -1, int index = -1, string errmsg = "Bad setter vector")
{
	if (lua_type(L, index) != LUA_TTABLE)
	{
		unsigned int setat = lua_tointeger(L, index - 1) - 1;
		if (dest->size() <= setat) return 0;

		T new_elem = T();
		LuaPopValue<T>(L, &new_elem);
		dest->at(setat) = new_elem;
		return 0;
	}

	unsigned int length = lua_rawlen(L, index);

	T new_elem;

	dest->clear();

	for (unsigned int i = 0; i < length; i++)
	{
		lua_geti(L, index, i + 1);
		LuaPopValue<T>(L, &new_elem);
		dest->push_back(new_elem);
	}

	lua_remove(L, index);

	return 0;
}

template<typename T> int lua_autosetvector(lua_State* L, vector<vector<T>>* dest, int type = -1, int index = -1, string errmsg = "Bad setter vector")
{
	if (lua_type(L, index) != LUA_TTABLE)
	{
		unsigned int setat = lua_tointeger(L, index - 2) - 1;
		if (dest->size() <= setat) return 0;

		vector<T>* setelem = &(dest->at(setat));

		setat = lua_tointeger(L, index - 1) - 1;
		if (setelem->size() <= setat) return 0;

		T new_elem;
		LuaPopValue<T>(L, &new_elem);

		setelem->at(setat) = new_elem;

		return 0;
	}

	vector<T> new_elem;
	unsigned int nelems = 1;

	lua_geti(L, index, 1);

	if (lua_type(L, -1) != LUA_TTABLE)
	{
		lua_pop(L, 1);
		new_elem.clear();

		unsigned int setat = lua_tointeger(L, index - 1) - 1;
		if (dest->size() <= setat) return 0;

		lua_autosetvector(L, &new_elem, type, index, errmsg);

		dest->at(setat) = new_elem;
		return 0;
	}

	lua_pop(L, 1);

	nelems = lua_rawlen(L, index);
	dest->clear();

	for (unsigned int i = 0; i < nelems; i++)
	{
		lua_geti(L, index, i + 1);

		new_elem.clear();
		lua_autosetvector(L, &new_elem, type, -1, errmsg);

		dest->push_back(new_elem);
	}

	lua_remove(L, index);

	return 0;
}

template<typename T> int lua_autosetarray(lua_State* L, T* dest, unsigned int size, int type = -1, int index = -1, string errmsg = "Bad setter array")
{
	if (lua_type(L, index) != LUA_TTABLE)
	{
		unsigned int add_at = lua_tointeger(L, index-1) - 1;

		if (add_at >= size)
		{
			cerr << "Trying to assign value outside of array range..." << endl;
			return 0;
		}

		T new_elem = T();
		LuaPopValue<T>(L, &new_elem);
		dest[add_at] = new_elem;
	}
	else
	{
		unsigned int length = lua_rawlen(L, index);
		T new_elem = T();

		for (unsigned int i = 0; i < min(length, size); i++)
		{
			lua_geti(L, index, i + 1);
			LuaPopValue<T>(L, &new_elem);
			dest[i] = new_elem;
		}

		for (unsigned int i = min(length, size); i < size; i++)
		{
			lua_pushinteger(L, 0);
			LuaPopValue<T>(L, &new_elem);
			dest[i] = new_elem;
		}

		lua_remove(L, index);
	}
	return 0;
}

template<typename T> int luaExt_ClearUserDataValue(lua_State* L)
{
	T* obj_ = *(static_cast<T**>(lua_touserdata(L, 1)));
	*obj_ = T();
	return 0;
}

template<typename T> typename enable_if<is_std_vector<T>::value, int>::type luaExt_ClearUserDataVector(lua_State* L)
{
	T* obj_ = *(static_cast<T**>(lua_touserdata(L, 1)));

	obj_->clear();
	return 0;
}

template<typename T> typename enable_if<!is_std_vector<T>::value, int>::type luaExt_ClearUserDataVector(lua_State* L)
{
	return 0;
}

template<typename T> int luaExt_ClearUserDataArray(lua_State* L)
{
	T* obj_ = *(static_cast<T**>(lua_touserdata(L, 1)));

	lua_getfield(L, 1, "array_size");
	int array_size = lua_tointeger(L, -1);
	lua_pop(L, 1);

	for (int i = 0; i < array_size; i++)
		obj_[i] = T();

	return 0;
}

template<typename T> int luaExt_ShiftUDAddress(lua_State* L)
{
	T** obj_ = static_cast<T**>(lua_touserdata(L, 1));
	int shift = lua_tointeger(L, 2);
	*obj_ += shift;
	return 0;
}

template<typename T> int luaExt_SetUDAddress(lua_State* L)
{
	T** obj_ = static_cast<T**>(lua_touserdata(L, 1));
	char* address = *((char**) lua_touserdata(L, 2));
	int offset = lua_tointegerx(L, 3, nullptr);
	address += offset;
	*obj_ = (T*) address;
	return 0;
}

template<typename T> int luaExt_AllocateUD(lua_State* L)
{
	T** obj_ = static_cast<T**>(lua_touserdata(L, 1));
	*obj_ = new T();
	return 0;
}

template<typename T> typename enable_if<is_base_of<LuaUserClass, T>::value>::type SetupMetatable(lua_State* L)
{
	lua_getglobal(L, "SetupMetatable");
	lua_pushvalue(L, -2);
	lua_pcall(L, 1, 0, 0);

	T* obj = *(static_cast<T**>(lua_touserdata(L, -1)));
	AddMethod(L, luaExt_ShiftUDAddress<T>, "ShiftAddress");
	AddMethod(L, luaExt_SetUDAddress<T>, "SetAddress");
	AddMethod(L, luaExt_AllocateUD<T>, "Allocate");

	lua_getfield(L, -1, "type");
	string type = lua_tostring(L, -1);
	lua_pop(L, 1);

	if (type.find("vector") == string::npos && type.find("[") == string::npos) obj->SetupMetatable(L);
}

template<typename T> typename enable_if<!is_base_of<LuaUserClass, T>::value>::type SetupMetatable(lua_State* L)
{
	lua_getglobal(L, "SetupMetatable");
	lua_pushvalue(L, -2);
	lua_pcall(L, 1, 0, 0);

	AddMethod(L, luaExt_ShiftUDAddress<T>, "ShiftAddress");
	AddMethod(L, luaExt_SetUDAddress<T>, "SetAddress");
	AddMethod(L, luaExt_AllocateUD<T>, "Allocate");
}

template<typename T> typename enable_if<!is_std_tuple<T>::value && !is_std_vector<T>::value>::type LuaPushValue(lua_State* L, T src)
{
	T* obj = *(reinterpret_cast<T**>(lua_newuserdata(L, sizeof(T*))));
	obj = new T();
	*obj = src;
	MakeMetatable(L);

	lua_getfield(L, 1, "type");

	string type = lua_tostring(L, -1);
	if (type.find("[") != string::npos)
	{
		type = type.substr(0, type.find("[") - 1);
		lua_pop(L, 1);
		lua_pushstring(L, type.c_str());
	}

	lua_setfield(L, -2, "type");

	SetupMetatable<T>(L);
}

template<typename T> typename enable_if<is_std_tuple<T>::value>::type LuaPushValue(lua_State* L, T src)
{
	LuaPushTuple(L, src);
}

template<typename T> typename enable_if<is_std_vector<T>::value>::type LuaPushValue(lua_State* L, T src)
{
	lua_autogetvector(L, src);
}

template<> void LuaPushValue<bool>(lua_State* L, bool src);
template<> void LuaPushValue<char>(lua_State* L, char src);
template<> void LuaPushValue<char*>(lua_State* L, char* src);
template<> void LuaPushValue<const char*>(lua_State* L, const char* src);
template<> void LuaPushValue<short>(lua_State* L, short src);
template<> void LuaPushValue<unsigned short>(lua_State* L, unsigned short src);
template<> void LuaPushValue<int>(lua_State* L, int src);
template<> void LuaPushValue<unsigned int>(lua_State* L, unsigned int src);
template<> void LuaPushValue<long>(lua_State* L, long src);
template<> void LuaPushValue<unsigned long>(lua_State* L, unsigned long src);
template<> void LuaPushValue<long long>(lua_State* L, long long src);
template<> void LuaPushValue<unsigned long long>(lua_State* L, unsigned long long src);
template<> void LuaPushValue<float>(lua_State* L, float src);
template<> void LuaPushValue<double>(lua_State* L, double src);
template<> void LuaPushValue<string>(lua_State* L, string src);

template<typename T> int lua_autogetvector(lua_State* L, vector<T> src, int type = -1, string errmsg = "Bad getter vector")
{
	lua_newtable(L);

	for (unsigned int i = 0; i < src.size(); i++)
	{
		LuaPushValue<T>(L, src.at(i));
		lua_seti(L, -2, i + 1);
	}

	return 1;
}

template<typename T> int lua_autogetvector(lua_State* L, vector<vector<T>> src, int type = -1, string errmsg = "Bad getter vector")
{
	lua_newtable(L);

	for (unsigned int i = 0; i < src.size(); i++)
	{
		lua_newtable(L);

		for (unsigned int j = 0; j < src.at(i).size(); j++)
		{
			LuaPushValue<T>(L, src.at(i).at(j));
			lua_seti(L, -2, j + 1);
		}

		lua_seti(L, -2, i + 1);
	}

	return 1;
}

template<typename T> int lua_autogetarray(lua_State* L, T* src, unsigned int size, int type = -1, string errmsg = "Bad getter array")
{
	lua_newtable(L);

	for (unsigned int i = 0; i < size; i++)
	{
		LuaPushValue<T>(L, src[i]);
		lua_seti(L, -2, i + 1);
	}

	return 1;
}

// --------------------------------------------------------------------------------------------------------- //
// ----------------------- Multi-return/push from Lua and function call ------------------------------------ //
// --------------------------------------------------------------------------------------------------------- //

extern map<string, function<int()>> methodList;
extern map<string, map<int, function<int(lua_State*, int, int)>>> constructorList;

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
	LuaPushValue<T>(L, val);
}

template<typename First, typename ... Rest> void LuaMultPush(lua_State* L, First fst, Rest ... rest)
{
	LuaPushValue<First>(L, fst);
	LuaMultPush<Rest...>(L, rest...);
}

template<typename ... Ts> void LuaPushTuple(lua_State* L, tuple<Ts...> src)
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
	static typename enable_if<!is_pointer<T>::value, tuple<T>>::type LuaStackToTuple(lua_State* L, const int index)
	{
		T stack_element = T();
		if (lua_type(L, index) != LUA_TNIL)
		{
			if (is_std_vector<T>::value) lua_autosetvector(L, &stack_element, -1, index);
			else LuaPopValue<T>(L, &stack_element, index);
		}
		return make_tuple(stack_element);
	}

	template<typename T>
	static typename enable_if<is_pointer<T>::value, tuple<T>>::type LuaStackToTuple(lua_State* L, const int index)
	{
		T stack_element = nullptr;
		if (lua_type(L, index) != LUA_TNIL)
		{
			stack_element = *(static_cast<T*>(lua_touserdata(L, index)));
			lua_remove(L, index);
		}
		return make_tuple(stack_element);
	}

	// inductive case
	template<typename T1, typename T2, typename ... Rest>
	static typename enable_if<!is_pointer<T1>::value, tuple<T1, T2, Rest...>>::type LuaStackToTuple(lua_State* L, const int index)
	{
		T1 stack_element = T1();
		if (lua_type(L, index) != LUA_TNIL)
		{
			if (is_std_vector<T1>::value) lua_autosetvector(L, &stack_element, -1, index);
			else LuaPopValue<T1>(L, &stack_element, index);
		}
		tuple<T1> head = make_tuple(stack_element);
		return tuple_cat(head, LuaStackToTuple<T2, Rest...>(L, index));
	}

	// inductive case
	template<typename T1, typename T2, typename ... Rest>
	static typename enable_if<is_pointer<T1>::value, tuple<T1, T2, Rest...>>::type LuaStackToTuple(lua_State* L, const int index)
	{
		T1 stack_element = nullptr;
		if (lua_type(L, index) != LUA_TNIL)
		{
			stack_element = *(static_cast<T1*>(lua_touserdata(L, index)));
			lua_remove(L, index);
		}
		tuple<T1> head = make_tuple(stack_element);
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
	auto ctor = [=](lua_State* L_, int index, int array_size)
	{
		if(array_size == 0) NewUserData<T>(L_);
		else NewUserDataArray<T>(L_, array_size);

		MakeMetatable ( L_ );

		lua_pushstring(L_, name.c_str());
		lua_setfield(L_, -2, "type");

		SetupMetatable<T> ( L_ );

		return sizeof(T);
	};

	constructorList[name][0] = ctor;

	lua_getglobal(L, "MakeEasyConstructors");
	lua_pushstring(L, name.c_str());
	lua_pcall(L, 1, 0, 0);
}

template<typename T, typename ... Args> void AddObjectConstructor(lua_State* L, string name)
{
	auto ctor = [=](lua_State* L_, int index, int array_size)
	{
		auto args = LuaPopHelper<sizeof...(Args), Args...>::DoPop(L_, index);

		T** obj;

		if(array_size > 0) obj = NewUserDataArray<T>(L_, array_size);
		else
		{
			auto final_args = tuple_cat(make_tuple(L_), args);
			StoreArgsAndCallFn<sizeof...(Args), T**, lua_State*, Args...> retrieved =
			{	NewUserData<T>, final_args};

			obj = reinterpret_cast<T**>(lua_newuserdata(L_, sizeof(T*)));
			obj = retrieved.DoCallFn();
		}

		MakeMetatable ( L_ );

		lua_pushstring(L_, name.c_str());
		lua_setfield(L_, -2, "type");

		SetupMetatable<T> ( L_ );

		return sizeof(T);
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

	int type_size = (constructorList[classname][nargs])(L, index, arraySize);

	lua_pushinteger(L, type_size * (arraySize > 0 ? arraySize : 1));
	lua_setfield(L, -2, "sizeof");

	if (findIfArray != string::npos)
	{
		lua_pushinteger(L, arraySize);
		lua_setfield(L, -2, "array_size");

		lua_getfield(L, -1, "Reset");
		lua_pushvalue(L, -2);
		lua_pcall(L, 1, 0, 0);
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
extern map<string, function<void(lua_State*, char*)>> setUserDataFns;
extern map<string, function<void(lua_State*, char*)>> getUserDataFns;
extern map<string, function<void(lua_State*, char*)>> assignUserDataFns;

extern map<string, int> userDataSizes;

int luaExt_GetUserDataSize(lua_State* L);

template<typename T> int StandardSetterFn(lua_State* L)
{
	T* ud = GetUserData<T>(L, 1, "setUserDataFns");
	if (lua_type(L, 2) == LUA_TNIL) *ud = T();
	else LuaPopValue<T>(L, ud);
	return 0;
}

template<typename T> int StandardGetterFn(lua_State* L)
{
	T* ud = GetUserData<T>(L, 1, "getUserDataFns");
	LuaPushValue<T>(L, *ud);
	return 1;
}

template<typename T> int VectorSetterFn(lua_State* L)
{
	vector<T>* ud = GetUserData<vector<T>>(L, 1, "getUserDataFns");
	if (lua_type(L, 2) == LUA_TNIL) ud->clear();
	else lua_autosetvector(L, ud);
	return 0;
}

template<typename T> int VectorPushBackFn(lua_State* L)
{
	vector<T>* ud = GetUserData<vector<T>>(L, 1, "getUserDataFns");
	lua_autopushback(L, ud);
	return 0;
}

template<typename T> int VectorGetterFn(lua_State* L)
{
	vector<T>* ud = GetUserData<vector<T>>(L, 1, "getUserDataFns");
	int index = lua_tointegerx(L, 2, nullptr);

	if (index >= 1) LuaPushValue<T>(L, ud->at(index - 1));
	else lua_autogetvector(L, *ud);

	return 1;
}

template<typename T> int DoubleVectorSetterFn(lua_State* L)
{
	vector<vector<T>>* ud = GetUserData<vector<vector<T>>>(L, 1, "getUserDataFns");
	if (lua_type(L, 2) == LUA_TNIL) ud->clear();
	if (lua_type(L, 2) == LUA_TNUMBER && lua_type(L, 3) == LUA_TNIL) ud->at(lua_tointeger(L, 2)).clear();
	else lua_autosetvector(L, ud);
	return 0;
}

template<typename T> int DoubleVectorPushBackFn(lua_State* L)
{
	vector<vector<T>>* ud = GetUserData<vector<vector<T>>>(L, 1, "getUserDataFns");
	lua_autopushback(L, ud);
	return 0;
}

template<typename T> int DoubleVectorGetterFn(lua_State* L)
{
	vector<vector<T>>* ud = GetUserData<vector<vector<T>>>(L, 1, "getUserDataFns");
	int index = lua_tointegerx(L, 2, nullptr);
	int subindex = lua_tointegerx(L, 3, nullptr);

	if (index >= 1)
	{
		if (subindex >= 1) LuaPushValue<T>(L, ud->at(index - 1).at(subindex - 1));
		else lua_autogetvector(L, ud->at(index - 1));
	}
	else lua_autogetvector(L, *ud);

	return 1;
}

template<typename T> int ArraySetterFn(lua_State* L)
{
	T* ud = GetUserData<T>(L, 1, "getUserDataFns");

	lua_getfield(L, 1, "array_size");
	int array_size = lua_tointeger(L, -1);
	lua_pop(L, 1);

	if (lua_type(L, 2) == LUA_TNIL)
	{
		for (int i = 0; i < array_size; i++)
			ud[i] = T();

		return 0;
	}

	lua_autosetarray(L, ud, array_size);
	return 0;
}

template<> int ArraySetterFn<char>(lua_State* L);

template<typename T> int ArrayGetterFn(lua_State* L)
{
	T* ud = GetUserData<T>(L, 1, "getUserDataFns");
	int index = lua_tointegerx(L, 2, nullptr);

	if (index >= 1) LuaPushValue<T>(L, ud[index - 1]);

	else
	{
		lua_getfield(L, 1, "array_size");
		int array_size = lua_tointeger(L, -1);
		lua_pop(L, 1);
		lua_autogetarray(L, ud, array_size);
	}

	return 1;
}

template<> int ArrayGetterFn<char>(lua_State* L);

template<typename T> void MakeAccessorsUserDataFuncs(lua_State* _lstate, string type)
{
	string finalType = type;

	userDataSizes[type] = sizeof(T);
	userDataSizes[type + "[]"] = sizeof(T);

	MakeDefaultConstructor<T>(_lstate, finalType);
	AddObjectConstructor<T, T>(_lstate, finalType);

	// Standard Setter ----------------------------------------------------------------
	setUserDataFns[finalType] = [=] ( lua_State* L, char* address)
	{
		T* ud;
		if(address == nullptr) ud = GetUserData<T> ( L, 1, "setUserDataFns" );
		else ud = (T*) address;
		LuaPopValue<T>(L, ud);
	};

	RegisterMethodInTable(_lstate, StandardSetterFn<T>, finalType.c_str(), "_setterfns");

	// Standard Getter ----------------------------------------------------------------
	getUserDataFns[finalType] = [=] ( lua_State* L, char* address)
	{
		T* ud;
		if(address == nullptr) ud = GetUserData<T> ( L, 1, "getUserDataFns" );
		else ud = (T*) address;
		LuaPushValue<T>(L, *ud);
	};

	RegisterMethodInTable(_lstate, StandardGetterFn<T>, finalType.c_str(), "_getterfns");

	// Standard Assign ----------------------------------------------------------------
	assignUserDataFns[finalType] = [=] (lua_State* L, char* addr)
	{
		T** ud = GetUserDataPtr<T> ( L, 1, "assignUserDataFns" );
		*ud = (T*) addr;
	};

	finalType = "vector<" + type + ">";

	MakeDefaultConstructor<vector<T>>(_lstate, finalType);
	AddObjectConstructor<vector<T>, vector<T>>(_lstate, finalType);

	// Vector Setter ----------------------------------------------------------------
	setUserDataFns[finalType] = [=] ( lua_State* L, char* address)
	{
		vector<T>* ud;
		if(address == nullptr) ud = GetUserData<vector<T>> ( L, 1, "setUserDataFns" );
		else ud = (vector<T>*) address;
		lua_autosetvector ( L, ud, -1 );
	};

	RegisterMethodInTable(_lstate, VectorSetterFn<T>, finalType.c_str(), "_setterfns");
	RegisterMethodInTable(_lstate, VectorPushBackFn<T>, finalType.c_str(), "_pushbackfns");

	// Vector Getters ----------------------------------------------------------------
	getUserDataFns[finalType] = [=] ( lua_State* L, char* address )
	{
		vector<T>* ud;
		if(address == nullptr) ud = GetUserData<vector<T>> ( L, 1, "getUserDataFns" );
		else ud = (vector<T>*) address;
		lua_autogetvector ( L, *ud, -1 );
	};

	RegisterMethodInTable(_lstate, VectorGetterFn<T>, finalType.c_str(), "_getterfns");

	// Vector Assign ----------------------------------------------------------------
	assignUserDataFns[finalType] = [=] (lua_State* L, char* addr)
	{
		vector<T>** ud = GetUserDataPtr<vector<T>> ( L, 1, "assignUserDataFns" );
		*ud = (vector<T>*) addr;
	};

	finalType = "vector<vector<" + type + ">>";

	MakeDefaultConstructor<vector<vector<T>>>(_lstate, finalType);

	// Vector Vector Setters ----------------------------------------------------------------
	setUserDataFns[finalType] = [=] ( lua_State* L, char* address )
	{
		vector<vector<T>>* ud;
		if(address == nullptr) ud = GetUserData<vector<vector<T>>> ( L, 1, "setUserDataFns" );
		else ud = (vector<vector<T>>*) address;
		lua_autosetvector ( L, ud, -1 );
	};

	RegisterMethodInTable(_lstate, DoubleVectorSetterFn<T>, finalType.c_str(), "_setterfns");
	RegisterMethodInTable(_lstate, DoubleVectorPushBackFn<T>, finalType.c_str(), "_pushbackfns");

	// Vector Vector Getters ----------------------------------------------------------------
	getUserDataFns[finalType] = [=] ( lua_State* L, char* address )
	{
		vector<vector<T>>* ud;
		if(address == nullptr) ud = GetUserData<vector<vector<T>>> ( L, 1, "getUserDataFns" );
		else ud = (vector<vector<T>>*) address;
		lua_autogetvector ( L, *ud, -1 );
	};

	RegisterMethodInTable(_lstate, DoubleVectorGetterFn<T>, finalType.c_str(), "_getterfns");

	// Vector Vector Assign ----------------------------------------------------------------
	assignUserDataFns[finalType] = [=] (lua_State* L, char* addr)
	{
		vector<vector<T>>** ud = GetUserDataPtr<vector<vector<T>>> ( L, 1, "assignUserDataFns" );
		*ud = (vector<vector<T>>*) addr;
	};

	finalType = type + "[]";

	MakeDefaultConstructor<T>(_lstate, finalType);
	AddObjectConstructor<T, T>(_lstate, finalType);

	// Array Setters ----------------------------------------------------------------
	setUserDataFns[finalType] = [=] ( lua_State* L, char* address )
	{
		T* ud;
		if(address == nullptr)
		{
			ud = GetUserData<T> ( L, 1, "setUserDataFns" );
			lua_getfield ( L, 1, "array_size" );
		}
		else ud = (T*) address;
		int array_size = lua_tointeger ( L, -1 );
		lua_pop ( L, 1 );
		lua_autosetarray ( L, ud, array_size, -1 );
	};

	RegisterMethodInTable(_lstate, ArraySetterFn<T>, finalType.c_str(), "_setterfns");

	// Array Getters ----------------------------------------------------------------
	getUserDataFns[finalType] = [=] ( lua_State* L, char* address )
	{
		T* ud;
		if(address == nullptr)
		{
			ud = GetUserData<T> ( L, 1, "getUserDataFns" );
			lua_getfield ( L, 1, "array_size" );
		}
		else ud = (T*) address;
		int array_size = lua_tointeger ( L, -1 );
		lua_pop ( L, 1 );
		lua_autogetarray ( L, ud, array_size, -1 );
	};

	RegisterMethodInTable(_lstate, ArrayGetterFn<T>, finalType.c_str(), "_getterfns");

	// Array Assign ----------------------------------------------------------------
	assignUserDataFns[finalType] = [=] (lua_State* L, char* addr)
	{
		T** ud = GetUserDataPtr<T> ( L, 1, "assignUserDataFns" );
		*ud = (T*) addr;
	};
}

int StrLength(lua_State* L);

void MakeStringAccessor(lua_State* L);

int SetMemoryBlock(lua_State* L, char* address);
void GetMemoryBlock(lua_State* L, char* address);

int luaExt_SetMemoryBlock(lua_State* L);
int luaExt_GetMemryBlock(lua_State* L);

#endif
