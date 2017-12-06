#include <iostream>
#include <vector>

#include "LuaRootClasses.h"

using namespace std;

void LuaTH1::DoFill(double x, double w)
{
	((TH1D*) rootObj)->Fill(x, w);
}

void LuaTH1::Reset()
{
	((TH1D*) rootObj)->Reset();
}

void LuaTH1::Rebuild()
{
	((TH1D*) rootObj)->Rebuild();
}

void LuaTH1::Scale(double s)
{
	((TH1D*) rootObj)->Scale(s);
}

void LuaTH1::SetRangeUserX(double xmin, double xmax)
{
	((TH1D*) rootObj)->SetAxisRange(xmin, xmax);
}

void LuaTH1::SetRangeUserY(double ymin, double ymax)
{
	((TH1D*) rootObj)->SetAxisRange(ymin, ymax, "Y");
}

void LuaTH1::SetXProperties(int nbinsx, double xmin, double xmax)
{
//	((TH1D*) rootObj)->SetBins(nbinsx, xmin, xmax);
	const char* hname = rootObj->GetName();
	TH1D* newHist = new TH1D("temp_hist_setxprop", rootObj->GetTitle(), nbinsx, xmin, xmax);

	for (int i = 1; i < ((TH1D*) rootObj)->GetNbinsX(); i++)
	{
		int bcontent = ((TH1D*) rootObj)->GetBinContent(i);
		if (bcontent > 0)
		{
			double prev_bin_center = ((TH1D*) rootObj)->GetXaxis()->GetBinCenter(i);
			newHist->SetBinContent(newHist->FindBin(prev_bin_center), bcontent);
		}
	}

	LuaCanvas* canv = 0;

	auto itr = canvasTracker.find(rootObj);
	if (itr != canvasTracker.end())
	{
		canv = itr->second;
		canvasTracker.erase(itr);
	}

	delete rootObj;

	rootObj = newHist;
	((TH1D*) rootObj)->SetName(hname);

	if (canv != nullptr) canvasTracker[rootObj] = canv;
}

tuple<int, double, double> LuaTH1::GetXProperties()
{
	return make_tuple(((TH1D*) rootObj)->GetNbinsX(), ((TH1D*) rootObj)->GetXaxis()->GetXmin(), ((TH1D*) rootObj)->GetXaxis()->GetXmax());
}

tuple<vector<double>, vector<int>> LuaTH1::GetContent()
{
	vector<double> xvals;
	vector<int> bincontents;

	for (int i = 1; i <= ((TH1D*) rootObj)->GetNbinsX(); i++)
	{
		int bcontent = ((TH1D*) rootObj)->GetBinContent(i);
		if (bcontent > 0)
		{
			bincontents.push_back(bcontent);
			xvals.push_back(((TH1D*) rootObj)->GetXaxis()->GetBinCenter(i));
		}
	}

	return make_tuple(xvals, bincontents);
}

void LuaTH1::SetLogScale(string axis, bool val)
{
	theApp->NotifyUpdatePending();
	LuaCanvas* can = canvasTracker[rootObj];

	if (can != nullptr)
	{
		if (axis == "X") can->SetLogx(val);
		else if (axis == "Y") can->SetLogy(val);
		else if (axis == "Z") can->SetLogz(val);
	}

	can->Modified();
	can->Update();

	theApp->NotifyUpdateDone();
}

double LuaTH1::Integral(double xmin, double xmax)
{
	if (xmin != xmax)
	{
		TAxis* xaxis = ((TH1D*) rootObj)->GetXaxis();

		int minbin = xaxis->FindBin(xmin);
		int maxbin = xaxis->FindBin(xmax);

		return ((TH1D*) rootObj)->Integral(minbin, maxbin, "width");
	}
	else
	{
		return ((TH1D*) rootObj)->Integral("width");
	}
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

	AddClassMethod(L, &LuaTH1::SetXProperties, "SetXProperties");
	AddClassMethod(L, &LuaTH1::GetXProperties, "GetXProperties");

	AddClassMethod(L, &LuaTH1::GetContent, "GetContent");

	AddClassMethod(L, &LuaTH1::SetLogScale, "SetLogScale");

	AddClassMethod(L, &LuaTH1::Integral, "Integral");

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
	((TH2D*) rootObj)->Fill(x, y, w);
}

void LuaTH2::Reset()
{
	((TH2D*) rootObj)->Reset();
}

void LuaTH2::Rebuild()
{
	((TH2D*) rootObj)->Rebuild();
}
void LuaTH2::Scale(double s)
{
	((TH2D*) rootObj)->Scale(s);
}

void LuaTH2::SetRangeUserX(double xmin, double xmax)
{
	((TH2D*) rootObj)->SetAxisRange(xmin, xmax);
}

void LuaTH2::SetRangeUserY(double ymin, double ymax)
{
	((TH2D*) rootObj)->SetAxisRange(ymin, ymax, "Y");
}

void LuaTH2::ProjectX(double ymin, double ymax)
{
	TAxis* yax = ((TH2D*) rootObj)->GetYaxis();
	projX = ((TH2D*) rootObj)->ProjectionX(((string) rootObj->GetName() + (string) "_projX").c_str(), yax->FindBin(ymin), yax->FindBin(ymax));

	LuaDrawTObject(projX);
}

void LuaTH2::ProjectY(double xmin, double xmax)
{
	TAxis* xax = ((TH2D*) rootObj)->GetXaxis();
	projY = ((TH2D*) rootObj)->ProjectionY(((string) rootObj->GetName() + (string) "_projY").c_str(), xax->FindBin(xmin), xax->FindBin(xmax));

	LuaDrawTObject(projY);
}

void LuaTH2::SetXProperties(int nbinsx, double xmin, double xmax)
{
//	((TH2D*) rootObj)->SetBins(nbinsx, xmin, xmax, ((TH2D*) rootObj)->GetNbinsY(), ((TH2D*) rootObj)->GetYaxis()->GetXmin(), ((TH2D*) rootObj)->GetYaxis()->GetXmax());
	const char* hname = rootObj->GetName();
	TH2D* newHist = new TH2D("temp_hist_setxprop", rootObj->GetTitle(), nbinsx, xmin, xmax, ((TH2D*) rootObj)->GetNbinsY(), ((TH2D*) rootObj)->GetYaxis()->GetXmin(),
			((TH2D*) rootObj)->GetYaxis()->GetXmax());

	for (int i = 1; i <= ((TH2D*) rootObj)->GetNbinsX(); i++)
	{
		for (int j = 1; j <= ((TH2D*) rootObj)->GetNbinsY(); j++)
		{
			int bcontent = ((TH2D*) rootObj)->GetBinContent(i, j);
			if (bcontent > 0)
			{
				double prev_bin_centerx = ((TH2D*) rootObj)->GetXaxis()->GetBinCenter(i);
				double prev_bin_centery = ((TH2D*) rootObj)->GetYaxis()->GetBinCenter(j);
				newHist->SetBinContent(newHist->FindBin(prev_bin_centerx, prev_bin_centery), bcontent);
			}
		}
	}

	LuaCanvas* canv = 0;

	auto itr = canvasTracker.find(rootObj);
	if (itr != canvasTracker.end())
	{
		canv = itr->second;
		canvasTracker.erase(itr);
	}

	delete rootObj;

	rootObj = newHist;
	((TH2D*) rootObj)->SetName(hname);

	if (canv != nullptr) canvasTracker[rootObj] = canv;
}

void LuaTH2::SetYProperties(int nbinsy, double ymin, double ymax)
{
//	((TH2D*) rootObj)->SetBins(((TH2D*) rootObj)->GetNbinsX(), ((TH2D*) rootObj)->GetXaxis()->GetXmin(), ((TH2D*) rootObj)->GetXaxis()->GetXmax(), nbinsy, ymin, ymax);
	const char* hname = rootObj->GetName();
	TH2D* newHist = new TH2D("temp_hist_setxprop", rootObj->GetTitle(), ((TH2D*) rootObj)->GetNbinsX(), ((TH2D*) rootObj)->GetXaxis()->GetXmin(),
			((TH2D*) rootObj)->GetXaxis()->GetXmax(), nbinsy, ymin, ymax);

	for (int i = 1; i <= ((TH2D*) rootObj)->GetNbinsX(); i++)
	{
		for (int j = 1; j <= ((TH2D*) rootObj)->GetNbinsY(); j++)
		{
			int bcontent = ((TH2D*) rootObj)->GetBinContent(i, j);
			if (bcontent > 0)
			{
				double prev_bin_centerx = ((TH2D*) rootObj)->GetXaxis()->GetBinCenter(i);
				double prev_bin_centery = ((TH2D*) rootObj)->GetYaxis()->GetBinCenter(j);
				newHist->SetBinContent(newHist->FindBin(prev_bin_centerx, prev_bin_centery), bcontent);
			}
		}
	}

	LuaCanvas* canv = 0;

	auto itr = canvasTracker.find(rootObj);
	if (itr != canvasTracker.end())
	{
		canv = itr->second;
		canvasTracker.erase(itr);
	}

	delete rootObj;

	rootObj = newHist;
	((TH2D*) rootObj)->SetName(hname);
	if (canv != nullptr) canvasTracker[rootObj] = canv;
}

tuple<int, double, double> LuaTH2::GetXProperties()
{
	return make_tuple(((TH2D*) rootObj)->GetNbinsX(), ((TH2D*) rootObj)->GetXaxis()->GetXmin(), ((TH2D*) rootObj)->GetXaxis()->GetXmax());
}

tuple<int, double, double> LuaTH2::GetYProperties()
{
	return make_tuple(((TH2D*) rootObj)->GetNbinsY(), ((TH2D*) rootObj)->GetYaxis()->GetXmin(), ((TH2D*) rootObj)->GetYaxis()->GetXmax());
}

void LuaTH2::SetLogScale(string axis, bool val)
{
	theApp->NotifyUpdatePending();
	LuaCanvas* can = canvasTracker[rootObj];

	if (can != nullptr)
	{
		if (axis == "X") can->SetLogx(val);
		else if (axis == "Y") can->SetLogy(val);
		else if (axis == "Z") can->SetLogz(val);
	}

	can->Modified();
	can->Update();

	theApp->NotifyUpdateDone();
}

double LuaTH2::Integral(double xmin, double xmax, double ymin, double ymax)
{
	if (xmin != xmax && ymin != ymax)
	{
		TAxis* xaxis = ((TH2D*) rootObj)->GetXaxis();
		int minbinx = xaxis->FindBin(xmin);
		int maxbinx = xaxis->FindBin(xmax);

		TAxis* yaxis = ((TH2D*) rootObj)->GetYaxis();
		int minbiny = yaxis->FindBin(ymin);
		int maxbiny = yaxis->FindBin(ymax);

		return ((TH2D*) rootObj)->Integral(minbinx, maxbinx, minbiny, maxbiny, "width");
	}
	else
	{
		return ((TH2D*) rootObj)->Integral("width");
	}
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

	AddClassMethod(L, &LuaTH2::ProjectX, "ProjectX");
	AddClassMethod(L, &LuaTH2::ProjectY, "ProjectY");

	AddClassMethod(L, &LuaTH2::SetXProperties, "SetXProperties");
	AddClassMethod(L, &LuaTH2::SetYProperties, "SetYProperties");

	AddClassMethod(L, &LuaTH2::GetXProperties, "GetXProperties");
	AddClassMethod(L, &LuaTH2::GetYProperties, "GetYProperties");

	AddClassMethod(L, &LuaTH2::SetLogScale, "SetLogScale");

	AddClassMethod(L, &LuaTH2::Integral, "Integral");

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
	rootObjectAliases["TH1"] = "TH1";

	AddObjectConstructor<LuaTH1, string, string, int, int, int>(L, "TH1");

	MakeAccessFunctions<LuaTH2>(L, "TH2");
	rootObjectAliases["TH2F"] = "TH2";
	rootObjectAliases["TH2D"] = "TH2";
	rootObjectAliases["TH2"] = "TH2";

	AddObjectConstructor<LuaTH2, string, string, int, int, int, int, int, int>(L, "TH2");
}
