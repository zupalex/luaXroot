#include "LuaRootClasses.h"

void LuaTFile::MakeActive()
{
	((TFile*)rootObj)->cd();
}

void LuaTFile::ListContent()
{
	((TFile*)rootObj)->ls();
}

void LuaTFile::MakeAccessors(lua_State* L)
{
	AddClassMethod(L, &LuaTFile::MakeActive, "cd");
	AddClassMethod(L, &LuaTFile::ListContent, "ls");

	AddClassMethod(L, &LuaTFile::DoWrite, "Write");
}

void LuaTFile::AddNonClassMethods(lua_State* L)
{
	AddMethod(L, LuaTObjectSetName<LuaTFile>, "SetName");
	AddMethod(L, LuaTObjectGetName<LuaTFile>, "GetName");

	AddMethod(L, LuaGetROOTObjectFromDir, "GetObject");
}

extern "C" void LoadLuaTFileLib(lua_State* L)
{
	MakeAccessFunctions<LuaTFile>(L, "TFile");
	rootObjectAliases["TFile"] = "TFile";

	AddObjectConstructor<LuaTFile, string, string>(L, "TFile");
}