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

void LuaTCanvas::Draw(LuaROOTBase* obj, int row, int col)
{
	if (obj == nullptr) return;

	TObject* robj = obj->rootObj;

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

void LuaTCanvas::SetSize(int width, int height)
{
	theApp->NotifyUpdatePending();

	((LuaCanvas*) rootObj)->SetCanvasSize(width, height);

	theApp->NotifyUpdateDone();
}

void LuaTCanvas::SetWindowSize(int width, int height)
{
	theApp->NotifyUpdatePending();

	((LuaCanvas*) rootObj)->SetWindowSize(width, height);

	theApp->NotifyUpdateDone();
}

void LuaTCanvas::SetLogScale(int rown_, int coln_, string axis, bool val)
{
	theApp->NotifyUpdatePending();
	LuaCanvas* can = (LuaCanvas*) rootObj;

	if (rown_ > 0 && coln_ > 0)
	{
		int subpadnum = coln_ + (ncol * (rown_ - 1));
		can = (LuaCanvas*) ((LuaCanvas*) rootObj)->cd(subpadnum);
	}

	if (axis == "X") can->SetLogx(val);
	else if (axis == "Y") can->SetLogy(val);
	else if (axis == "Z") can->SetLogz(val);

	can->Modified();
	can->Update();

	theApp->NotifyUpdateDone();
}

void LuaTCanvas::MakeAccessors(lua_State* L)
{
	AddClassMethod(L, &LuaTCanvas::SetTitle, "SetTitle");
	AddClassMethod(L, &LuaTCanvas::GetTitle, "GetTitle");

	AddClassMethod(L, &LuaTCanvas::Close, "Close");
	AddClassMethod(L, &LuaTCanvas::Divide, "Divide");
	AddClassMethod(L, &LuaTCanvas::Draw, "Draw");
	AddClassMethod(L, &LuaTCanvas::Clear, "Clear");
	AddClassMethod(L, &LuaTCanvas::Update, "Update");

	AddClassMethod(L, &LuaTCanvas::SetSize, "SetSize");
	AddClassMethod(L, &LuaTCanvas::SetWindowSize, "SetWindowSize");

	AddClassMethod(L, &LuaTCanvas::SetLogScale, "SetLogScale");
}

void LuaTCanvas::AddNonClassMethods(lua_State* L)
{
	AddMethod(L, LuaTObjectSetName<LuaTCanvas>, "SetName");
	AddMethod(L, LuaTObjectGetName<LuaTCanvas>, "GetName");
}

extern "C" void LoadLuaTCanvasLib(lua_State* L)
{
	MakeAccessFunctions<LuaTCanvas>(L, "TCanvas");
	rootObjectAliases["TCanvas"] = "TCanvas";
}
