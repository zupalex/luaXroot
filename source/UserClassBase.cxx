#include "UserClassBase.h"

void LuaUserClass::MakeMetatable ( lua_State* L )
{
    SetupMetatable ( L );

    AddMethod ( L, GetMember, "Get" );
    AddMethod ( L, SetMember, "Set" );
    AddMethod ( L, GetMemberValue, "Value" );

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

int GetMemberValue ( lua_State* L )
{
    LuaUserClass* obj = GetUserData<LuaUserClass> ( L );

    string member = lua_tostringx ( L, 2 );

    if ( !member.empty() )
    {
        obj->getters[member] ( L );
        lua_getfield ( L, -1, "Get" );
        lua_insert ( L, -2 );
        lua_pcall ( L, 1, 1, 0 );
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
//         obj->setters[member] ( L );
        obj->getters[member] ( L );
        lua_remove ( L, 1 );
        lua_insert ( L, 1 );
        luaExt_SetUserDataValue ( L );
    }

    return 0;
}
