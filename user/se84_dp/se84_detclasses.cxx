#include <iostream>
#include <vector>

#include "UserClassBase.h"

using namespace std;

// ----------------- SIDAR Class ------------------------- //

class SIDAR_detclass: public LuaUserClass {
// This class is for ASICs hit
	public:
		SIDAR_detclass()
		{
		}

		int detID = -1;

		vector<short> dE_strips;
		vector<float> dE_energies;

		vector<short> E_strips;
		vector<float> E_energies;

		void Reset()
		{
			dE_strips.clear();
			dE_energies.clear();
			E_strips.clear();
			E_energies.clear();
		}

		string to_string() const
		{
			cout << "SIDAR_detclass::to_string => " << endl;

			cout << "dE_strips = ";
			for (unsigned int i = 0; i < dE_strips.size(); i++)
			{
				cout << dE_strips[i] << " ";
			}
			cout << endl;

			cout << "dE_energies = ";
			for (unsigned int i = 0; i < dE_energies.size(); i++)
			{
				cout << dE_energies[i] << " ";
			}
			cout << endl;

			cout << "E_strips = ";
			for (unsigned int i = 0; i < E_strips.size(); i++)
			{
				cout << E_strips[i] << " ";
			}
			cout << endl;

			cout << "E_energies = ";
			for (unsigned int i = 0; i < E_energies.size(); i++)
			{
				cout << E_energies[i] << " ";
			}
			cout << endl;

			char* dump = new char[512];

			sprintf(dump, "SIDAR_detclass %d %d %d %d", (int) dE_strips.size(), (int) dE_energies.size(), (int) E_strips.size(), (int) E_energies.size());

			return (string) dump;
		}

		void MakeAccessors(lua_State* L)
		{
			AddAccessor(L, &detID, "detID", "int");
			AddAccessor(L, &dE_strips, "dE_strips", "vector<short>");
			AddAccessor(L, &dE_energies, "dE_energies", "vector<float>");
			AddAccessor(L, &E_strips, "E_strips", "vector<short>");
			AddAccessor(L, &E_energies, "E_energies", "vector<float>");
		}
};

// ----------------- Barrel Class ------------------------- //

class Barrel_detclass: public LuaUserClass {
// This class is for ASICs hit
	public:
		Barrel_detclass()
		{
		}

		int detID = -1;

		vector<short> dE_strips;
		vector<float> dE_energies;

		vector<short> E_front_contacts;
		vector<float> E_front_energies;

		vector<short> E_back_strips;
		vector<float> E_back_energies;

		void Reset()
		{
			dE_strips.clear();
			dE_energies.clear();
			E_front_contacts.clear();
			E_front_energies.clear();
			E_back_strips.clear();
			E_back_energies.clear();
		}

		void MakeAccessors(lua_State* L)
		{
			AddAccessor(L, &detID, "detID", "int");
			AddAccessor(L, &dE_strips, "dE_strips", "vector<short>");
			AddAccessor(L, &dE_energies, "dE_energies", "vector<float>");
			AddAccessor(L, &E_front_contacts, "E_front_contacts", "vector<short>");
			AddAccessor(L, &E_front_energies, "E_front_energies", "vector<float>");
			AddAccessor(L, &E_back_strips, "E_back_strips", "vector<short>");
			AddAccessor(L, &E_back_energies, "E_back_energies", "vector<float>");
		}
};

// ----------------- Ion Chamber Class ------------------------- //

class IonChamber_detclass: public LuaUserClass {
// This class is for ASICs hit
	public:
		IonChamber_detclass()
		{
		}

		vector<short> pads;
		vector<float> energies;

		float average_energy;

		void Reset()
		{
			average_energy = 0;
			pads.clear();
			energies.clear();
		}

		void MakeAccessors(lua_State* L)
		{
			AddAccessor(L, &pads, "pads", "vector<short>");
			AddAccessor(L, &energies, "energies", "vector<float>");
			AddAccessor(L, &average_energy, "average_energy", "float");
		}
};

// ----------------- CRDC Class ------------------------- //

class CRDC_detclass: public LuaUserClass {
// This class is for ASICs hit
	public:
		CRDC_detclass()
		{
		}

//		vector<short> pads;
//		vector<vector<short>> sample_nbr;
//		vector<vector<float>> raw;

		float time;
		float average_raw;
		float xgrav;

		void Reset()
		{
//			pads.clear();
//			sample_nbr.clear();
//			raw.clear();
			time = 0;
			xgrav = 0;
			average_raw = 0;
		}

		void MakeAccessors(lua_State* L)
		{
//			AddAccessor(L, &pads, "pads", "vector<short>");
//			AddAccessor(L, &sample_nbr, "sample_nbr", "vector<vector<short>>");
//			AddAccessor(L, &raw, "raw", "vector<vector<float>>");
			AddAccessor(L, &time, "time", "float");
			AddAccessor(L, &average_raw, "average_raw", "float");
			AddAccessor(L, &xgrav, "xgrav", "float");
		}
};

// ----------------- MTDC Class ------------------------- //

class MTDC_detclass: public LuaUserClass {
// This class is for ASICs hit
	public:
		MTDC_detclass()
		{
		}

		vector<unsigned int> e1up_hits;
		vector<unsigned int> e1down_hits;
		vector<unsigned int> xf_hits;
		vector<unsigned int> rf_hits;

		void Reset()
		{
			e1up_hits.clear();
			e1down_hits.clear();
			xf_hits.clear();
			rf_hits.clear();
		}

		void MakeAccessors(lua_State* L)
		{
			AddAccessor(L, &xf_hits, "xf_hits", "vector<unsigned int>");
			AddAccessor(L, &e1up_hits, "e1up_hits", "vector<unsigned int>");
			AddAccessor(L, &e1down_hits, "e1down_hits", "vector<unsigned int>");
			AddAccessor(L, &rf_hits, "rf_hits", "vector<unsigned int>");
		}
};

// ----------------- Scintillators Class ------------------------- //

class Scintillators_detclass: public LuaUserClass {
// This class is for ASICs hit
	public:
		Scintillators_detclass()
		{
		}

		vector<float> up;
		vector<float> down;

		void Reset()
		{
			up.clear();
			down.clear();
		}

		void MakeAccessors(lua_State* L)
		{
			AddAccessor(L, &up, "up", "vector<float>");
			AddAccessor(L, &down, "down", "vector<float>");
		}
};

extern "C" int openlib_se84_detclasses(lua_State* L)
{
	MakeAccessFunctions<SIDAR_detclass>(L, "SIDAR_detclass");
	MakeAccessFunctions<Barrel_detclass>(L, "Barrel_detclass");
	MakeAccessFunctions<IonChamber_detclass>(L, "IonChamber_detclass");
	MakeAccessFunctions<CRDC_detclass>(L, "CRDC_detclass");
	MakeAccessFunctions<MTDC_detclass>(L, "MTDC_detclass");
	MakeAccessFunctions<Scintillators_detclass>(L, "Scintillators_detclass");
	return 0;
}

#ifdef __CINT__

#pragma link C++ class SIDAR_detclass+;
#pragma link C++ class vector<SIDAR_detclass>+;
#pragma link C++ class Barrel_detclass+;
#pragma link C++ class vector<Barrel_detclass>+;
#pragma link C++ class IonChamber_detclass+;
#pragma link C++ class CRDC_detclass+;
#pragma link C++ class MTDC_detclass+;
#pragma link C++ class Scintillators_detclass+;

#endif
