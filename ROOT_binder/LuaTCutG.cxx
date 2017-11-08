#include "LuaRootClasses.h"

int LuaTCutG::IsInside(double x, double y)
{
	return ((TCutG*)rootObj)->IsInside(x, y);
}

void LuaTCutG::MakeAccessors(lua_State* L)
{
	AddClassMethod(L, &LuaTCutG::IsInside, "IsInside");

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
