#include <iostream>
#include <vector>

#include "UserClassBase.h"

using namespace std;

class MyLateClass : public LuaUserClass
{
private:

public:
    MyLateClass() {}
    ~MyLateClass() {}

    int anInt;
    double aDouble;
    vector<float> aVector;
    unsigned short anArray[8];

    void MakeAccessors ( lua_State* L )
    {
        AddAccessor ( L, &anInt, "anInt", "int" );
        AddAccessor ( L, &aDouble, "aDouble", "double" );
        AddAccessor ( L, &aVector, "aVector", "vector<float>" );
        AddAccessor ( L, anArray, "anArray", "unsigned short[8]" );
    }
};

extern "C" int openlib_late_compile ( lua_State* L )
{
    MakeAccessFunctions<MyLateClass> ( L, "MyLateClass" );

    return 0;
}

#ifdef __CINT__

#pragma link C++ class MyLateClass+;
#pragma link C++ class vector<MyLateClass>+;

#endif

