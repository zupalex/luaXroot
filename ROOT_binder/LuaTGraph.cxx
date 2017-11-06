#include <iostream>
#include <vector>

#include "UserClassBase.h"

using namespace std;

class LuaGraphError: public LuaUserClass, public TGraphErrors {
private:

public:
	LuaGraphError()
	{
	}

	LuaGraphError(int i) :
			TGraphErrors(i)
	{
	}

	~LuaGraphError()
	{
	}

	void Draw(string chopt = "")
	{
		TGraphErrors::Draw(chopt.c_str());
	}

	virtual void Set(int n)
	{
		TGraphErrors::Set(n);
	}

	int GetMaxSize()
	{
		return TGraphErrors::GetMaxSize();
	}

	void MakeAccessors(lua_State* L)
	{
		AddClassMethod(L, &LuaGraphError::Draw, "Draw");
		AddClassMethod(L, &LuaGraphError::GetMaxSize, "GetMaxSize");
		AddClassMethod(L, &LuaGraphError::Set, "SetNPoint");
	}
};

extern "C" int openlib_lua_root_classes(lua_State* L)
{
	MakeAccessFunctions<LuaGraphError>(L, "TGraph");
	AddObjectConstructor<LuaGraphError, int>(L, "TGraph");

	return 0;
}

#ifdef __CINT__

#pragma link C++ class LuaGraphError+;
#pragma link C++ class vector<LuaGraphError>+;

#endif
