#include "LuaRootClasses.h"

void LuaTFile::MakeActive()
{
	((TFile*) rootObj)->cd();
}

void LuaTFile::ListContent()
{
	((TFile*) rootObj)->ls();
}

void LuaTFile::Close()
{
	((TFile*) rootObj)->Close();
}

void LuaTFile::Open(string path, string opts)
{
	((TFile*) rootObj)->Open(path.c_str(), opts.c_str());
}

void LuaTFile::Flush()
{
	((TFile*) rootObj)->Flush();
}

int LuaTFile::Write(string name)
{
	return ((TFile*) rootObj)->Write(name.c_str());
}

int LuaTFile::Overwrite(string name)
{
	return ((TFile*) rootObj)->Write(name.c_str(), 4);
}

void LuaTFile::ReadKeys()
{
	((TFile*) rootObj)->ReadKeys();
}

void LuaTFile::Refresh(string name, LuaROOTBase* dest)
{
	theApp->NotifyUpdatePending();
	delete ((TFile*) rootObj)->FindObject(name.c_str());
	((TFile*) rootObj)->GetObject(name.c_str(), dest->rootObj);
	theApp->NotifyUpdateDone();
}

void LuaTFile::MakeAccessors(lua_State* L)
{
	AddClassMethod(L, &LuaTFile::MakeActive, "cd");
	AddClassMethod(L, &LuaTFile::ListContent, "ls");

	AddClassMethod(L, &LuaTFile::Close, "Close");
	AddClassMethod(L, &LuaTFile::Open, "Open");

	AddClassMethod(L, &LuaTFile::Write, "Write");
	AddClassMethod(L, &LuaTFile::Overwrite, "Overwrite");
	AddClassMethod(L, &LuaTFile::Flush, "Flush");

	AddClassMethod(L, &LuaTFile::ReadKeys, "ReadKeys");
	AddClassMethod(L, &LuaTFile::Refresh, "Refresh");
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
