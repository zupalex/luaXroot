#ifndef __USERCLASSBASE__
#define __USERCLASSBASE__

#include "LuaRootBinder.h"

class LuaUserClass
{
private:

protected:

public:
    LuaUserClass() {}
    virtual ~LuaUserClass() {}

    map<string, function<void ( lua_State* ) >> getters; //!
    map<string, function<void ( lua_State* ) >> setters; //!

    void MakeMetatable ( lua_State* L );

    template<typename T> typename enable_if<!is_std_vector<T>::value, void>::type AddGetter ( T* src, string name )
    {
        getters[name] = [=] ( lua_State* L_ )
        {
            lua_autogetvalue ( L_, *src );
        };
    }

    template<typename T> typename enable_if<is_std_vector<T>::value, void>::type AddGetter ( T* src, string name )
    {
        getters[name] = [=] ( lua_State* L_ )
        {
            lua_autogetvector ( L_, *src );
        };
    }

    template<typename T> void AddGetterArray ( T* src, string name, unsigned int size )
    {
        getters[name] = [=] ( lua_State* L_ )
        {
            lua_autogetarray ( L_, src, size );
        };
    }

    template<typename T> typename enable_if<!is_std_vector<T>::value, void>::type AddSetter ( T* dest, string name )
    {
        setters[name] = [=] ( lua_State* L_ )
        {
            lua_autosetvalue ( L_, dest, -1, 3 );
        };
    }

    template<typename T> typename enable_if<is_std_vector<T>::value, void>::type AddSetter ( T* dest, string name )
    {
        setters[name] = [=] ( lua_State* L_ )
        {
            lua_autosetvector ( L_, dest, -1, 3 );
        };
    }

    template<typename T> void AddSetterArray ( T* dest, string name, unsigned int size )
    {
        setters[name] = [=] ( lua_State* L_ )
        {
            lua_autosetarray ( L_, dest, size, -1, 3 );
        };
    }

    template<typename T> void AddAccessor ( T* member, string name )
    {
        size_t findIfArray = name.find ( "[" );

        if ( findIfArray == string::npos )
        {
            AddGetter ( member, name );
            AddSetter ( member, name );
        }
        else
        {
            size_t endArraySize = name.find ( "]" );
            int arraySize = stoi ( name.substr ( findIfArray+1, endArraySize-findIfArray-1 ) );
            AddGetterArray ( member, name.substr(0, findIfArray), arraySize );
            AddSetterArray ( member, name.substr(0, findIfArray), arraySize );
        }
    }
    
    ClassDef(LuaUserClass, 1)
};

int GetMember ( lua_State* L );
int SetMember ( lua_State* L );

template<typename T> void MakeTTreeFunctions(string type_name)
{
    newBranchFns[type_name] = [=] ( lua_State* L, TTree* tree, const char* bname, int arraysize )
    {
        T* branch_ptr = new T;
        tree->Branch ( bname, branch_ptr );
        T** ud = NewUserData<T>(L);
        *ud = branch_ptr;

        (*ud)->MakeMetatable(L);
    };

    newBranchFns["vector<" + type_name + ">"] = [=] ( lua_State* L, TTree* tree, const char* bname, int arraysize )
    {
        vector<T>* branch_ptr = new vector<T>;
        tree->Branch ( bname, branch_ptr );
        vector<T>** ud = NewUserData<vector<T>>(L);
        *ud = branch_ptr;

        SetupMetatable(L);
        AddMethod(L, luaExt_SetUserDataValue, "Set");
        AddMethod(L, luaExt_GetUserDataValue, "Get");
    };

    MakeAccessorsUserDataFuncs<T>(type_name);
}

#endif
