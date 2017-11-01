#include "UserClassBase.h"

void LuaUserClass::SetupMetatable ( lua_State* L )
{
    AddMethod ( L, GetMember, "Get" );
    AddMethod ( L, SetMember, "Set" );
    AddMethod ( L, GetMemberValue, "Value" );

    lua_newtable ( L );
    MakeAccessors ( L );
    lua_setfield ( L, -2, "members" );
}

int GetMember ( lua_State* L )
{
    string member = lua_tostringx ( L, 2 );

    if ( !member.empty() )
    {
        lua_getfield ( L, 1, "members" );
        lua_getfield ( L, -1, member.c_str() );
    }

    return 1;
}

int GetMemberValue ( lua_State* L )
{
    string member = lua_tostringx ( L, 2 );

    if ( !member.empty() )
    {
        lua_getfield ( L, 1, "members" );
        lua_getfield ( L, -1, member.c_str() );
        lua_getfield ( L, -1, "Get" );
        lua_insert ( L, -2 );
        lua_pcall ( L, 1, 1, 0 );
    }
    else lua_pushnil ( L );

    return 1;
}

int SetMember ( lua_State* L )
{
    string member = lua_tostringx ( L, 2 );
//
    if ( !member.empty() )
    {
        lua_getfield ( L, 1, "members" );
        lua_getfield ( L, -1, member.c_str() );
        lua_remove ( L, 1 );
        lua_insert ( L, 1 );
        lua_pop ( L, 1 );
        luaExt_SetUserDataValue ( L );
    }
    else
    {
        luaExt_SetUserDataValue ( L );
    }

    return 0;
}
