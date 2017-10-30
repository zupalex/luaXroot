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

    map<string, function<void ( lua_State* ) >> getters; //!
    map<string, function<void ( lua_State* ) >> setters; //!

    void SetupMetatable ( lua_State* L );
    virtual void MakeAccessors () = 0;

    template<typename T> void AddGetter ( T* src, string name, string type, int arraySize )
    {
        getters[name] = [=] ( lua_State* L )
        {
            T** ud = NewUserData<T> ( L );
            *ud = src;

            MakeMetatable ( L );

            lua_pushfstring ( L, type.c_str() );
            lua_setfield ( L, -2, "type" );

            lua_pushinteger ( L, arraySize );
            lua_setfield ( L, -2, "array_size" );

            AddMethod ( L, luaExt_SetUserDataValue, "Set" );
            AddMethod ( L, luaExt_GetUserDataValue, "Get" );

            if ( type.find ( "vector" ) != string::npos )
            {
                AddMethod ( L, luaExt_SetUserDataValue, "PushBack" );
            }
        };
    }

    template<typename T> void AddAccessor ( T* member, string name, string type )
    {
        size_t findIfArray = type.find ( "[" );
        int arraySize = 0;

        if ( findIfArray != string::npos )
        {
            size_t endArraySize = type.find ( "]" );
            arraySize = stoi ( type.substr ( findIfArray+1, endArraySize-findIfArray-1 ) );
            type = type.substr ( 0, findIfArray );
            type += "[]";
        }

        AddGetter ( member, name, type, arraySize );
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




