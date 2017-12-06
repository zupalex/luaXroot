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

	for (unsigned int i = 0; i < params.size(); i++)
	{
		cout << params[i] << endl;
	}

	((TF1*) rootObj)->SetParameters(&params[0]);

	if (canvasTracker[rootObj] != nullptr)
	{
		canvasTracker[rootObj]->Modified();
		canvasTracker[rootObj]->Update();
	}

	theApp->NotifyUpdateDone();
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

	AddClassMethod(L, &LuaTF1::GetParameter, "GetParameter");
	AddClassMethod(L, &LuaTF1::GetParameters, "GetParameters");

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
