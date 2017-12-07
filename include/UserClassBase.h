#ifndef __USERCLASSBASE__
#define __USERCLASSBASE__

#include "LuaRootBinder.h"
#include "TObject.h"

class LuaUserClass {
private:

protected:

public:
	LuaUserClass()
	{
	}
	virtual ~LuaUserClass()
	{
	}

	vector<string> methods;

	void SetupMetatable(lua_State* L);

	virtual void MakeAccessors(lua_State* L)
	{
	}

	virtual void AddNonClassMethods(lua_State* L)
	{
	}

	virtual void Reset()
	{
	}

	template<typename T> void AddAccessor(lua_State* L, T* member, string name, string type)
	{
		lua_pushstring(L, type.c_str());
		LuaCtor(L, -1);
		lua_remove(L, -2);

		T** ud = GetUserDataPtr<T>(L, -1);
		*ud = member;

		lua_setfield(L, -2, name.c_str());
	}

	template<typename T, typename R, typename ... Args> typename enable_if<!is_same<R, void>::value, void>::type AddClassMethod(lua_State* L, R (T::*method)(Args...), string name)
	{
		lua_getfield(L, -2, "type");
		string luaClassName = lua_tostring(L, -1);
		lua_pop(L, 1);

		LuaUserClass* obj_base = GetUserData<LuaUserClass>(L, -2);
		obj_base->methods.push_back(name);

		methodList[luaClassName + "::" + name] = [=]()
		{
			T* obj = GetUserData<T>(L, 1);

			auto args = LuaMultPop(L, 1, method);

			LuaMemFuncCaller<T, R, Args...> func;
			func.mfptr = method;

			StoreArgsAndCallMemberFn<sizeof...(Args), T, R, Args...> retrieved =
			{	func, args, obj};
			LuaPushValue<R>(L, retrieved.DoCallMemFn());
			return 1;
		};
	}

	template<typename T, typename R, typename ... Args> typename enable_if<is_same<R, void>::value, void>::type AddClassMethod(lua_State* L, R (T::*method)(Args...), string name)
	{
		lua_getfield(L, -2, "type");
		string luaClassName = lua_tostring(L, -1);
		lua_pop(L, 1);

		LuaUserClass* obj_base = GetUserData<LuaUserClass>(L, -2);
		obj_base->methods.push_back(name);

		methodList[luaClassName + "::" + name] = [=]()
		{
			T* obj = GetUserData<T>(L, 1);

			auto args = LuaMultPop(L, 1, method);

			LuaMemFuncCaller<T, R, Args...> func;
			func.mfptr = method;

			StoreArgsAndCallMemberFn<sizeof...(Args), T, R, Args...> retrieved =
			{	func, args, obj};
			retrieved.DoCallMemFn();
			return 0;
		};
	}

	template<typename T, typename R, typename ... Args> typename enable_if<!is_same<R, void>::value, void>::type AddClassMethod(lua_State* L, R (T::*method)(Args...) const,
			string name)
	{
		lua_getfield(L, -2, "type");
		string luaClassName = lua_tostring(L, -1);
		lua_pop(L, 1);

		LuaUserClass* obj_base = GetUserData<LuaUserClass>(L, -2);
		obj_base->methods.push_back(name);

		methodList[luaClassName + "::" + name] = [=]()
		{
			T* obj = GetUserData<T>(L, 1);

			auto args = LuaMultPop(L, 1, method);

			LuaMemFuncCaller<T, R, Args...> func;
			func.const_mfptr = method;

			StoreArgsAndCallMemberFn<sizeof...(Args), T, R, Args...> retrieved =
			{	func, args, obj};
			LuaPushValue<R>(L, retrieved.DoCallMemFnConst());
			return 1;
		};
	}

	template<typename T, typename R, typename ... Args> typename enable_if<is_same<R, void>::value, void>::type AddClassMethod(lua_State* L, R (T::*method)(Args...) const,
			string name)
	{
		lua_getfield(L, -2, "type");
		string luaClassName = lua_tostring(L, -1);
		lua_pop(L, 1);

		LuaUserClass* obj_base = GetUserData<LuaUserClass>(L, -2);
		obj_base->methods.push_back(name);

		methodList[luaClassName + "::" + name] = [=]()
		{
			T* obj = GetUserData<T>(L, 1);

			auto args = LuaMultPop(L, 1, method);

			LuaMemFuncCaller<T, R, Args...> func;
			func.const_mfptr = method;

			StoreArgsAndCallMemberFn<sizeof...(Args), T, R, Args...> retrieved =
			{	func, args, obj};
			retrieved.DoCallMemFnConst();
			return 0;
		};
	}
};

int CallMethod(lua_State* L);

template<typename T> void MakeAccessFunctions(lua_State* L, string type_name)
{
	lua_getglobal(L, "MakeCppClassCtor");
	lua_pushstring(L, type_name.c_str());
	lua_pcall(L, 1, 1, 0);

	MakeAccessorsUserDataFuncs<T>(L, type_name);

	newBranchFns[type_name] = [=] ( lua_State* L_, TTree* tree, const char* bname )
	{
		LuaCtor(L, -1);
		lua_remove(L, -2);
		T* branch_ptr = GetUserData<T> ( L_, -1, "luaExt_TTree_NewBranch" );
		tree->Branch ( bname, branch_ptr );
	};

	newBranchFns["vector<" + type_name + ">"] = [=] ( lua_State* L_, TTree* tree, const char* bname )
	{
		LuaCtor(L, -1);
		lua_remove(L, -2);
		vector<T>* branch_ptr = GetUserData<vector<T>> ( L_, -1, "luaExt_TTree_NewBranch" );
		tree->Branch ( bname, branch_ptr );
	};
}

#endif

