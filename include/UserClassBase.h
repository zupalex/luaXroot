#ifndef __USERCLASSBASE__
#define __USERCLASSBASE__

#include "LuaRootBinder.h"
#include "TObject.h"

class LuaUserClass : public TObject
{
private:

protected:

public:
    LuaUserClass() {}
    virtual ~LuaUserClass() {}

    void SetupMetatable ( lua_State* L );
    virtual void MakeAccessors ( lua_State* L ) = 0;

    template<typename T> void AddAccessor ( lua_State* L, T* member, string name, string type )
    {
        lua_pushstring ( L, type.c_str() );
        lua_insert ( L, 1 );
        luaExt_NewUserData ( L );

        T** ud = GetUserDataPtr<T> ( L, -1 );
        *ud = member;

        lua_setfield ( L, -2, name.c_str() );
    }

    ClassDef ( LuaUserClass, 1 )
};

int GetMember ( lua_State* L );
int SetMember ( lua_State* L );
int GetMemberValue ( lua_State* L );

template<typename T> void MakeAccessFunctions ( lua_State* L, string type_name )
{
    lua_getglobal ( L, "MakeCppClassCtor" );
    lua_pushstring ( L, type_name.c_str() );
    lua_pcall ( L, 1, 1, 0 );

    MakeAccessorsUserDataFuncs<T> ( type_name );

    newBranchFns[type_name] = [=] ( lua_State* L_, TTree* tree, const char* bname )
    {
        luaExt_NewUserData ( L_ );
        T* branch_ptr = GetUserData<T> ( L_, -1, "luaExt_TTree_NewBranch" );
        tree->Branch ( bname, branch_ptr );
    };

    newBranchFns["vector<" + type_name + ">"] = [=] ( lua_State* L_, TTree* tree, const char* bname )
    {
        luaExt_NewUserData ( L_ );
        vector<T>* branch_ptr = GetUserData<vector<T>> ( L_, -1, "luaExt_TTree_NewBranch" );
        tree->Branch ( bname, branch_ptr );
    };
}

#endif




