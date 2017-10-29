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

    void MakeMetatable ( lua_State* L );
    virtual void MakeAccessors () = 0;

    template<typename T> void AddGetter ( T* src, string name, string type, int arraySize )
    {
        getters[name] = [=] ( lua_State* L )
        {
            T** ud = NewUserData<T> ( L );
            *ud = src;

            SetupMetatable ( L );

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
        size_t findIfArray = name.find ( "[" );
        int arraySize = 0;

        if ( findIfArray != string::npos )
        {
            size_t endArraySize = name.find ( "]" );
            arraySize = stoi ( name.substr ( findIfArray+1, endArraySize-findIfArray-1 ) );
            name = name.substr ( 0, findIfArray );
            type += "[]";
        }

        AddGetter ( member, name, type, arraySize );
    }

    ClassDef ( LuaUserClass, 1 )
};

int GetMember ( lua_State* L );
int SetMember ( lua_State* L );
int GetMemberValue ( lua_State* L );

template<typename T> void MakeTTreeFunctions ( string type_name )
{
    newBranchFns[type_name] = [=] ( lua_State* L, TTree* tree, const char* bname, int arraysize )
    {
        T* branch_ptr = * ( NewUserData<T> ( L ) );
        tree->Branch ( bname, branch_ptr );

        branch_ptr->MakeMetatable ( L );
    };

    newBranchFns["vector<" + type_name + ">"] = [=] ( lua_State* L, TTree* tree, const char* bname, int arraysize )
    {
        vector<T>* branch_ptr = * ( NewUserData<vector<T>> ( L ) );
        tree->Branch ( bname, branch_ptr );

        SetupMetatable ( L );
        AddMethod ( L, luaExt_SetUserDataValue, "Set" );
        AddMethod ( L, luaExt_GetUserDataValue, "Get" );
        AddMethod ( L, luaExt_SetUserDataValue, "PushBack" );
    };

    MakeAccessorsUserDataFuncs<T> ( type_name );
}

#endif




