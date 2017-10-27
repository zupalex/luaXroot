#include "UserClassBase.h"

void LuaUserClass::MakeMetatable ( lua_State* L )
{
    SetupMetatable ( L );

    AddMethod ( L, GetMember, "Get" );
    AddMethod ( L, SetMember, "Set" );
    
    MakeAccessors();
}

int GetMember ( lua_State* L )
{
    LuaUserClass* obj = GetUserData<LuaUserClass> ( L );

    string member = lua_tostringx ( L, 2 );

    if ( !member.empty() )
    {
        obj->getters[member] ( L );
    }

    return 1;
}

int SetMember ( lua_State* L )
{
    LuaUserClass* obj = GetUserData<LuaUserClass> ( L );

    string member = lua_tostringx ( L, 2 );
//
    if ( !member.empty() )
    {
        obj->setters[member] ( L );
    }

    return 0;
}
