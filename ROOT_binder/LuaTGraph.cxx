#include <iostream>
#include <vector>

#include "LuaRootClasses.h"

using namespace std;

void LuaGraphError::SetPointColor(int colorid)
{
	((TGraphErrors*) rootObj)->SetMarkerColor(colorid);
}

void LuaGraphError::SetPointSize(int size)
{
	((TGraphErrors*) rootObj)->SetMarkerSize(size);
}

void LuaGraphError::SetPointStyle(int styleid)
{
	((TGraphErrors*) rootObj)->SetMarkerStyle(styleid);
}

void LuaGraphError::SetPointColorAlpha(int colorid, float alpha)
{
	((TGraphErrors*) rootObj)->SetMarkerColorAlpha(colorid, alpha);
}

void LuaGraphError::SetLineColor(int colorid)
{
	((TGraphErrors*) rootObj)->SetLineColor(colorid);
}

void LuaGraphError::SetLineWidth(int width)
{
	((TGraphErrors*) rootObj)->SetLineWidth(width);
}

void LuaGraphError::SetLineStyle(int styleid)
{
	((TGraphErrors*) rootObj)->SetLineStyle(styleid);
}

void LuaGraphError::SetLineColorAlpha(int colorid, float alpha)
{
	((TGraphErrors*) rootObj)->SetLineColorAlpha(colorid, alpha);
}

void LuaGraphError::Set(int n)
{
	((TGraphErrors*) rootObj)->Set(n);
}

int LuaGraphError::GetMaxSize()
{
	return ((TGraphErrors*) rootObj)->GetMaxSize();
}

tuple<double, double> LuaGraphError::GetPointVals(int i)
{
	double x, y;
	((TGraphErrors*) rootObj)->GetPoint(i - 1, x, y);
	return make_tuple(x, y);
}

tuple<double, double> LuaGraphError::GetPointErrors(int i)
{
	return make_tuple(((TGraphErrors*) rootObj)->GetErrorX(i - 1), ((TGraphErrors*) rootObj)->GetErrorY(i - 1));
}

void LuaGraphError::SetPointVals(int i, double x, double y)
{
	((TGraphErrors*) rootObj)->SetPoint(i - 1, x, y);
}

void LuaGraphError::SetPointErrors(int i, double errx, double erry)
{
	((TGraphErrors*) rootObj)->SetPointError(i - 1, errx, erry);
}

int LuaGraphError::RemovePoint(int i)
{
	return ((TGraphErrors*) rootObj)->RemovePoint(i);
}

double LuaGraphError::Eval(double x)
{
	return ((TGraphErrors*) rootObj)->Eval(x);
}

void LuaGraphError::MakeAccessors(lua_State* L)
{
	AddClassMethod(L, &LuaGraphError::SetTitle, "SetTitle");
	AddClassMethod(L, &LuaGraphError::GetTitle, "GetTitle");

	AddClassMethod(L, &LuaGraphError::GetMaxSize, "GetMaxSize");

	AddClassMethod(L, &LuaGraphError::GetPointVals, "GetPoint");
	AddClassMethod(L, &LuaGraphError::GetPointErrors, "GetPointError");

	AddClassMethod(L, &LuaGraphError::Set, "SetNPoint");
	AddClassMethod(L, &LuaGraphError::SetPointVals, "SetPoint");
	AddClassMethod(L, &LuaGraphError::SetPointErrors, "SetPointError");

	AddClassMethod(L, &LuaGraphError::SetPointColor, "SetPointColor");
	AddClassMethod(L, &LuaGraphError::SetPointSize, "SetPointSize");
	AddClassMethod(L, &LuaGraphError::SetPointStyle, "SetPointStyle");
	AddClassMethod(L, &LuaGraphError::SetPointColorAlpha, "SetPointColorAlpha");

	AddClassMethod(L, &LuaGraphError::SetLineColor, "SetLineColor");
	AddClassMethod(L, &LuaGraphError::SetLineWidth, "SetLineWidth");
	AddClassMethod(L, &LuaGraphError::SetLineStyle, "SetLineStyle");
	AddClassMethod(L, &LuaGraphError::SetLineColorAlpha, "SetLineColorAlpha");

	AddClassMethod(L, &LuaGraphError::RemovePoint, "RemovePoint");
	AddClassMethod(L, &LuaGraphError::Eval, "Eval");

	AddClassMethod(L, &LuaGraphError::DoDraw, "Draw");
	AddClassMethod(L, &LuaGraphError::DoUpdate, "Update");
	AddClassMethod(L, &LuaGraphError::DoWrite, "Write");
}

void LuaGraphError::AddNonClassMethods(lua_State* L)
{
	AddMethod(L, LuaTObjectSetName<LuaGraphError>, "SetName");
	AddMethod(L, LuaTObjectGetName<LuaGraphError>, "GetName");

	AddMethod(L, LuaTFit<LuaGraphError>, "Fit");
	AddMethod(L, LuaTClone<LuaGraphError>, "Clone");
}

extern "C" void LoadLuaTGraphLib(lua_State* L)
{
	MakeAccessFunctions<LuaGraphError>(L, "TGraph");
	rootObjectAliases["TGraph"] = "TGraph";
	rootObjectAliases["TGraphErrors"] = "TGraph";

	AddObjectConstructor<LuaGraphError, int>(L, "TGraph");
	AddObjectConstructor<LuaGraphError, int, vector<double>, vector<double>>(L, "TGraph");
	AddObjectConstructor<LuaGraphError, int, vector<double>, vector<double>, vector<double>, vector<double>>(L, "TGraph");
}
