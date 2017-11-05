#ifndef __USERCLASSBASE__
#define __USERCLASSBASE__

#include "LuaRootBinder.h"
#include "TObject.h"

extern map<string, function<void()>> methodList;

template<int...> struct seq
{};

template<int N, int ...S> struct gens: gens<N - 1, N - 1, S...> {
};

template<int ...S> struct gens<0, S...> {
	typedef seq<S...> type;
};

template<size_t, typename T, typename R, typename ... Args>
struct StoreArgsAndCallFn {
	T* obj;
	R (T::*func)(Args...);
	std::tuple<Args...> params;

	R DoCall()
	{
		return CallFn(typename gens<sizeof...(Args)>::type());
	}

	template<int ...S>
	R CallFn(seq<S...>)
	{
		return (obj->*func)(std::get<S>(params) ...);
	}
};

template<typename T, typename R, typename ... Args>
struct StoreArgsAndCallFn<0, T, R, Args...> {
	T* obj;
	R (T::*func)(Args...);
	void* params;

	R DoCall()
	{
		return (obj->*func)();
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

	static std::tuple<Ts...> DoPop(lua_State* L)
	{
		auto ret = LuaStackToTuple<Ts...>(L, 2);
		return ret;
	}
};

template<typename ... Ts>
struct LuaPopHelper<0, Ts...> {
	typedef void* type;

	static void* DoPop(lua_State* L)
	{
		return nullptr;
	}
};

template<typename T, typename R, typename ... Args> typename LuaPopHelper<sizeof...(Args), Args...>::type LuaMultPop(lua_State* L, R (T::*method)(Args...))
{
	return LuaPopHelper<sizeof...(Args), Args...>::DoPop(L);
}

template<typename T, typename R, typename ... Args> void AddClassMethod(lua_State* L, R (T::*method)(Args...), string name)
{
	T* obj_base = GetUserData<T>(L, -2);
	obj_base->methods.push_back(name);

	methodList[name] = [=]()
	{
		T* obj = GetUserData<T>(L, 1);

		auto args = LuaMultPop(L, method);

		StoreArgsAndCallFn<sizeof...(Args), T, R, Args...> retrieved =
		{	obj, method, args};
		retrieved.DoCall();
		return;
	};
}

class LuaUserClass: public TObject {
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
	virtual void MakeAccessors(lua_State* L) = 0;
	virtual void Reset() = 0;

	template<typename T> void AddAccessor(lua_State* L, T* member, string name, string type)
	{
		lua_pushstring(L, type.c_str());
		lua_insert(L, 1);
		luaExt_NewUserData(L);

		T** ud = GetUserDataPtr<T>(L, -1);
		*ud = member;

		lua_setfield(L, -2, name.c_str());
	}

ClassDef ( LuaUserClass, 1 )
};

int GetMember(lua_State* L);
int SetMember(lua_State* L);
int GetMemberValue(lua_State* L);
int ResetValues(lua_State* L);
int CallMethod(lua_State* L);

template<typename T> void MakeAccessFunctions(lua_State* L, string type_name)
{
	lua_getglobal(L, "MakeCppClassCtor");
	lua_pushstring(L, type_name.c_str());
	lua_pcall(L, 1, 1, 0);

	MakeAccessorsUserDataFuncs<T>(type_name);

	newBranchFns[type_name] = [=] ( lua_State* L_, TTree* tree, const char* bname )
	{
		luaExt_NewUserData ( L_ );
		T* branch_ptr = GetUserData<T> ( L_, -1, "luaExt_TTree_NewBranch" );
		tree->Branch ( bname, branch_ptr );
	};

	newBranchFns["vector<" + type_name + ">"] = [=] ( lua_State* L_, TTree* tree, const char* bname )
	{
		luaExt_NewUserData ( L_ );
		vector<T>* branch_ptr = GetUserData<vector<T>> ( L_, -1, "luaExt_TTree_NewBranch" );
		tree->Branch ( bname, branch_ptr );
	};
}

#endif

