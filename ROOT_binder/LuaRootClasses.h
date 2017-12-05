#ifndef __LUAROOTCLASSES__
#define __LUAROOTCLASSES__

#include "UserClassBase.h"

void LuaDrawTObject(TObject* obj, string opts = "");

template<typename T> class LuaROOTBase : public LuaUserClass {
	private:

	public:
		LuaROOTBase()
		{
		}

		virtual ~LuaROOTBase()
		{
		}

		TObject* rootObj = 0;
		typedef T type;

		virtual TObject* GetROOTObject()
		{
			return rootObj;
		}

		virtual void SetROOTObject(TObject* obj)
		{
			rootObj = obj;
		}

		void SetTitle(string title)
		{
			((T*) rootObj)->SetTitle(title.c_str());
		}

		string GetTitle()
		{
			return ((T*) rootObj)->GetTitle();
		}

		TAxis* GetXaxis()
		{
			return ((T*) rootObj)->GetXaxis();
		}

		void Fit(TF1* fitfunc, string opts = "", string gopts = "", double xmin = 0, double xmax = 0)
		{
			((T*) rootObj)->Fit(fitfunc, opts.c_str(), gopts.c_str(), xmin, xmax);
		}

		void Add(LuaROOTBase<T>* h2, double s)
		{
			((T*) rootObj)->Add((T*) (h2->rootObj), s);
		}

		virtual void Draw(string varexp, string cond, string opts, unsigned long long nentries, unsigned long long firstentry)
		{
			rootObj->Draw(varexp.c_str());
		}

		virtual void DoDraw(string varexp, string cond = "", string opts = "", unsigned long long nentries = numeric_limits<unsigned long long>::max(), unsigned long long firstentry =
				0)
		{
			theApp->NotifyUpdatePending();

			if (!is_same<T, TTree>::value) opts = varexp;

			if (canvasTracker[rootObj] != nullptr && ((string) canvasTracker[rootObj]->GetName()).empty()) delete canvasTracker[rootObj];

			if (canvasTracker[rootObj] == nullptr || ((string) canvasTracker[rootObj]->GetName()).empty())
			{
				if (opts.find("same") == string::npos && opts.find("SAME") == string::npos)
				{
					LuaCanvas* disp = new LuaCanvas();
					disp->cd();
					canvasTracker[rootObj] = disp;
					disp->SetTitle(rootObj->GetName());
				}
				else
				{
					canvasTracker[rootObj] = (LuaCanvas*) gPad->GetCanvas();
				}

				Draw(varexp, cond, opts, nentries, firstentry);
				canvasTracker[rootObj]->Update();
			}
			else
			{
				canvasTracker[rootObj]->cd();
				canvasTracker[rootObj]->SetTitle(rootObj->GetName());
				Draw(varexp, cond, opts, nentries, firstentry);
//			if (dynamic_cast<TH1*>(rootObj) != nullptr) ((TH1*) rootObj)->Rebuild();
//			canvasTracker[rootObj]->Modified();
				canvasTracker[rootObj]->Update();
			}

			theApp->NotifyUpdateDone();
		}

		void DoUpdate()
		{
			theApp->NotifyUpdatePending();

			if (canvasTracker[rootObj] != nullptr)
			{
				canvasTracker[rootObj]->Modified();
				canvasTracker[rootObj]->Update();
			}

			theApp->NotifyUpdateDone();
		}

		void DoWrite()
		{
			((T*) rootObj)->Write();
		}
};

extern "C" int openlib_lua_root_classes(lua_State* L);

template<typename T> int LuaTObjectSetName(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "LuaTObjectSetName", LUA_TUSERDATA, LUA_TSTRING)) return 0;

	T* obj = GetUserData<T>(L);
	string name;
	name = lua_tostring(L, 2);

	if (dynamic_cast<TNamed*>(obj->rootObj) != nullptr) ((TNamed*) obj->rootObj)->SetName(name.c_str());

	return 0;
}

template<typename T> int LuaTObjectGetName(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "LuaTObjectSetName", LUA_TUSERDATA)) return 0;

	T* obj = GetUserData<T>(L);

	if (dynamic_cast<TNamed*>(obj->rootObj) != nullptr) lua_pushstring(L, ((TNamed*) obj->rootObj)->GetName());

	return 1;
}

template<typename T> int LuaTClone(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "LuaTClone", LUA_TUSERDATA)) return 0;

	T* obj = GetUserData<T>(L);

	lua_getglobal(L, "New");
	lua_getfield(L, -2, "type");

	string type = lua_tostring(L, -1);

	lua_pcall(L, 1, 1, 0);

	T* clone = GetUserData<T>(L, -1);

	clone->rootObj = obj->rootObj->Clone();

	return 1;
}

int LuaGetROOTObjectFromDir(lua_State* L);
int luaExt_GetGDirContent(lua_State* L);

// _____________________________________________________________________________ //
//                                                                               //
// -------------------------------- TCanvas ------------------------------------ //
// _____________________________________________________________________________ //

extern "C" void LoadLuaTCanvasLib(lua_State* L);

class LuaTCanvas : public LuaROOTBase<LuaCanvas> {
	private:

	public:
		LuaTCanvas()
		{
			theApp->NotifyUpdatePending();
			rootObj = new LuaCanvas();
			theApp->NotifyUpdateDone();
		}

		~LuaTCanvas()
		{
		}

		int nrow = 0;
		int ncol = 0;

		map<LuaCanvas*, TObject*> dispObjs;

		void Close();
		void Clear();

		void Divide(int nrow_, int ncol_);

		void Draw(LuaUserClass* obj, int row, int col);
		void Update();

		void SetSize(int width, int height);
		void SetWindowSize(int width, int height);

		virtual void MakeAccessors(lua_State* L);
		virtual void AddNonClassMethods(lua_State* L);
};

// _____________________________________________________________________________ //
//                                                                               //
// -------------------------------- TFile -------------------------------------- //
// _____________________________________________________________________________ //

extern "C" void LoadLuaTFileLib(lua_State* L);

class LuaTFile : public LuaROOTBase<TFile> {
	private:

	public:
		LuaTFile()
		{
			rootObj = new TFile("temp", "recreate");
		}

		LuaTFile(string name, string opts)
		{
			rootObj = new TFile(name.c_str(), opts.c_str());
		}

		~LuaTFile()
		{
		}

		virtual void MakeActive();
		virtual void ListContent();

		void Close();
		void Open(string path, string opts);

		void GetObject(string type, string name);

		virtual void MakeAccessors(lua_State* L);
		virtual void AddNonClassMethods(lua_State* L);
};

// _____________________________________________________________________________ //
//																				 //
// --------------------------------- TF1 --------------------------------------- //
// _____________________________________________________________________________ //

extern "C" void LoadLuaTF1Lib(lua_State* L);

class LuaTF1 : public LuaROOTBase<TF1> {
	private:

	public:
		LuaTF1()
		{
			rootObj = new TF1();
		}

		LuaTF1(string name, string formula)
		{
			rootObj = new TF1(name.c_str(), formula.c_str());
		}

		LuaTF1(string name, string formula, double xmin, double xmax)
		{
			rootObj = new TF1(name.c_str(), formula.c_str(), xmin, xmax);
		}

		LuaTF1(string name, string fnname, double xmin, double xmax, int npar)
		{
			rootObj = new TF1(name.c_str(), registeredTF1fns[fnname], xmin, xmax, npar);
		}

		LuaTF1(string name, string fnname, double xmin, double xmax, int npar, int ndim)
		{
			rootObj = new TF1(name.c_str(), registeredTF1fns[fnname], xmin, xmax, npar, ndim);
		}

		~LuaTF1()
		{
		}

		virtual void SetParameter(int param, double value);
		virtual void SetParameters(vector<double> params);

		virtual double GetParameter(int param);
		virtual vector<double> GetParameters();
		double GetChi2();

		virtual double Eval(double x);

		virtual void MakeAccessors(lua_State* L);
		virtual void AddNonClassMethods(lua_State* L);
};

template<typename T> int LuaTFit(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "LuaTGraphFit", LUA_TUSERDATA, LUA_TTABLE)) return 0;

	T* obj = *(reinterpret_cast<T**>(lua_touserdata(L, 1)));

	lua_getfield(L, 2, "fn");

	if (!CheckLuaArgs(L, -1, true, "LuaTGraphFit argument table: field fn ", LUA_TUSERDATA)) return 0;

	LuaTF1* fitObj = GetUserData<LuaTF1>(L, -1);
	TF1* fitfunc = (TF1*) fitObj->rootObj;
	lua_pop(L, 1);

	TAxis* xAxis = obj->GetXaxis();

	double xmin, xmax;
	string opts = "";

	if (lua_checkfield(L, -1, "xmin", LUA_TNUMBER))
	{
		xmin = lua_tonumber(L, -1);
		lua_pop(L, 1);
	}
	else xmin = xAxis->GetXmin();

	if (lua_checkfield(L, -1, "xmax", LUA_TNUMBER))
	{
		xmax = lua_tonumber(L, -1);
		lua_pop(L, 1);
	}
	else xmax = xAxis->GetXmax();

	if (lua_checkfield(L, -1, "opts", LUA_TSTRING))
	{
		opts = lua_tostring(L, -1);
		lua_pop(L, 1);
	}

	obj->Fit((TF1*) fitfunc, opts.c_str(), "", xmin, xmax);
	if ( gPad != nullptr)
	{
		gPad->Update();
	}

	return 0;
}

// _____________________________________________________________________________ //
//																				 //
// -------------------------------- TGraph ------------------------------------- //
// _____________________________________________________________________________ //

extern "C" void LoadLuaTGraphLib(lua_State* L);

class LuaGraphError : public LuaROOTBase<TGraphErrors> {
	private:

	public:
		LuaGraphError()
		{
			rootObj = new TGraphErrors();
			SetGraphStyle();
		}

		LuaGraphError(int i)
		{
			rootObj = new TGraphErrors(i);
			SetGraphStyle();
		}

		LuaGraphError(int i, vector<double> xs, vector<double> ys)
		{
			rootObj = new TGraphErrors(i, &xs[0], &ys[0]);
			SetGraphStyle();
		}

		LuaGraphError(int i, vector<double> xs, vector<double> ys, vector<double> errxs, vector<double> errys)
		{
			rootObj = new TGraphErrors(i, &xs[0], &ys[0], &errxs[0], &errys[0]);
			SetGraphStyle();
		}

		~LuaGraphError()
		{
		}

		void SetGraphStyle();

		virtual void Set(int n);
		int GetMaxSize();

		tuple<double, double> GetPointVals(int i);
		tuple<double, double> GetPointErrors(int i);
		void SetPointVals(int i, double x, double y);
		void SetPointErrors(int i, double errx, double erry);

		int RemovePoint(int i);

		double Eval(double x);

		virtual void MakeAccessors(lua_State* L);
		virtual void AddNonClassMethods(lua_State* L);
};

// _____________________________________________________________________________ //
//																				 //
// ----------------------------- THistograms ----------------------------------- //
// _____________________________________________________________________________ //

template<typename T> int LuaAddHist(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "LuaAddHist", LUA_TUSERDATA, LUA_TUSERDATA)) return 0;

	T* h1 = GetUserData<T>(L, 1);
	T* h2 = GetUserData<T>(L, 2);

	double scale = lua_tonumberx(L, 3, nullptr);

	h1->Add(h2, scale > 0 ? scale : 1);

	return 0;
}

extern "C" void LoadLuaTHistLib(lua_State* L);

class LuaTH1 : public LuaROOTBase<TH1D> {
	private:

	public:
		LuaTH1()
		{
			rootObj = new TH1D();
		}

		LuaTH1(string name, string title, int nbinsx, int xmin, int xmax)
		{
			rootObj = new TH1D(name.c_str(), title.c_str(), nbinsx, xmin, xmax);
		}

		~LuaTH1()
		{
		}

		void DoFill(double x, double w);
		void Reset();
		void Rebuild();
		void Scale(double s);
		void SetRangeUserX(double xmin, double xmax);
		void SetRangeUserY(double ymin, double ymax);

		void SetXProperties(int nbinsx, double xmin, double xmax);
		tuple<int, double, double> GetXProperties();

		tuple<vector<double>, vector<int>> GetContent();

		virtual void MakeAccessors(lua_State* L);
		virtual void AddNonClassMethods(lua_State* L);
};

class LuaTH2 : public LuaROOTBase<TH2D> {
	private:

	public:
		LuaTH2()
		{
			rootObj = new TH2D();
		}

		LuaTH2(string name, string title, int nbinsx, int xmin, int xmax, int nbinsy, int ymin, int ymax)
		{
			rootObj = new TH2D(name.c_str(), title.c_str(), nbinsx, xmin, xmax, nbinsy, ymin, ymax);
		}

		~LuaTH2()
		{
		}

		TH1D* projX = 0;
		TH1D* projY = 0;

		void DoFill(double x, double y, double w);
		void Reset();
		void Rebuild();
		void Scale(double s);
		void SetRangeUserX(double xmin, double xmax);
		void SetRangeUserY(double ymin, double ymax);

		void SetXProperties(int nbinsx, double xmin, double xmax);
		void SetYProperties(int nbinsy, double ymin, double ymax);

		void ProjectX(double ymin, double ymax);
		void ProjectY(double xmin, double xmax);

		tuple<int, double, double> GetXProperties();
		tuple<int, double, double> GetYProperties();

		virtual void MakeAccessors(lua_State* L);
		virtual void AddNonClassMethods(lua_State* L);
};

// _____________________________________________________________________________ //
//																				 //
// ------------------------------- TSpectrum ----------------------------------- //
// _____________________________________________________________________________ //

extern "C" void LoadLuaTSpectrumLib(lua_State* L);

class LuaTSpectrum : public LuaROOTBase<TSpectrum> {
	private:

	public:
		LuaTSpectrum()
		{
			rootObj = new TSpectrum();
		}

		LuaTSpectrum(int maxposition)
		{
			rootObj = new TSpectrum(maxposition);
		}

		~LuaTSpectrum()
		{
		}

		TH1D* background = 0;

		void Background(string histname, int niter = 20, string opts = "");
		void DrawBackground();
		string GetBackgroundName();

		void Search(string histname, double sigma = 2, string opts = "", double threshold = 0.05);
		int GetNPeaks();
		vector<double> GetPositionX();
		vector<double> GetPositionY();

		virtual void MakeAccessors(lua_State* L);
		virtual void AddNonClassMethods(lua_State* L);
};

// _____________________________________________________________________________ //
//																				 //
// --------------------------------- TCutG ------------------------------------- //
// _____________________________________________________________________________ //

extern "C" void LoadLuaTCutGLib(lua_State* L);

class LuaTCutG : public LuaROOTBase<TCutG> {
	private:

	public:
		LuaTCutG()
		{
			rootObj = new TCutG();
		}

		~LuaTCutG()
		{
		}

		virtual int IsInside(double x, double y);

		virtual void MakeAccessors(lua_State* L);
		virtual void AddNonClassMethods(lua_State* L);
};

// _____________________________________________________________________________ //
//																				 //
// --------------------------------- TTree ------------------------------------- //
// _____________________________________________________________________________ //

extern "C" void LoadLuaTTreeLib(lua_State* L);

int luaExt_TTree_NewBranch_Interface(lua_State* L);
int luaExt_TTree_GetBranch_Interface(lua_State* L);

class LuaTTree : public LuaROOTBase<TTree> {
	private:

	public:
		LuaTTree()
		{
			rootObj = new TTree("tree", "tree");
		}

		LuaTTree(string name, string title)
		{
			rootObj = new TTree(name.c_str(), title.c_str());
		}

		~LuaTTree()
		{
		}

		unsigned long long GetEntries();

		void Fill();

		void GetEntry(unsigned long long entry);

		virtual void Draw(string varexp, string cond, string opts, unsigned long long nentries, unsigned long long firstentry);

		virtual void MakeAccessors(lua_State* L);
		virtual void AddNonClassMethods(lua_State* L);
};

#endif
