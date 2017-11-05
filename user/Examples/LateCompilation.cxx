#include <iostream>
#include <vector>

#include "UserClassBase.h" // Only mandatory include

using namespace std;

class MyLateClass: public LuaUserClass { // Don't forget to make your class derived from LuaUserClass (required to use some wrapper function below)
private:

public:
	MyLateClass()
	{
	}
	~MyLateClass()
	{
	}

	int anInt = 0;
	double aDouble = 0;
	vector<float> aVector;
	unsigned short anArray[8];

	void Reset() // The Reset method doesn't need to be added in the MakeAccessor as it is automatically added in the call to MakeAccessFunctions<ClassName>(L, "ClassName") - see below
	{
		anInt = 0;
		aDouble = 0;
		aVector.clear();
		memset(anArray, 0, sizeof(anArray));
	}

	void SetMembers(int i, double d, vector<float> v, vector<unsigned short> a)
	{
		anInt = i;
		aDouble = d;
		aVector = v;
		for (int i = 0; i < min((int) a.size(), 8); i++) // array are not supported by the automatic parser so use this trick if you have to manipulate arrays
			anArray[i] = a[i];
	}

	void MakeAccessors(lua_State* L)
	{
		AddAccessor(L, &anInt, "anInt", "int"); // Expose the member anInt to be accessed in the interpreter or Lua scripts using object:Get("anInt") or object:Set("anInt", value)
		AddAccessor(L, &aDouble, "aDouble", "double"); // Same as above but for member aDouble
		AddAccessor(L, &aVector, "aVector", "vector<float>"); // Same as above but for member aVector
		AddAccessor(L, anArray, "anArray", "unsigned short[8]"); // Same as above but for member anArray. Note that you do not need the & here as you pass already the address.

		AddClassMethod(L, &MyLateClass::SetMembers, "SetMembers"); // Expose the method SetMember to be accessed in the interpreter or Lua scripts using object:SetMember(arguments...)
	}
};

extern "C" int openlib_late_compile(lua_State* L)
{
	MakeAccessFunctions<MyLateClass>(L, "MyLateClass"); // Wrapper function to setup you class so it can be used within Lua scripts or directly in the interpreter
														// using either object = ClassName() or object = New("ClassName")
	return 0;
}

// The following section is to allow ROOT to know about your class. The syntax is pretty straightforward.
#ifdef __CINT__

#pragma link C++ class MyLateClass+;
#pragma link C++ class vector<MyLateClass>+;

#endif

