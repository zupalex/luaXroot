#include <iostream>

#include "LuaRootClasses.h"

using namespace std;

void LuaTCanvas::Close()
{
	((LuaCanvas*) rootObj)->Close();
}

void LuaTCanvas::Clear()
{
	theApp->NotifyUpdatePending();

	TList* prims = ((LuaCanvas*) rootObj)->GetListOfPrimitives();

	for (int i = 0; i < prims->GetSize(); i++)
	{
		TObject* prim_obj = prims->At(i);
		canvasTracker[prim_obj] = nullptr;
	}

	dispObjs.clear();

	((LuaCanvas*) rootObj)->Clear();

	((LuaCanvas*) rootObj)->Modified();
	((LuaCanvas*) rootObj)->Update();

	theApp->NotifyUpdateDone();
}

void LuaTCanvas::Divide(int nrow_, int ncol_)
{
	((LuaCanvas*) rootObj)->Divide(ncol_, nrow_);

	nrow = nrow_;
	ncol = ncol_;
}

void LuaTCanvas::Draw(LuaUserClass* obj, int row, int col)
{
	if (obj == nullptr) return;

	TObject* robj = obj->GetROOTObject();

	int subpadnum = col + (ncol * (row - 1));

	LuaCanvas* subpad = (LuaCanvas*) ((LuaCanvas*) rootObj)->cd(subpadnum);

	if (dispObjs[subpad] != nullptr)
	{
		canvasTracker[dispObjs[subpad]] = nullptr;
	}

	dispObjs[subpad] = robj;

	canvasTracker[robj] = subpad;

	// This function is completed on the Lua side in lua_root_binder.lua
}

void LuaTCanvas::Update()
{
	theApp->NotifyUpdatePending();
	((LuaCanvas*) rootObj)->Modified();
	((LuaCanvas*) rootObj)->Update();

	for (auto itr = dispObjs.begin(); itr != dispObjs.end(); itr++)
	{
		itr->first->Modified();
		itr->first->Update();
	}

	theApp->NotifyUpdateDone();
}

void LuaTCanvas::MakeAccessors(lua_State* L)
{
	AddClassMethod(L, &LuaTCanvas::Close, "Close");
	AddClassMethod(L, &LuaTCanvas::Divide, "Divide");
	AddClassMethod(L, &LuaTCanvas::Draw, "Draw");
	AddClassMethod(L, &LuaTCanvas::Clear, "Clear");
	AddClassMethod(L, &LuaTCanvas::Update, "Update");
}

void LuaTCanvas::AddNonClassMethods(lua_State* L)
{

}

extern "C" void LoadLuaTCanvasLib(lua_State* L)
{
	MakeAccessFunctions<LuaTCanvas>(L, "TCanvas");
	rootObjectAliases["TCanvas"] = "TCanvas";
}
