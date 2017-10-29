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

    void MakeAccessors ()
    {
        AddAccessor ( &anInt, "anInt" );
        AddAccessor ( &aDouble, "aDouble" );
        AddAccessor ( &aVector, "aVector" );
        AddAccessor ( anArray, "anArray[8]" );
    }
};

int MyLateClass_luactor ( lua_State* L )
{
    MyLateClass* obj = * ( NewUserData<MyLateClass> ( L ) );
    obj->MakeMetatable(L);

    return 1;
}

static const luaL_Reg late_compile_funcs [] =
{
    {"MyLateClass", MyLateClass_luactor},

    {NULL, NULL}
};

extern "C" int luaopen_late_compile ( lua_State* L )
{
    lua_getglobal ( L, "_G" );
    luaL_setfuncs ( L, late_compile_funcs, 0 );
    lua_pop ( L, 1 );

    MakeTTreeFunctions<MyLateClass>("MyLateClass");

    return 0;
}

#ifdef __CINT__

#pragma link C++ class MyLateClass+;
#pragma link C++ class vector<MyLateClass>+;

#endif

