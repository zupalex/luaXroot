#include "LuaRootClasses.h"
#include "TTree.h"

unsigned long long LuaTTree::GetEntries()
{
	return ((TTree*) rootObj)->GetEntries();
}

void LuaTTree::Fill()
{
	((TTree*) rootObj)->Fill();
}

void LuaTTree::GetEntry(unsigned long long entry)
{
	((TTree*) rootObj)->GetEntry(entry);
}

void LuaTTree::Draw(string varexp, string cond, string opts, unsigned long long nentries, unsigned long long firstentry)
{
	if(nentries == 0) nentries = numeric_limits<long long>::max();
	((TTree*) rootObj)->Draw(varexp.c_str(), cond.c_str(), opts.c_str(), nentries, firstentry);
}

void LuaTTree::MakeAccessors(lua_State* L)
{
	AddClassMethod(L, &LuaTTree::GetEntries, "GetEntries");
	AddClassMethod(L, &LuaTTree::Fill, "Fill");

	AddClassMethod(L, &LuaTTree::GetEntry, "GetEntry");
	AddClassMethod(L, &LuaTTree::DoDraw, "Draw");

	AddClassMethod(L, &LuaTTree::DoWrite, "Write");
}

int luaExt_TTree_NewBranch_Interface(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "luaExt_TTree_NewBranch_Interface", LUA_TUSERDATA, LUA_TTABLE)) return 0;

	lua_unpackarguments(L, 2, "luaExt_TTree_NewBranch_Interface argument table",
		{ "type", "name" },
		{ LUA_TSTRING, LUA_TSTRING },
		{ true, true });

	lua_remove(L, 2);

	const char* bname = lua_tostring(L, -1);
	lua_pop(L, 1);

	string btype = lua_tostring(L, -1);
	string adjust_type = btype;

	if (btype.find("[") != string::npos) adjust_type = btype.substr(0, btype.find("[")) + "[]";

	LuaTTree* lua_tree = GetUserData<LuaTTree>(L, 1, "luaExt_TTree_NewBranch");
	TTree* tree = (TTree*) lua_tree->rootObj;

	lua_getfield(L, 1, "branches_list");
	lua_remove(L, 1);

	lua_pushstring(L, btype.c_str());
	newBranchFns[adjust_type](L, tree, bname);

	lua_pushvalue(L, -1);
	lua_setfield(L, -3, bname);

	return 1;
}

int luaExt_TTree_GetBranch_Interface(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "luaExt_TTree_GetBranch_Interface", LUA_TUSERDATA, LUA_TTABLE)) return 0;

	lua_unpackarguments(L, 2, "luaExt_TTree_GetBranch_Interface argument table",
		{ "name" },
		{ LUA_TSTRING },
		{ true });

	const char* bname = lua_tostring(L, -1);

	lua_getfield(L, 1, "branches_list");
	lua_getfield(L, -1, bname);

	if (lua_type(L, -1) == LUA_TNIL)
	{
		cerr << "Trying to retrieve branch " << bname << " but it does not exists..." << endl;
		lua_pushnil(L);
		return 1;
	}

	return 1;
}

void LuaTTree::AddNonClassMethods(lua_State* L)
{
	AddMethod(L, LuaTObjectSetName<LuaTTree>, "SetName");
	AddMethod(L, LuaTObjectGetName<LuaTTree>, "GetName");

	AddMethod(L, LuaTClone<LuaTTree>, "Clone");

	AddMethod(L, luaExt_TTree_NewBranch_Interface, "NewBranch");
	AddMethod(L, luaExt_TTree_GetBranch_Interface, "GetBranch");
}

extern "C" void LoadLuaTTreeLib(lua_State* L)
{
	MakeAccessFunctions<LuaTTree>(L, "TTree");
	rootObjectAliases["TTree"] = "TTree";

	AddObjectConstructor<LuaTTree, string, string>(L, "TTree");
}
