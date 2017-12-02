#include <iostream>

#include "LuaRootClasses.h"

using namespace std;

void LuaTSpectrum::Background(string histname, int niter, string opts)
{
	TH1* hist = (TH1*) gDirectory->FindObject(histname.c_str());

	if (hist != nullptr) background = (TH1D*) ((TSpectrum*) rootObj)->Background(hist, niter, opts.c_str());
	background->SetName((histname + "_background").c_str());
}

void LuaTSpectrum::DrawBackground()
{
	if (background != nullptr) LuaDrawTObject(background);
}

string LuaTSpectrum::GetBackgroundName()
{
	return background->GetName();
}

void LuaTSpectrum::Search(string histname, double sigma, string opts, double threshold)
{
	TH1* hist = (TH1*) gDirectory->FindObject(histname.c_str());

	((TSpectrum*) rootObj)->Search(hist, sigma, opts.c_str(), threshold);
}

int LuaTSpectrum::GetNPeaks()
{
	return ((TSpectrum*) rootObj)->GetNPeaks();
}

vector<double> LuaTSpectrum::GetPositionX()
{
	vector<double> peaks_pos;
	double* retrieved_pos = ((TSpectrum*) rootObj)->GetPositionX();

	for (int i = 0; i < ((TSpectrum*) rootObj)->GetNPeaks(); i++)
		peaks_pos.push_back(retrieved_pos[i]);

	return peaks_pos;
}

vector<double> LuaTSpectrum::GetPositionY()
{
	vector<double> peaks_height;
	double* retrieved_height = ((TSpectrum*) rootObj)->GetPositionY();

	for (int i = 0; i < ((TSpectrum*) rootObj)->GetNPeaks(); i++)
		peaks_height.push_back(retrieved_height[i]);

	return peaks_height;
}

void LuaTSpectrum::MakeAccessors(lua_State* L)
{
	AddClassMethod(L, &LuaTSpectrum::Background, "Background");
	AddClassMethod(L, &LuaTSpectrum::DrawBackground, "DrawBackground");
	AddClassMethod(L, &LuaTSpectrum::GetBackgroundName, "GetBackgroundName");

	AddClassMethod(L, &LuaTSpectrum::Search, "Search");
	AddClassMethod(L, &LuaTSpectrum::GetNPeaks, "GetNPeaks");
	AddClassMethod(L, &LuaTSpectrum::GetPositionX, "GetPositionX");
	AddClassMethod(L, &LuaTSpectrum::GetPositionY, "GetPositionY");
}

void LuaTSpectrum::AddNonClassMethods(lua_State* L)
{

}

extern "C" void LoadLuaTSpectrumLib(lua_State* L)
{
	MakeAccessFunctions<LuaTSpectrum>(L, "TSpectrum");
	rootObjectAliases["TSpectrum"] = "TSpectrum";

	AddObjectConstructor<LuaTSpectrum, int>(L, "TSpectrum");
}
