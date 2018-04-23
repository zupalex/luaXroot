#include <iostream>
#include <vector>

#include "LuaRootClasses.h"

using namespace std;

void LuaGraphError::SetPointColor(int colorid)
{
	((TGraphAsymmErrors*) rootObj)->SetMarkerColor(colorid);
}

void LuaGraphError::SetPointSize(int size)
{
	((TGraphAsymmErrors*) rootObj)->SetMarkerSize(size);
}

void LuaGraphError::SetPointStyle(int styleid)
{
	((TGraphAsymmErrors*) rootObj)->SetMarkerStyle(styleid);
}

void LuaGraphError::SetPointColorAlpha(int colorid, float alpha)
{
	((TGraphAsymmErrors*) rootObj)->SetMarkerColorAlpha(colorid, alpha);
}

void LuaGraphError::SetLineColor(int colorid)
{
	((TGraphAsymmErrors*) rootObj)->SetLineColor(colorid);
}

void LuaGraphError::SetLineWidth(int width)
{
	((TGraphAsymmErrors*) rootObj)->SetLineWidth(width);
}

void LuaGraphError::SetLineStyle(int styleid)
{
	((TGraphAsymmErrors*) rootObj)->SetLineStyle(styleid);
}

void LuaGraphError::SetLineColorAlpha(int colorid, float alpha)
{
	((TGraphAsymmErrors*) rootObj)->SetLineColorAlpha(colorid, alpha);
}

void LuaGraphError::Set(int n)
{
	((TGraphAsymmErrors*) rootObj)->Set(n);
}

int LuaGraphError::GetNPoints()
{
	return ((TGraphAsymmErrors*) rootObj)->GetN();
}

int LuaGraphError::GetMaxSize()
{
	return ((TGraphAsymmErrors*) rootObj)->GetMaxSize();
}

tuple<double, double> LuaGraphError::GetPointVals(int i)
{
	double x, y;
	((TGraphAsymmErrors*) rootObj)->GetPoint(i - 1, x, y);
	return make_tuple(x, y);
}

tuple<double, double, double, double> LuaGraphError::GetPointErrors(int i)
{
	return make_tuple(((TGraphAsymmErrors*) rootObj)->GetErrorXlow(i - 1), ((TGraphAsymmErrors*) rootObj)->GetErrorXhigh(i - 1),
			((TGraphAsymmErrors*) rootObj)->GetErrorYlow(i - 1), ((TGraphAsymmErrors*) rootObj)->GetErrorYhigh(i - 1));
}

void LuaGraphError::SetPointVals(int i, double x, double y)
{
	((TGraphAsymmErrors*) rootObj)->SetPoint(i - 1, x, y);
}

void LuaGraphError::SetPointErrors(int i, double exl, double exh, double eyl, double eyh)
{
	((TGraphAsymmErrors*) rootObj)->SetPointError(i - 1, exl, exh, eyl, eyh);
}

void LuaGraphError::SetPointErrorsX(int i, double exl, double exh)
{
	((TGraphAsymmErrors*) rootObj)->SetPointEXlow(i - 1, exl);
	((TGraphAsymmErrors*) rootObj)->SetPointEXhigh(i - 1, exh);
}

void LuaGraphError::SetPointErrorsY(int i, double eyl, double eyh)
{
	((TGraphAsymmErrors*) rootObj)->SetPointEYlow(i - 1, eyl);

}

void LuaGraphError::SetPointErrorsXHigh(int i, double exh)
{
	((TGraphAsymmErrors*) rootObj)->SetPointEXhigh(i - 1, exh);
}

void LuaGraphError::SetPointErrorsXLow(int i, double exl)
{
	((TGraphAsymmErrors*) rootObj)->SetPointEXlow(i - 1, exl);
}

void LuaGraphError::SetPointErrorsYHigh(int i, double eyh)
{
	((TGraphAsymmErrors*) rootObj)->SetPointEYhigh(i - 1, eyh);
}

void LuaGraphError::SetPointErrorsYLow(int i, double eyl)
{
	((TGraphAsymmErrors*) rootObj)->SetPointEYlow(i - 1, eyl);
}

int LuaGraphError::RemovePoint(int i)
{
	return ((TGraphAsymmErrors*) rootObj)->RemovePoint(i);
}

double LuaGraphError::Eval(double x)
{
	return ((TGraphAsymmErrors*) rootObj)->Eval(x);
}

void LuaGraphError::SetRangeUserX(double xmin, double xmax)
{
	((TGraphAsymmErrors*) rootObj)->GetXaxis()->SetRangeUser(xmin, xmax);
}

void LuaGraphError::SetRangeUserY(double ymin, double ymax)
{
	((TGraphAsymmErrors*) rootObj)->GetYaxis()->SetRangeUser(ymin, ymax);
}

void LuaGraphError::MakeAccessors(lua_State* L)
{
	AddClassMethod(L, &LuaGraphError::SetTitle, "SetTitle");
	AddClassMethod(L, &LuaGraphError::GetTitle, "GetTitle");

	AddClassMethod(L, &LuaGraphError::GetNPoints, "GetNPoints");
	AddClassMethod(L, &LuaGraphError::GetMaxSize, "GetMaxSize");

	AddClassMethod(L, &LuaGraphError::GetPointVals, "GetPoint");
	AddClassMethod(L, &LuaGraphError::GetPointErrors, "GetPointError");

	AddClassMethod(L, &LuaGraphError::Set, "SetNPoint");
	AddClassMethod(L, &LuaGraphError::SetPointVals, "SetPoint");
	AddClassMethod(L, &LuaGraphError::SetPointErrors, "SetPointErrors");
	AddClassMethod(L, &LuaGraphError::SetPointErrorsX, "SetPointErrorsX");
	AddClassMethod(L, &LuaGraphError::SetPointErrorsY, "SetPointErrorsY");
	AddClassMethod(L, &LuaGraphError::SetPointErrorsXHigh, "SetPointErrorsXHigh");
	AddClassMethod(L, &LuaGraphError::SetPointErrorsXLow, "SetPointErrorsXLow");
	AddClassMethod(L, &LuaGraphError::SetPointErrorsYHigh, "SetPointErrorsYHigh");
	AddClassMethod(L, &LuaGraphError::SetPointErrorsYLow, "SetPointErrorsYLow");

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

	AddClassMethod(L, &LuaGraphError::SetRangeUserX, "SetRangeUserX");
  AddClassMethod(L, &LuaGraphError::SetRangeUserY, "SetRangeUserY");
  
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
	AddObjectConstructor<LuaGraphError, int, vector<double>, vector<double>, vector<double>, vector<double>, vector<double>, vector<double>>(L, "TGraph");
}
