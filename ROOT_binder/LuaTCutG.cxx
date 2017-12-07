#include "LuaRootClasses.h"

int LuaTCutG::IsInside(double x, double y)
{
	return ((TCutG*) rootObj)->IsInside(x, y);
}

void LuaTCutG::SetNPoint(int n)
{
	return ((TCutG*) rootObj)->Set(n);
}

void LuaTCutG::SetPoint(int n, double x, double y)
{
	return ((TCutG*) rootObj)->SetPoint(n, x, y);
}

double LuaTCutG::Area()
{
	return ((TCutG*) rootObj)->Area();
}

double LuaTCutG::IntegralHist(LuaROOTBase* h2d)
{
	TH2* hist = dynamic_cast<TH2*>(h2d->rootObj);

	if (hist != nullptr) return ((TCutG*) rootObj)->IntegralHist(hist, "width");
	else return 0;
}

void LuaTCutG::MakeAccessors(lua_State* L)
{
	AddClassMethod(L, &LuaTCutG::SetNPoint, "SetNPoint");
	AddClassMethod(L, &LuaTCutG::SetPoint, "SetPoint");

	AddClassMethod(L, &LuaTCutG::IsInside, "IsInside");

	AddClassMethod(L, &LuaTCutG::Area, "Area");
	AddClassMethod(L, &LuaTCutG::IntegralHist, "IntegralHist");

	AddClassMethod(L, &LuaTCutG::DoDraw, "Draw");
	AddClassMethod(L, &LuaTCutG::DoUpdate, "Update");
	AddClassMethod(L, &LuaTCutG::DoWrite, "Write");
}

void LuaTCutG::AddNonClassMethods(lua_State* L)
{
	AddMethod(L, LuaTObjectSetName<LuaTCutG>, "SetName");
	AddMethod(L, LuaTObjectGetName<LuaTCutG>, "GetName");

	AddMethod(L, LuaTClone<LuaTCutG>, "Clone");
}

extern "C" void LoadLuaTCutGLib(lua_State* L)
{
	MakeAccessFunctions<LuaTCutG>(L, "TCutG");
	rootObjectAliases["TCutG"] = "TCutG";
}
