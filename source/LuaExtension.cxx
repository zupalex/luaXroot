#include "LuaExtension.h"
#include <llex.h>

#undef lua_saveline
#define lua_saveline(L,line) ((void)L, add_history(line), srtysjghs, appendtohist())

lua_State* lua = 0;

map<string, function<int()>> methodList;
map<string, map<int, function<void(int)>>> constructorList;

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

	if (sepPos == string::npos)
		chain.push_back(gfield);
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

	if (sepPos == string::npos)
		chain.push_back(gfield);
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
		} else if (lua_type(L, -1) != LUA_TTABLE)
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
	} else
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

map<string, function<void(lua_State*)>> setUserDataFns;
map<string, function<void(lua_State*)>> getUserDataFns;
map<string, function<void(lua_State*)>> newUserDataFns;

int luaExt_SetUserDataValue(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "luaExt_SetUserDataValue", LUA_TUSERDATA)) return 0;

	lua_getfield(L, 1, "type");
	string ud_type = lua_tostring(L, -1);
	lua_pop(L, 1);

	setUserDataFns[ud_type](L);

	return 0;
}

int luaExt_PushBackUserDataValue(lua_State* L)
{
	if (lua_type(L, -1) == LUA_TNIL)
	{
		cerr << "Cannot push_back nil..." << endl;

		return 0;
	}

	return luaExt_SetUserDataValue(L);
}

int luaExt_GetVectorSize(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "luaExt_GetVectorSize", LUA_TUSERDATA)) return 0;

	lua_settop(L, 1);

	lua_getfield(L, 1, "Get");
	lua_insert(L, 1);

	lua_pcall(L, 1, 1, 0);

	if (!CheckLuaArgs(L, -1, true, "luaExt_GetVectorSize: tried to call on invalid userdata", LUA_TTABLE)) return 0;

	lua_pushinteger(L, lua_rawlen(L, -1));

	return 1;
}

int luaExt_GetUserDataValue(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "luaExt_GetUserDataValue", LUA_TUSERDATA)) return 0;

	int index = lua_tointegerx(L, 2, nullptr);

	lua_getfield(L, 1, "type");
	string ud_type = lua_tostring(L, -1);
	lua_pop(L, 1);

	getUserDataFns[ud_type](L);

	if (index > 0)
	{
		lua_geti(L, -1, index);
	}

	return 1;
}

