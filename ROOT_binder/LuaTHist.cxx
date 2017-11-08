#include <iostream>
#include <vector>

#include "LuaRootClasses.h"

using namespace std;

void LuaTH1::DoFill(double x, double w)
{
	((TH1D*)rootObj)->Fill(x, w);
}

void LuaTH1::Reset()
{
	((TH1D*)rootObj)->Reset();
}

void LuaTH1::Rebuild()
{
	((TH1D*)rootObj)->Rebuild();
}

void LuaTH1::Scale(double s)
{
	((TH1D*)rootObj)->Scale(s);
}

void LuaTH1::SetRangeUserX(double xmin, double xmax)
{
	((TH1D*)rootObj)->SetAxisRange(xmin, xmax);
}

void LuaTH1::SetRangeUserY(double ymin, double ymax)
{
	((TH1D*)rootObj)->SetAxisRange(ymin, ymax, "Y");
}

void LuaTH1::MakeAccessors(lua_State* L)
{
	AddClassMethod(L, &LuaTH1::SetTitle, "SetTitle");
	AddClassMethod(L, &LuaTH1::GetTitle, "GetTitle");

	AddClassMethod(L, &LuaTH1::DoFill, "Fill");
	AddClassMethod(L, &LuaTH1::Reset, "Reset");
	AddClassMethod(L, &LuaTH1::Rebuild, "Rebuild");
	AddClassMethod(L, &LuaTH1::Scale, "Scale");

	AddClassMethod(L, &LuaTH1::SetRangeUserX, "SetRangeUserX");
	AddClassMethod(L, &LuaTH1::SetRangeUserY, "SetRangeUserY");

	AddClassMethod(L, &LuaTH1::DoDraw, "Draw");
	AddClassMethod(L, &LuaTH1::DoUpdate, "Update");
	AddClassMethod(L, &LuaTH1::DoWrite, "Write");
}

void LuaTH1::AddNonClassMethods(lua_State* L)
{
	AddMethod(L, LuaTObjectSetName<LuaTH1>, "SetName");
	AddMethod(L, LuaTObjectGetName<LuaTH1>, "GetName");

	AddMethod(L, LuaTFit<LuaTH1>, "Fit");
	AddMethod(L, LuaTClone<LuaTH1>, "Clone");
	AddMethod(L, LuaAddHist<LuaTH1>, "Add");
}

void LuaTH2::DoFill(double x, double y, double w)
{
	((TH2D*)rootObj)->Fill(x, y, w);
}

void LuaTH2::Reset()
{
	((TH2D*)rootObj)->Reset();
}

void LuaTH2::Rebuild()
{
	((TH2D*)rootObj)->Rebuild();
}
void LuaTH2::Scale(double s)
{
	((TH2D*)rootObj)->Scale(s);
}

void LuaTH2::SetRangeUserX(double xmin, double xmax)
{
	((TH2D*)rootObj)->SetAxisRange(xmin, xmax);
}

void LuaTH2::SetRangeUserY(double ymin, double ymax)
{
	((TH2D*)rootObj)->SetAxisRange(ymin, ymax, "Y");
}

void LuaTH2::MakeAccessors(lua_State* L)
{
	AddClassMethod(L, &LuaTH2::SetTitle, "SetTitle");
	AddClassMethod(L, &LuaTH2::GetTitle, "GetTitle");

	AddClassMethod(L, &LuaTH2::DoFill, "Fill");
	AddClassMethod(L, &LuaTH2::Reset, "Reset");
	AddClassMethod(L, &LuaTH2::Rebuild, "Rebuild");
	AddClassMethod(L, &LuaTH2::Scale, "Scale");

	AddClassMethod(L, &LuaTH2::SetRangeUserX, "SetRangeUserX");
	AddClassMethod(L, &LuaTH2::SetRangeUserY, "SetRangeUserY");

	AddClassMethod(L, &LuaTH2::DoDraw, "Draw");
	AddClassMethod(L, &LuaTH2::DoUpdate, "Update");
	AddClassMethod(L, &LuaTH2::DoWrite, "Write");
}

void LuaTH2::AddNonClassMethods(lua_State* L)
{
	AddMethod(L, LuaTObjectSetName<LuaTH2>, "SetName");
	AddMethod(L, LuaTObjectGetName<LuaTH2>, "GetName");

	AddMethod(L, LuaTFit<LuaTH2>, "Fit");
	AddMethod(L, LuaTClone<LuaTH2>, "Clone");
	AddMethod(L, LuaAddHist<LuaTH2>, "Add");
}

extern "C" void LoadLuaTHistLib(lua_State* L)
{
	MakeAccessFunctions<LuaTH1>(L, "TH1");
	rootObjectAliases["TH1F"] = "TH1";
	rootObjectAliases["TH1D"] = "TH1";

	AddObjectConstructor<LuaTH1, string, string, int, int, int>(L, "TH1");

	MakeAccessFunctions<LuaTH2>(L, "TH2");
	rootObjectAliases["TH2F"] = "TH2";
	rootObjectAliases["TH2D"] = "TH2";

	AddObjectConstructor<LuaTH2, string, string, int, int, int, int, int, int>(L, "TH2");
}
