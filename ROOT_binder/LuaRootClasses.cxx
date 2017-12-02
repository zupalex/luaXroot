#include "LuaRootClasses.h"

using namespace std;

int luaExt_GetGDirContent(lua_State* L)
{
	gDirectory->ls();

	return 0;
}

int LuaGetROOTObjectFromDir(lua_State* L)
{
	TDirectory* dir = gDirectory;

	if (lua_type(L, 1) == LUA_TUSERDATA)
	{
		LuaTFile* file = GetUserData<LuaTFile>(L, 1);
		dir = (TDirectory*) file->rootObj;
		lua_remove(L, 1);
	}

	if (!CheckLuaArgs(L, 1, true, "LuaGetROOTObjectFromDir", LUA_TSTRING, LUA_TSTRING)) return 0;

	string type = lua_tostring(L, 1);
	type = rootObjectAliases[type];

	string objName = lua_tostring(L, 2);

	TObject* ret = dir->FindObject(objName.c_str());

	if (ret == nullptr)
	{
		cerr << "The object with name " << objName << " does not exist in this location..." << endl;
		return 0;
	}

	lua_getglobal(L, "New");
	lua_pushstring(L, type.c_str());

	lua_pcall(L, 1, 1, 0);

	if (lua_type(L, -1) != LUA_TUSERDATA) return 0;

	LuaUserClass* obj = GetUserData<LuaUserClass>(L, -1);

	obj->SetROOTObject(ret);

	return 1;
}

void LuaDrawTObject(TObject* obj, string opts)
{
	theApp->NotifyUpdatePending();

	if (canvasTracker[obj] != nullptr && ((string) canvasTracker[obj]->GetName()).empty()) delete canvasTracker[obj];

	if (canvasTracker[obj] == nullptr || ((string) canvasTracker[obj]->GetName()).empty())
	{
		if (opts.find("same") == string::npos && opts.find("SAME") == string::npos)
		{
			LuaCanvas* disp = new LuaCanvas();
			disp->cd();
			canvasTracker[obj] = disp;
		}
		else
		{
			canvasTracker[obj] = (LuaCanvas*) gPad->GetCanvas();
		}

		obj->Draw(opts.c_str());
		canvasTracker[obj]->Update();
	}
	else
	{
		canvasTracker[obj]->cd();
		obj->Draw(opts.c_str());
//			if (dynamic_cast<TH1*>(rootObj) != nullptr) ((TH1*) rootObj)->Rebuild();
//			canvasTracker[rootObj]->Modified();
		canvasTracker[obj]->Update();
	}

	theApp->NotifyUpdateDone();
}

extern "C" int openlib_lua_root_classes(lua_State* L)
{
	lua_register(L, "GetObject", LuaGetROOTObjectFromDir);
	lua_register(L, "ListCurrentDir", luaExt_GetGDirContent);

	LoadLuaTFileLib(L);
	LoadLuaTGraphLib(L);
	LoadLuaTF1Lib(L);
	LoadLuaTHistLib(L);
	LoadLuaTSpectrumLib(L);
	LoadLuaTCutGLib(L);
	LoadLuaTTreeLib(L);

	return 0;
}
