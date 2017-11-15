#include "LuaExtension.h"
#include <llex.h>

#undef lua_saveline
#define lua_saveline(L,line) ((void)L, add_history(line), srtysjghs, appendtohist())

lua_State* lua = 0;

map<string, function<int()>> methodList;
map<string, map<int, function<int(int)>>> constructorList;

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

            //      cout << "pushed: [" << i << "] = { type = " << dtype.c_str() << " , name = " << dentry->d_name << " }" << endl;

            i++;
        }
    }

    closedir(dir);

    return 1;
}

map<string, function<void(lua_State*)>> newUserDataFns;
map<string, function<void(lua_State*, char*)>> assignUserDataFns;

map<string, int> userDataSizes;

template<> void LuaPopValue<bool>(lua_State* L, bool* dest)
{
    *dest = lua_toboolean(L, -1);
}

template<> void LuaPopValue<char>(lua_State* L, char* dest)
{
    sprintf(dest, "%s", lua_tostring(L, -1));
}

template<> void LuaPopValue<short>(lua_State* L, short* dest)
{
    *dest = lua_tointeger(L, -1);
}

template<> void LuaPopValue<unsigned short>(lua_State* L, unsigned short* dest)
{
    *dest = lua_tointeger(L, -1);
}

template<> void LuaPopValue<int>(lua_State* L, int* dest)
{
    *dest = lua_tointeger(L, -1);
}

template<> void LuaPopValue<unsigned int>(lua_State* L, unsigned int* dest)
{
    *dest = lua_tointeger(L, -1);
}

template<> void LuaPopValue<long>(lua_State* L, long* dest)
{
    *dest = lua_tointeger(L, -1);
}

template<> void LuaPopValue<unsigned long>(lua_State* L, unsigned long* dest)
{
    *dest = lua_tointeger(L, -1);
}

template<> void LuaPopValue<long long>(lua_State* L, long long* dest)
{
    *dest = lua_tointeger(L, -1);
}

template<> void LuaPopValue<unsigned long long>(lua_State* L, unsigned long long* dest)
{
    *dest = lua_tointeger(L, -1);
}

template<> void LuaPopValue<float>(lua_State* L, float* dest)
{
    *dest = lua_tonumber(L, -1);
}

template<> void LuaPopValue<double>(lua_State* L, double* dest)
{
    *dest = lua_tonumber(L, -1);
}

template<> void LuaPopValue<string>(lua_State* L, string* dest)
{
    *dest = lua_tostring(L, -1);
}

template<> void LuaPushValue<bool>(lua_State* L, bool src)
{
    lua_pushboolean(L, src);
}

template<> void LuaPushValue<char>(lua_State* L, char src)
{
    lua_pushstring(L, &src);
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

int luaExt_GetUserDataSize(lua_State* L)
{
    if (!CheckLuaArgs(L, 1, true, "luaExt_SetUserDataValue", LUA_TSTRING)) return 0;

    string type = lua_tostring(L, 1);

    lua_pushinteger(L, userDataSizes[type]);

    return 1;
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

inline int StrLength(lua_State* L)
{
    string* str = GetUserData<string>(L);

    lua_pushinteger(L, str->length());

    return 1;
}

void MakeStringAccessor(lua_State* L)
{
    userDataSizes["string"] = sizeof(string);

    auto ctor = [=](int index)
    {
        NewUserData<string>(L);

        MakeMetatable ( L );
        SetupMetatable<string> ( L );

        AddMethod(L, StrLength, "StrLength");

        return sizeof(string);
    };

    constructorList["string"][0] = ctor;

//  setUserDataFns["string"] = [=] ( lua_State* L_, char* address)
//  {
//      string* ud;
//      if(address == nullptr)
//      {
//          ud = GetUserData<string> ( L_, 1, "setUserDataFns" );
//          *ud = lua_tostring(L, -1);
//      }
//      else
//      {
//          int strlen = lua_rawlen(L, -1);
//          *((int*)address) = strlen;
//          sprintf(address+sizeof(int), "%s", lua_tostring(L, -1));
//      }
//  };

//  getUserDataFns["string"] = [=] ( lua_State* L_, char* address)
//  {
//      string* ud;
//      if(address == nullptr)
//      {
//          ud = GetUserData<string> ( L_, 1, "setUserDataFns" );
//          lua_pushstring(L, (*ud).c_str());
//      }
//      else
//      {
//          int strlen = *((int*) address);
//          lua_pushlstring(L, address+sizeof(int), strlen);
//      }
//  };

    assignUserDataFns["string"] = [=] (lua_State* L_, char* addr)
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

//          setUserDataFns[type](L, address);
        }
        else
        {
            lua_geti(L, 2, i + 1);
            type = lua_tostring(L, -1);
            lua_pop(L, 1);

            if (type != "string") effectiveSize = userDataSizes[type];
            else effectiveSize = lua_rawlen(L, -1) + sizeof(int);

//          setUserDataFns[type](L, address);
        }

        address += effectiveSize;
        totsize += effectiveSize;
    }

    return totsize;
}

void GetMemoryBlock(lua_State* L, char* address)
{
    int struct_size = lua_rawlen(L, 1);

    bool toUserData = false;

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
//          getUserDataFns[type](L, address);

            if (type != "string") effectiveSize = userDataSizes[type];
            else effectiveSize = lua_rawlen(L, -1) + sizeof(int);

            lua_getfield(L, -2, "Set");
            lua_insert(L, -3);
            lua_pcall(L, 2, 0, 0);
        }
        else
        {
            type = lua_tostring(L, -1);
//          getUserDataFns[type](L, address);

            if (type != "string") effectiveSize = userDataSizes[type];
            else effectiveSize = lua_rawlen(L, -1) + sizeof(int);

            lua_seti(L, 2, i + 1);
            lua_pop(L, 1);
        }

        address += effectiveSize;
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
