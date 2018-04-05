#include <iostream>
#include <vector>

#include "LuaRootClasses.h"

using namespace std;

void LuaTF1::SetParameter(int param, double value)
{
	theApp->NotifyUpdatePending();

	((TF1*) rootObj)->SetParameter(param, value);

	if (canvasTracker[rootObj] != nullptr)
	{
		canvasTracker[rootObj]->Modified();
		canvasTracker[rootObj]->Update();
	}

	theApp->NotifyUpdateDone();
}

void LuaTF1::SetParameters(vector<double> params)
{
	theApp->NotifyUpdatePending();

	/*for (unsigned int i = 0; i < params.size(); i++)
	 {
	 cout << params[i] << endl;
	 }*/

	((TF1*) rootObj)->SetParameters(&params[0]);

	if (canvasTracker[rootObj] != nullptr)
	{
		canvasTracker[rootObj]->Modified();
		canvasTracker[rootObj]->Update();
	}

	theApp->NotifyUpdateDone();
}

void LuaTF1::SetParLimits(int ipar, double parmin, double parmax)
{
	((TF1*) rootObj)->SetParLimits(ipar, parmin, parmax);
}

void LuaTF1::SetParError(int ipar, double error)
{
	((TF1*) rootObj)->SetParError(ipar, error);
}

void LuaTF1::SetParErrors(vector<double> errors)
{
	((TF1*) rootObj)->SetParErrors(&errors[0]);
}

void LuaTF1::SetParName(int ipar, string parname)
{
	((TF1*) rootObj)->SetParName(ipar, parname.c_str());
}

void LuaTF1::FixParameter(int ipar, double val)
{
	((TF1*) rootObj)->FixParameter(ipar, val);
}

double LuaTF1::GetParameter(int param)
{
	return ((TF1*) rootObj)->GetParameter(param);
}

vector<double> LuaTF1::GetParameters()
{
	double* params = ((TF1*) rootObj)->GetParameters();

	vector<double> parsv;

	for (int i = 0; i < ((TF1*) rootObj)->GetNpar(); i++)
		parsv.push_back(params[i]);

	return parsv;
}

string LuaTF1::GetParName(int ipar)
{
	return (string) ((TF1*) rootObj)->GetParName(ipar);
}

int LuaTF1::GetParNumber(string parname)
{
	return ((TF1*) rootObj)->GetParNumber(parname.c_str());
}

double LuaTF1::GetParError(int i)
{
	return ((TF1*) rootObj)->GetParError(i);
}

vector<double> LuaTF1::GetParErrors()
{
	const double* errors = ((TF1*) rootObj)->GetParErrors();

	vector<double> parserrs;

	for (int i = 0; i < ((TF1*) rootObj)->GetNpar(); i++)
		parserrs.push_back(errors[i]);

	return parserrs;
}

vector<double> LuaTF1::GetParLimits(int ipar)
{
	double pmin, pmax;

	((TF1*) rootObj)->GetParLimits(ipar, pmin, pmax);

	vector<double> parlimits = { pmin, pmax };

	return parlimits;
}

double LuaTF1::GetChi2()
{
	return ((TF1*) rootObj)->GetChisquare();
}

double LuaTF1::Eval(double x)
{
	return ((TF1*) rootObj)->Eval(x);
}

double LuaTF1::GetX(double y)
{
	return ((TF1*) rootObj)->GetX(y);
}

double LuaTF1::Integral(double xmin, double xmax)
{
	return ((TF1*) rootObj)->Integral(xmin, xmax);
}

void LuaTF1::SetRange(double xmin, double xmax)
{
	return ((TF1*) rootObj)->SetRange(xmin, xmax);
}

void LuaTF1::SetNpx(int npx)
{
	return ((TF1*) rootObj)->SetNpx(npx);
}

double LuaTF1::GetRandom(double xmin, double xmax)
{
	return ((TF1*) rootObj)->GetRandom(xmin, xmax);
}

bool LuaTF1::IsValid()
{
	return ((TF1*) rootObj)->IsValid();
}

void LuaTF1::MakeAccessors(lua_State* L)
{
	AddClassMethod(L, &LuaTF1::SetParameter, "SetParameter");
	AddClassMethod(L, &LuaTF1::SetParameters, "SetParameters");

	AddClassMethod(L, &LuaTF1::SetParLimits, "SetParLimits");

	AddClassMethod(L, &LuaTF1::SetParError, "SetParError");
	AddClassMethod(L, &LuaTF1::SetParErrors, "SetParErrors");

	AddClassMethod(L, &LuaTF1::SetParName, "SetParName");

	AddClassMethod(L, &LuaTF1::FixParameter, "FixParameter");

	AddClassMethod(L, &LuaTF1::GetParameter, "GetParameter");
	AddClassMethod(L, &LuaTF1::GetParameters, "GetParameters");

	AddClassMethod(L, &LuaTF1::GetParName, "GetParName");
	AddClassMethod(L, &LuaTF1::GetParNumber, "GetParNumber");

	AddClassMethod(L, &LuaTF1::GetParError, "GetParError");
	AddClassMethod(L, &LuaTF1::GetParErrors, "GetParErrors");

	AddClassMethod(L, &LuaTF1::GetParLimits, "GetParLimits");

	AddClassMethod(L, &LuaTF1::SetRange, "SetRange");
	AddClassMethod(L, &LuaTF1::SetNpx, "SetNpx");

	AddClassMethod(L, &LuaTF1::GetChi2, "GetChi2");

	AddClassMethod(L, &LuaTF1::Eval, "Eval");
	AddClassMethod(L, &LuaTF1::GetX, "GetX");

	AddClassMethod(L, &LuaTF1::Integral, "Integral");

	AddClassMethod(L, &LuaTF1::GetRandom, "GetRandom");

	AddClassMethod(L, &LuaTF1::DoDraw, "Draw");
	AddClassMethod(L, &LuaTF1::DoUpdate, "Update");
	AddClassMethod(L, &LuaTF1::DoWrite, "Write");

	AddClassMethod(L, &LuaTF1::IsValid, "IsValid");
}

void LuaTF1::AddNonClassMethods(lua_State* L)
{
	AddMethod(L, LuaTObjectSetName<LuaTF1>, "SetName");
	AddMethod(L, LuaTObjectGetName<LuaTF1>, "GetName");
}

extern "C" void LoadLuaTF1Lib(lua_State* L)
{
	MakeAccessFunctions<LuaTF1>(L, "TF1");
	rootObjectAliases["TF1"] = "TF1";

	AddObjectConstructor<LuaTF1, string, string>(L, "TF1");
	AddObjectConstructor<LuaTF1, string, string, double, double>(L, "TF1");
	AddObjectConstructor<LuaTF1, string, string, double, double, int>(L, "TF1");
	AddObjectConstructor<LuaTF1, string, string, double, double, int, int>(L, "TF1");
}
