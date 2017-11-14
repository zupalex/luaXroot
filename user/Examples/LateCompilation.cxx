#include <iostream>
#include <vector>

#include "UserClassBase.h" // Only mandatory include

using namespace std;

class MyLateClass: public LuaUserClass {// Don't forget to make your class derived from LuaUserClass (required to use some wrapper function below)
										// Instead of adding dependencies to ROOT classes, it is best to add a member pointer to the class you want (see sources in ROOT_binder for examples)
private:

public:
	MyLateClass()
	{
	}

	MyLateClass(int i, double d, vector<float> v, vector<unsigned short> a)
	{
		anInt = i;
		aDouble = d;
		aVector = v;
		for (int i = 0; i < min((int) a.size(), 8); i++)  // array are not supported by the automatic parser so use this trick if you have to manipulate arrays
			anArray[i] = a[i];
	}

	~MyLateClass()
	{
	}

	int anInt = 0;
	double aDouble = 0;
	vector<float> aVector;
	unsigned short anArray[8];

	void Reset()  // The Reset method doesn't need to be added in the MakeAccessor as it is automatically added in the call to MakeAccessFunctions<ClassName>(L, "ClassName") - see below
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
		for (int i = 0; i < min((int) a.size(), 8); i++)  // array are not supported by the automatic parser so use this trick if you have to manipulate arrays
			anArray[i] = a[i];
	}

	int GetInt() const
	{
		return anInt;
	}

	void MakeAccessors(lua_State* L)
	{
		AddAccessor(L, &anInt, "anInt", "int");  // Expose the member anInt to be accessed in the interpreter or Lua scripts using object:Get("anInt") or object:Set("anInt", value)
		AddAccessor(L, &aDouble, "aDouble", "double");  // Same as above but for member aDouble
		AddAccessor(L, &aVector, "aVector", "vector<float>");  // Same as above but for member aVector
		AddAccessor(L, anArray, "anArray", "unsigned short[8]");  // Same as above but for member anArray. Note that you do not need the & here as you pass already the address.

		AddClassMethod(L, &MyLateClass::SetMembers, "SetMembers");  // Expose the method SetMember to be accessed in the interpreter or Lua scripts using object:SetMember(arguments...)
		AddClassMethod(L, &MyLateClass::GetInt, "GetInt");
	}
};

extern "C" int openlib_late_compile(lua_State* L)
{
	MakeAccessFunctions<MyLateClass>(L, "MyLateClass");  // Helper function to setup you class so it can be used within Lua scripts or directly in the interpreter
														 // using either object = ClassName() or object = New("ClassName").

	AddObjectConstructor<MyLateClass, int, double, vector<float>, vector<unsigned short>>(L, "MyLateClass");  // Helper function to add non default constructor (constructors with arguments).

	return 0;
}
