#include "LuaExtension.h"
#include <llex.h>

#undef lua_saveline
#define lua_saveline(L,line) ((void)L, add_history(line), srtysjghs, appendtohist())

lua_State* lua = 0;

map<string, function<int()>> methodList;
map<string, map<int, function<int(lua_State*, int, int)>>> constructorList;

lua_State* InitLuaEnv()
{
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	lua_register(lua, "ls", LuaListDirContent);

	string lua_path = "";

	lua_path += "?;?.lua";

	lua_getglobal(lua, "package");
	lua_getfield(lua, -1, "path");

	if (lua_type(lua, -1) == LUA_TSTRING) lua_path = (string) lua_tostring(lua, -1) + ";" + lua_path;
	lua_pop(lua, 1);

	lua_pushstring(lua, lua_path.c_str());

	lua_setfield(lua, -2, "path");
	lua_pop(lua, 1);

	cout << "Lua Env initialized..." << endl;

	return L;
}

int appendtohist(lua_State* L)
{
	append_history(1, histPath);
	return 0;
}

int trunctehistoryfile(lua_State* L)
{
	if (lua_type(L, 1) != LUA_TNUMBER) return 0;

	int maxHistLength = lua_tointeger(L, 1);

	history_truncate_file(histPath, maxHistLength);
	return 0;
}

int saveprompthistory(lua_State* L)
{
	HIST_ENTRY* hentry = current_history();
	if (hentry != nullptr && (((string) hentry->line) == "q()" || ((string) hentry->line) == "exit()")) remove_history(where_history());

	write_history(histPath);
	trunctehistoryfile(L);
	return 0;
}

int clearprompthistory(lua_State* L)
{
	clear_history();
	history_truncate_file(histPath, 0);
	return 0;
}

int luaExt_gettime(lua_State* L)
{
	int clock_id = lua_tonumberx(L, 1, nullptr);
	if (clock_id == 0) clock_id = CLOCK_REALTIME;

	timespec ts;
	clock_gettime(clock_id, &ts);

	lua_pushinteger(L, ts.tv_sec);
	lua_pushinteger(L, ts.tv_nsec);

	return 2;
}

int luaExt_nanosleep(lua_State* L)
{
	timespec ts;
	double timer = lua_tonumber(L, 1);

	ts.tv_sec = floor(timer);
	ts.tv_nsec = (timer - ts.tv_sec) * 1e9;

	nanosleep(&ts, nullptr);

	return 0;
}

string GetLuaTypename(int type_)
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

void DumpLuaStack(lua_State* L)
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

void MakeMetatable(lua_State* L)
{
	lua_newtable(L);

	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__newindex");

	lua_setmetatable(L, -2);
}

void AddMethod(lua_State* L, lua_CFunction func, const char* funcname)
{
	lua_pushcfunction(L, func);
	lua_setfield(L, -2, funcname);
}

void RegisterMethodInTable(lua_State* L, lua_CFunction func, const char* funcname, const char* dest_table)
{
	lua_getglobal(L, dest_table);
	lua_pushcfunction(L, func);
	lua_setfield(L, -2, funcname);
}

bool lua_unpackarguments(lua_State* L, int index, string errmsg, vector<const char*> arg_names, vector<int> arg_types, vector<bool> arg_required)
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
		if (arg_required[i] && !CheckLuaArgs(L, -1, true, errmsg, arg_types[i]))
		{
			cerr << "Missing or invalid field: " << arg_names[i] << endl;
			return false;
		}
	}

	return true;
}

bool CheckLuaArgs(lua_State* L, int argIdx, bool abortIfError, string funcName)
{
	(void) L;
	(void) argIdx;
	(void) abortIfError;
	(void) funcName;
	return true;
}

bool CheckLuaArgs(lua_State* L, int argIdx, bool abortIfError, string funcName, int arg)
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

void TryGetGlobalField(lua_State *L, string gfield)
{
	if (gfield.empty())
	{
		lua_pushnil(L);
		return;
	}

	lua_getglobal(L, "_G");

	if (lua_type(L, -1) == LUA_TNIL)
	{
		cerr << "Issue while retrieving global environment... aborting..." << endl;
		lua_pop(L, 1);
		return;
	}

	vector<string> chain;

	size_t sepPos = gfield.find_first_of(".");

	if (sepPos == string::npos) chain.push_back(gfield);
	else
	{
		chain.push_back(gfield.substr(0, sepPos));
		size_t nextSepPos = gfield.find_first_of(".", sepPos + 1);

		while (nextSepPos != string::npos)
		{
			chain.push_back(gfield.substr(sepPos + 1, nextSepPos - sepPos - 1));
			sepPos = nextSepPos;
			nextSepPos = gfield.find_first_of(".", sepPos + 1);
		}

		chain.push_back(gfield.substr(sepPos + 1));
	}

	for (unsigned int i = 0; i < chain.size(); i++)
	{
//         cout << "Getting field: " << chain[i] << endl;
		lua_getfield(L, -1, chain[i].c_str());
		lua_remove(L, -2);
		if (lua_type(L, -1) == LUA_TNIL)
		{
			return;
		}
//         cout << "Retrieved field: " << chain[i] << endl;
	}
}

bool TrySetGlobalField(lua_State *L, string gfield)
{
	if (gfield.empty()) return false;

	lua_getglobal(L, "_G");

	if (lua_type(L, -1) == LUA_TNIL)
	{
		cerr << "Issue while retrieving global environment... aborting..." << endl;
		lua_pop(L, 1);
		return false;
	}

	vector<string> chain;

	size_t sepPos = gfield.find_first_of(".");

	if (sepPos == string::npos) chain.push_back(gfield);
	else
	{
		chain.push_back(gfield.substr(0, sepPos));
		size_t nextSepPos = gfield.find_first_of(".", sepPos + 1);

		while (nextSepPos != string::npos)
		{
			chain.push_back(gfield.substr(sepPos + 1, nextSepPos - sepPos - 1));
			sepPos = nextSepPos;
			nextSepPos = gfield.find_first_of(".", sepPos + 1);
		}

		chain.push_back(gfield.substr(sepPos + 1));
	}

	for (unsigned int i = 0; i < chain.size() - 1; i++)
	{
		lua_getfield(L, -1, chain[i].c_str());

		if (lua_type(L, -1) == LUA_TNIL)
		{
			lua_pop(L, 1);
//             cout << "TrySetGlobalField: field " << chain[i] << " does not exists. Creating a table for it..." << endl;
			lua_newtable(L);
			lua_pushvalue(L, -1);
			lua_setfield(L, -3, chain[i].c_str());
		}
		else if (lua_type(L, -1) != LUA_TTABLE)
		{
			cerr << "attempt to assign a field to " << chain[i] << ": a " << lua_typename(L, lua_type(L, -1)) << endl;
			lua_pop(L, 1);
			return false;
		}

		lua_remove(L, -2);
	}

	lua_insert(L, -2);
//     cout << "TrySetGlobalField: Setting last field " << chain.back() << endl;
	lua_setfield(L, -2, chain.back().c_str());
	lua_pop(L, 1);

	return true;
}

void DoForEach(lua_State *L, int index, function<bool(lua_State *L_)> dofn)
{
	if (index < 0) index = lua_gettop(L) + index + 1;

	if (lua_type(L, index) == LUA_TTABLE)
	{
		lua_pushnil(L);

		while (lua_next(L, index) != 0)
		{
			bool stop = dofn(L);

			lua_pop(L, 1);

			if (stop)
			{
				lua_pop(L, 1);
				return;
			}
		}
	}
	else
	{
		dofn(L);
		return;
	}
}

int LuaListDirContent(lua_State* L)
{
	DIR* dir;
	dirent* dentry;
	int i;

	if (lua_type(L, 1) != LUA_TSTRING)
	{
		lua_pushnil(L);
		lua_pushstring(L, "Invalid argument");
		return 2;
	}

	const char* path = lua_tostring(L, 1);

	dir = opendir(path);

	if (dir == nullptr)
	{
		lua_pushnil(L);
		lua_pushstring(L, "no such directory");
		return 2;
	}

	lua_newtable(L);
	i = 1;

	while ((dentry = readdir(dir)) != nullptr)
	{
		string dtype;

		struct stat fstat;

		stat(dentry->d_name, &fstat);

		switch (fstat.st_mode & S_IFMT)
		{
			case S_IFDIR:
				dtype = "dir";
				break;
			case S_IFREG:
				dtype = "file";
				break;
			default:
				dtype = "undetermined";
				break;
		}

		if (dtype != "undetermined")
		{
			lua_pushnumber(L, i);

			lua_newtable(L);

			lua_pushstring(L, dentry->d_name);
			lua_setfield(L, -2, "name");

			lua_pushstring(L, dtype.c_str());
			lua_setfield(L, -2, "type");

			lua_settable(L, -3);

			// 	    cout << "pushed: [" << i << "] = { type = " << dtype.c_str() << " , name = " << dentry->d_name << " }" << endl;

			i++;
		}
	}

	closedir(dir);

	return 1;
}

map<string, function<void(lua_State*, char*)>> setUserDataFns;
map<string, function<void(lua_State*, char*)>> getUserDataFns;
map<string, function<void(lua_State*)>> newUserDataFns;
map<string, function<void(lua_State*, char*)>> assignUserDataFns;

map<string, int> userDataSizes;

int luaExt_GetUserDataSize(lua_State* L)
{
	string type = lua_tostring(L, 1);

	lua_pushinteger(L, userDataSizes[type]);

	return 1;
}

int luaExt_SetUserDataValue(lua_State* L)
{
	lua_getfield(L, 1, "type");
	string ud_type = lua_tostring(L, -1);
	lua_pop(L, 1);

	setUserDataFns[ud_type](L, nullptr);

	return 0;
}

int luaExt_PushBackUserDataValue(lua_State* L)
{
	if (lua_type(L, 2) == LUA_TNIL)
	{
		cerr << "Cannot push_back nil..." << endl;

		return 0;
	}

	return luaExt_SetUserDataValue(L);
}

int luaExt_GetUserDataValue(lua_State* L)
{
	int index = lua_tointegerx(L, 2, nullptr);

	lua_getfield(L, 1, "type");
	string ud_type = lua_tostring(L, -1);
	lua_pop(L, 1);

	getUserDataFns[ud_type](L, nullptr);

	if (index > 0)
	{
		lua_geti(L, -1, index);
	}

	return 1;
}

inline int StrLength(lua_State* L)
{
	string* str = GetUserData<string>(L);

	lua_pushinteger(L, str->length());

	return 1;
}

void MakeStringAccessor(lua_State* L)
{
	userDataSizes["cstring"] = sizeof(string);

	auto ctor = [=](lua_State* L_, int index, int array_size)
	{
		NewUserData<string>(L_);

		MakeMetatable ( L_ );

		lua_pushstring(L, "cstring");
		lua_setfield(L_, -2, "type");

		SetupMetatable<string> ( L_ );

		AddMethod(L_, StrLength, "StrLength");

		return sizeof(string);
	};

	constructorList["cstring"][0] = ctor;

	lua_getglobal(L, "MakeEasyConstructors");
	lua_pushstring(L, "cstring");
	lua_pcall(L, 1, 0, 0);

	setUserDataFns["cstring"] = [=] ( lua_State* L_, char* address)
	{
		string* ud;
		if(address == nullptr)
		{
			ud = GetUserData<string> ( L_, 1, "setUserDataFns" );
			*ud = lua_tostring(L, -1);
		}
		else
		{
			int strlen = lua_rawlen(L, -1);
			*((int*)address) = strlen;
			sprintf(address+sizeof(int), "%s", lua_tostring(L, -1));
		}
	};

	RegisterMethodInTable(L, StandardSetterFn<string>, "cstring", "_setterfns");

	getUserDataFns["cstring"] = [=] ( lua_State* L_, char* address)
	{
		string* ud;
		if(address == nullptr)
		{
			ud = GetUserData<string> ( L_, 1, "setUserDataFns" );
			lua_pushstring(L, (*ud).c_str());
		}
		else
		{
			int strlen = *((int*) address);
			lua_pushlstring(L, address+sizeof(int), strlen);
		}
	};

	RegisterMethodInTable(L, StandardGetterFn<string>, "cstring", "_getterfns");

	assignUserDataFns["cstring"] = [=] (lua_State* L_, char* addr)
	{
		string** ud = GetUserDataPtr<string> ( L_, 1, "assignUserDataFns" );
		*ud = (string*) addr;
	};
}

int SetMemoryBlock(lua_State* L, char* address)
{
	int struct_size = lua_rawlen(L, 1);

	bool toUserData = false;
	int totsize = 0;

	lua_geti(L, 1, 1);

	if (lua_type(L, -1) == LUA_TUSERDATA) toUserData = true;

	lua_pop(L, 1);

	for (int i = 0; i < struct_size; i++)
	{
		lua_geti(L, 1, i + 1);
		string type;
		int effectiveSize;
		if (toUserData)
		{
			lua_getfield(L, -1, "type");
			type = lua_tostring(L, -1);
			lua_pop(L, 1);
			lua_getfield(L, -1, "Get");
			lua_insert(L, -2);
			lua_pcall(L, 1, 1, 0);

			if (type != "string") effectiveSize = userDataSizes[type];
			else effectiveSize = lua_rawlen(L, -1) + sizeof(int);

			int array_size = 1;
			if (lua_type(L, -1) == LUA_TTABLE)
			{
				array_size = lua_rawlen(L, -1);
				effectiveSize *= array_size;
				lua_pushinteger(L, array_size);
			}

			effectiveSize *= array_size;

			setUserDataFns[type](L, address + totsize);
		}
		else
		{
			lua_geti(L, 2, i + 1);
			type = lua_tostring(L, -1);
			lua_pop(L, 1);

			int array_size = 1;
			size_t arrayPos = type.find("[");
			if (arrayPos != string::npos)
			{
				size_t endArrSize = type.find("]", arrayPos + 1);
				if (endArrSize > arrayPos + 1) type = type.substr(0, arrayPos + 1) + "]";
			}

			if (type != "string") effectiveSize = userDataSizes[type];
			else effectiveSize = lua_rawlen(L, -1) + sizeof(int);

			if (lua_type(L, -1) == LUA_TTABLE)
			{
				array_size = lua_rawlen(L, -1);
				effectiveSize *= array_size;
				lua_pushinteger(L, array_size);
			}

			setUserDataFns[type](L, address + totsize);
		}

		totsize += effectiveSize;
	}

	return totsize;
}

void GetMemoryBlock(lua_State* L, char* address)
{
	int struct_size = lua_rawlen(L, 1);

	bool toUserData = false;
	int totsize = 0;

	lua_geti(L, 1, 1);

	if (lua_type(L, -1) == LUA_TUSERDATA)
	{
		lua_pop(L, 1);
		toUserData = true;
	}
	else
	{
		lua_pop(L, 1);
		lua_newtable(L);
	}

	for (int i = 0; i < struct_size; i++)
	{
		lua_geti(L, 1, i + 1);
		string type;
		int effectiveSize;
		if (toUserData)
		{
			lua_getfield(L, -1, "type");
			type = lua_tostring(L, -1);
			lua_pop(L, 1);
			lua_getfield(L, -1, "array_size");
			int array_size = lua_tointegerx(L, -1, nullptr);
			if (lua_type(L, -1) == LUA_TNIL) lua_pop(L, 1);
			getUserDataFns[type](L, address + totsize);

			if (type != "string") effectiveSize = userDataSizes[type];
			else effectiveSize = lua_rawlen(L, -1) + sizeof(int);

			if (array_size > 0) effectiveSize *= array_size;

			lua_getfield(L, -2, "Set");
			lua_insert(L, -3);
			lua_pcall(L, 2, 0, 0);
		}
		else
		{
			type = lua_tostring(L, -1);

			int array_size = 1;
			size_t arrayPos = type.find("[");
			if (arrayPos != string::npos)
			{
				size_t endArrSize = type.find("]");
				array_size = stoi(type.substr(arrayPos + 1, endArrSize - arrayPos - 1));
				lua_pushinteger(L, array_size);
				type = type.substr(0, arrayPos + 1) + "]";
			}

			getUserDataFns[type](L, address + totsize);

			if (type != "string") effectiveSize = userDataSizes[type];
			else effectiveSize = lua_rawlen(L, -1) + sizeof(int);

			effectiveSize *= array_size;

			lua_seti(L, 2, i + 1);
			lua_pop(L, 1);
		}

		totsize += effectiveSize;
	}
}

int luaExt_SetMemoryBlock(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "luaExt_SetMemoryBlock", LUA_TUSERDATA, LUA_TTABLE)) return 0;

	void** memblock = reinterpret_cast<void**>(lua_touserdata(L, 1));
	lua_remove(L, 1);

	SetMemoryBlock(L, (char*) *memblock);

	return 0;
}

int luaExt_GetMemryBlock(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "luaExt_GetMemryBlock", LUA_TUSERDATA, LUA_TTABLE)) return 0;

	void** memblock = reinterpret_cast<void**>(lua_touserdata(L, 1));
	lua_remove(L, 1);

	GetMemoryBlock(L, (char*) *memblock);

	return 1;
}

template<> void LuaPopValue<bool>(lua_State* L, bool& dest, int index)
{
	dest = lua_toboolean(L, index);
	lua_remove(L, index);
}

template<> void LuaPopValue<char>(lua_State* L, char& dest, int index)
{
	dest = lua_tostring(L, index)[0];
	lua_remove(L, index);
}

template<> void LuaPopValue<short>(lua_State* L, short& dest, int index)
{
	dest = lua_tointeger(L, index);
	lua_remove(L, index);
}

template<> void LuaPopValue<unsigned short>(lua_State* L, unsigned short& dest, int index)
{
	dest = lua_tointeger(L, index);
	lua_remove(L, index);
}

template<> void LuaPopValue<int>(lua_State* L, int& dest, int index)
{
	dest = lua_tointeger(L, index);
	lua_remove(L, index);
}

template<> void LuaPopValue<unsigned int>(lua_State* L, unsigned int& dest, int index)
{
	dest = lua_tointeger(L, index);
	lua_remove(L, index);
}

template<> void LuaPopValue<long>(lua_State* L, long& dest, int index)
{
	dest = lua_tointeger(L, index);
	lua_remove(L, index);
}

template<> void LuaPopValue<unsigned long>(lua_State* L, unsigned long& dest, int index)
{
	dest = lua_tointeger(L, index);
	lua_remove(L, index);
}

template<> void LuaPopValue<long long>(lua_State* L, long long& dest, int index)
{
	dest = lua_tointeger(L, index);
	lua_remove(L, index);
}

template<> void LuaPopValue<unsigned long long>(lua_State* L, unsigned long long& dest, int index)
{
	dest = lua_tointeger(L, index);
	lua_remove(L, index);
}

template<> void LuaPopValue<float>(lua_State* L, float& dest, int index)
{
	dest = lua_tonumber(L, index);
	lua_remove(L, index);
}

template<> void LuaPopValue<double>(lua_State* L, double& dest, int index)
{
	dest = lua_tonumber(L, index);
	lua_remove(L, index);
}

template<> void LuaPopValue<string>(lua_State* L, string& dest, int index)
{
	dest = lua_tostringx(L, index);
	lua_remove(L, index);
}

template<> int VectorSetterFn<bool>(lua_State* L)
{
	vector<bool>* ud = GetUserData<vector<bool>>(L, 1, "getUserDataFns");
	if (lua_type(L, 2) == LUA_TNIL) ud->clear();
	else
	{
		bool res = false;
		unsigned int idx = lua_tointeger(L, 2);
		LuaPopValue<bool>(L, res);
		ud->at(idx) = res;
	}
	return 0;
}

template<> void LuaPushValue<bool>(lua_State* L, bool src)
{
	lua_pushboolean(L, src);
}

template<> void LuaPushValue<char>(lua_State* L, char src)
{
	lua_pushlstring(L, &src, 1);
}

template<> void LuaPushValue<char*>(lua_State* L, char* src)
{
	lua_pushlstring(L, src, 1);
}

template<> void LuaPushValue<const char*>(lua_State* L, const char* src)
{
	lua_pushlstring(L, src, 1);
}

template<> void LuaPushValue<short>(lua_State* L, short src)
{
	lua_pushinteger(L, src);
}

template<> void LuaPushValue<unsigned short>(lua_State* L, unsigned short src)
{
	lua_pushinteger(L, src);
}

template<> void LuaPushValue<int>(lua_State* L, int src)
{
	lua_pushinteger(L, src);
}

template<> void LuaPushValue<unsigned int>(lua_State* L, unsigned int src)
{
	lua_pushinteger(L, src);
}

template<> void LuaPushValue<long>(lua_State* L, long src)
{
	lua_pushinteger(L, src);
}

template<> void LuaPushValue<unsigned long>(lua_State* L, unsigned long src)
{
	lua_pushinteger(L, src);
}

template<> void LuaPushValue<long long>(lua_State* L, long long src)
{
	lua_pushinteger(L, src);
}

template<> void LuaPushValue<unsigned long long>(lua_State* L, unsigned long long src)
{
	lua_pushinteger(L, src);
}

template<> void LuaPushValue<float>(lua_State* L, float src)
{
	lua_pushnumber(L, src);
}

template<> void LuaPushValue<double>(lua_State* L, double src)
{
	lua_pushnumber(L, src);
}

template<> void LuaPushValue<string>(lua_State* L, string src)
{
	lua_pushstring(L, src.c_str());
}

template<> int ArraySetterFn<char>(lua_State* L)
{
	char* ud = GetUserData<char>(L, 1, "getUserDataFns");

	if (lua_type(L, 2) == LUA_TNIL) return 0;

	int index = lua_tointegerx(L, 2, nullptr);

	if (index >= 1)
	{
		ud[index - 1] = lua_tostring(L, -1)[0];
		return 0;
	}

	sprintf(ud, "%s", lua_tostring(L, -1));

	return 0;
}

template<> int ArrayGetterFn<char>(lua_State* L)
{
	char* ud = GetUserData<char>(L, 1, "getUserDataFns");
	int index = lua_tointegerx(L, 2, nullptr);

	if (index >= 1) LuaPushValue<char>(L, ud[index - 1]);

	else lua_pushstring(L, ud);

	return 1;
}

