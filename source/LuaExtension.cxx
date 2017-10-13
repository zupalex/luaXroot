#include "LuaExtension.h"
#include <llex.h>

lua_State* lua = 0;

lua_State* InitLuaEnv ( )
{
    lua_State* L = luaL_newstate();
    luaL_openlibs ( L );

    lua_register ( lua, "ls", LuaListDirContent );

    string lua_path = "";

    lua_path += "?;?.lua";

    lua_getglobal ( lua, "package" );
    lua_getfield ( lua, -1, "path" );

    if ( lua_type ( lua, -1 ) == LUA_TSTRING ) lua_path = ( string ) lua_tostring ( lua, -1 ) + ";" + lua_path;
    lua_pop ( lua, 1 );

    lua_pushstring ( lua, lua_path.c_str() );

    lua_setfield ( lua, -2, "path" );
    lua_pop ( lua, 1 );

    cout << "Lua Env initialized..." << endl;

    return L;
}

void TryGetGlobalField ( lua_State *L, string gfield )
{
    if ( gfield.empty() )
    {
        lua_pushnil ( L );
        return;
    }

    lua_getglobal ( L, "_G" );

    if ( lua_type ( L, -1 ) == LUA_TNIL )
    {
        cerr << "Issue while retrieving global environment... aborting..." << endl;
        lua_pop ( L, 1 );
        return;
    }

    vector<string> chain;

    size_t sepPos = gfield.find_first_of ( "." );

    if ( sepPos == string::npos ) chain.push_back ( gfield );
    else
    {
        chain.push_back ( gfield.substr ( 0, sepPos ) );
        size_t nextSepPos = gfield.find_first_of ( ".", sepPos+1 );

        while ( nextSepPos != string::npos )
        {
            chain.push_back ( gfield.substr ( sepPos+1, nextSepPos-sepPos-1 ) );
            sepPos = nextSepPos;
            nextSepPos = gfield.find_first_of ( ".", sepPos+1 );
        }

        chain.push_back ( gfield.substr ( sepPos+1 ) );
    }

    for ( unsigned int i = 0; i < chain.size(); i++ )
    {
//         cout << "Getting field: " << chain[i] << endl;
        lua_getfield ( L, -1, chain[i].c_str() );
        lua_remove ( L, -2 );
        if ( lua_type ( L, -1 ) == LUA_TNIL )
        {
            return;
        }
//         cout << "Retrieved field: " << chain[i] << endl;
    }
}

bool TrySetGlobalField ( lua_State *L, string gfield )
{
    if ( gfield.empty() ) return false;

    lua_getglobal ( L, "_G" );

    if ( lua_type ( L, -1 ) == LUA_TNIL )
    {
        cerr << "Issue while retrieving global environment... aborting..." << endl;
        lua_pop ( L, 1 );
        return false;
    }

    vector<string> chain;

    size_t sepPos = gfield.find_first_of ( "." );

    if ( sepPos == string::npos ) chain.push_back ( gfield );
    else
    {
        chain.push_back ( gfield.substr ( 0, sepPos ) );
        size_t nextSepPos = gfield.find_first_of ( ".", sepPos+1 );

        while ( nextSepPos != string::npos )
        {
            chain.push_back ( gfield.substr ( sepPos+1, nextSepPos-sepPos-1 ) );
            sepPos = nextSepPos;
            nextSepPos = gfield.find_first_of ( ".", sepPos+1 );
        }

        chain.push_back ( gfield.substr ( sepPos+1 ) );
    }

    for ( unsigned int i = 0; i < chain.size()-1; i++ )
    {
        lua_getfield ( L, -1, chain[i].c_str() );

        if ( lua_type ( L, -1 ) == LUA_TNIL )
        {
            lua_pop ( L, 1 );
//             cout << "TrySetGlobalField: field " << chain[i] << " does not exists. Creating a table for it..." << endl;
            lua_newtable ( L );
            lua_pushvalue ( L, -1 );
            lua_setfield ( L, -3, chain[i].c_str() );
        }
        else if ( lua_type ( L, -1 ) != LUA_TTABLE )
        {
            cerr << "attempt to assign a field to " << chain[i] << ": a " << lua_typename ( L, lua_type ( L, -1 ) ) << endl;
            lua_pop ( L, 1 );
            return false;
        }

        lua_remove ( L, -2 );
    }

    lua_insert ( L, -2 );
//     cout << "TrySetGlobalField: Setting last field " << chain.back() << endl;
    lua_setfield ( L, -2, chain.back().c_str() );
    lua_pop ( L, 1 );

    return true;
}

void DoForEach ( lua_State *L, int index, function<bool ( lua_State *L_ ) > dofn )
{
    if ( index < 0 ) index = lua_gettop ( L ) + index + 1;

    if ( lua_type ( L, index ) == LUA_TTABLE )
    {
        lua_pushnil ( L );

        while ( lua_next ( L, index ) != 0 )
        {
            bool stop = dofn ( L );

            lua_pop ( L, 1 );

            if ( stop )
            {
                lua_pop ( L, 1 );
                return;
            }
        }
    }
    else
    {
        dofn ( L );
        return;
    }
}

int LuaListDirContent ( lua_State* L )
{
    DIR* dir;
    dirent* dentry;
    int i;

    if ( lua_type ( L, 1 ) != LUA_TSTRING )
    {
        lua_pushnil ( L );
        lua_pushstring ( L, "Invalid argument" );
        return 2;
    }

    const char* path = lua_tostring ( L, 1 );

    dir = opendir ( path );

    if ( dir == nullptr )
    {
        lua_pushnil ( L );
        lua_pushstring ( L, "no such directory" );
        return 2;
    }

    lua_newtable ( L );
    i = 1;

    while ( ( dentry = readdir ( dir ) ) != nullptr )
    {
        string dtype;

        struct stat fstat;

        stat ( dentry->d_name, &fstat );

        switch ( fstat.st_mode & S_IFMT )
        {
        case S_IFDIR:
            dtype = "dir";
            break;
        case S_IFREG:
            dtype = "file";
            break;
        default:
            dtype = "undetermined";
            break;
        }

        if ( dtype != "undetermined" )
        {
            lua_pushnumber ( L, i );

            lua_newtable ( L );

            lua_pushstring ( L, dentry->d_name );
            lua_setfield ( L, -2, "name" );

            lua_pushstring ( L, dtype.c_str() );
            lua_setfield ( L, -2, "type" );

            lua_settable ( L, -3 );

            // 	    cout << "pushed: [" << i << "] = { type = " << dtype.c_str() << " , name = " << dentry->d_name << " }" << endl;

            i++;
        }
    }

    closedir ( dir );

    return 1;
}

// ************************************************************************************************ //
// ***************************************** Sockets Binder *************************************** //
// ************************************************************************************************ //

int LuaRegisterSocketConsts(lua_State* L)
{
    lua_pushinteger(L, AF_UNIX);
    lua_setglobal(L, "AF_UNIX");

    lua_pushinteger(L, AF_INET);
    lua_setglobal(L, "AF_INET");

    lua_pushinteger(L, AF_INET6);
    lua_setglobal(L, "AF_INET6");

    lua_pushinteger(L, AF_IPX);
    lua_setglobal(L, "AF_IPX");

    lua_pushinteger(L, AF_NETLINK);
    lua_setglobal(L, "AF_NETLINK");

    lua_pushinteger(L, AF_X25);
    lua_setglobal(L, "AF_X25");

    lua_pushinteger(L, AF_AX25);
    lua_setglobal(L, "AF_AX25");

    lua_pushinteger(L, AF_ATMPVC);
    lua_setglobal(L, "AF_ATMPVC");

    lua_pushinteger(L, AF_APPLETALK);
    lua_setglobal(L, "AF_APPLETALK");

    lua_pushinteger(L, AF_PACKET);
    lua_setglobal(L, "AF_PACKET");


    lua_pushinteger(L, SOCK_STREAM);
    lua_setglobal(L, "SOCK_STREAM");

    lua_pushinteger(L, SOCK_DGRAM);
    lua_setglobal(L, "SOCK_DGRAM");

    lua_pushinteger(L, SOCK_SEQPACKET);
    lua_setglobal(L, "SOCK_SEQPACKET");

    lua_pushinteger(L, SOCK_RAW);
    lua_setglobal(L, "SOCK_RAW");

    lua_pushinteger(L, SOCK_RDM);
    lua_setglobal(L, "SOCK_RDM");

    lua_pushinteger(L, SOCK_PACKET);
    lua_setglobal(L, "SOCK_PACKET");

    lua_pushinteger(L, SOCK_NONBLOCK);
    lua_setglobal(L, "SOCK_NONBLOCK");

    lua_pushinteger(L, SOCK_CLOEXEC);
    lua_setglobal(L, "SOCK_CLOEXEC");

    return 0;
}

map<int, SocketInfos> socketsList;
int maxSockFd = -1;

int LuaSysClose(lua_State* L)
{
    if(!CheckLuaArgs(L, 1, true, "LuaSysClose", LUA_TSTRING))
    {
        return 0;
    }

    const char* name = lua_tostring(L, 1);


    return 0;
}

int LuaSysUnlink(lua_State* L)
{
    if(!CheckLuaArgs(L, 1, true, "LuaSysUnlink", LUA_TSTRING))
    {
        return 0;
    }

    const char* name = lua_tostring(L, 1);

    unlink(name);

    return 0;
}

int LuaSysRead(lua_State* L)
{
    lua_pushstring(L, "");

    return 1;
}

int LuaSysWrite(lua_State* L)
{
    lua_pushboolean(L, 1);

    return 1;
}

int LuaNewSocket(lua_State* L)
{
    if(!CheckLuaArgs(L, 1, true, "LuaNewSocket", LUA_TTABLE))
    {
        return 0;
    }

    lua_getfield(L, 1, "domain");
    lua_getfield(L, 1, "type");
    lua_getfield(L, 1, "protocol");

    if(lua_type(L, -3) != LUA_TNUMBER || lua_type(L, -2) != LUA_TNUMBER)
    {
        cerr << "Missing or invalid fields: required { domain='d' , type='t' [, protocol='p'] }" << endl;
        return 0;
    }

    int sock_domain = lua_tointeger(L, -3);
    int sock_type = lua_tointeger(L, -2);
    int sock_protocol = 0;

    if(lua_type(L, -1) == LUA_TNUMBER) sock_protocol = lua_tointeger(L, -1);

    lua_pop(L, 3);

    int sd = socket(sock_domain, sock_type, sock_protocol);
    if(sd > maxSockFd) maxSockFd = sd;

    socketsList[sd] = SocketInfos(sock_domain, sock_type);

    lua_pushinteger(L, sd);

    return 1;
}

int LuaSocketBind(lua_State* L)
{
    if(!CheckLuaArgs(L, 1, true, "LuaSocketBind", LUA_TTABLE)) return 0;

    lua_getfield(L, 1, "sockfd");
    if(!CheckLuaArgs(L, -1, true, "LuaSocketBind arguments", LUA_TNUMBER)) return 0;
    int sockfd = lua_tointeger(L, -1);

    int sock_domain = socketsList[sockfd].domain;

    if(sock_domain == AF_UNIX)
    {
        sockaddr_un addr;

        addr.sun_family = AF_UNIX;

        lua_getfield(L, 1, "name");
        if(!CheckLuaArgs(L, -1, true, "LuaSocketBind arguments", LUA_TSTRING)) return 0;

        const char* name = lua_tostring(L, -1);

        strcpy(addr.sun_path, name);

        if(bind(sockfd, (sockaddr*)&addr, sizeof(addr)) < 0)
        {
            lua_pushboolean(L, 0);
            return 1;
        }
    }
    else if(sock_domain == AF_INET)
    {
        sockaddr_in addr;
        addr.sin_addr.s_addr = INADDR_ANY;

        lua_getfield(L, 1, "port");
        if(!CheckLuaArgs(L, -1, true, "LuaSocketBind arguments", LUA_TNUMBER)) return 0;
        int portno = lua_tointeger(L, -1);

        addr.sin_family = AF_INET;
        addr.sin_port = htons(portno);

        if(bind(sockfd, (sockaddr*)&addr, sizeof(addr)) < 0)
        {
            lua_pushboolean(L, 0);
            return 1;
        }
    }
    else if(sock_domain == AF_INET6)
    {
        sockaddr_in6 addr;

        addr.sin6_family = AF_INET6;
    }

    lua_pushboolean(L, 1);
    return 1;
}

int LuaSocketConnect(lua_State* L)
{
    if(!CheckLuaArgs(L, 1, true, "LuaSocketConnect", LUA_TTABLE)) return 0;

    lua_getfield(L, 1, "sockfd");
    if(!CheckLuaArgs(L, -1, true, "LuaSocketConnect arguments", LUA_TNUMBER)) return 0;
    int sockfd = lua_tointeger(L, -1);

    int sock_domain = socketsList[sockfd].domain;

    if(sock_domain == AF_UNIX)
    {
        sockaddr_un addr;

        addr.sun_family = AF_UNIX;

        lua_getfield(L, 1, "name");
        if(!CheckLuaArgs(L, -1, true, "LuaSocketConnect arguments", LUA_TSTRING)) return 0;

        const char* name = lua_tostring(L, -1);

        strcpy(addr.sun_path, name);

        if(connect(sockfd, (sockaddr*)&addr, sizeof(sockaddr_un)) < 0)
        {
            char err_msg[100];
            sprintf(err_msg, "Failed to connect to %s", name);
            perror(err_msg);
            lua_pushboolean(L, 0);
            lua_pushstring(L, err_msg);
            return 2;
        }
    }

    lua_pushboolean(L, 1);
    return 1;
}

int LuaSocketSelect(lua_State* L)
{
    if(!CheckLuaArgs(L, 1, true, "LuaSocketSelect", LUA_TTABLE)) return 0;

    lua_getfield(L, 1, "sockfd");
    if(!CheckLuaArgs(L, -1, true, "LuaSocketSelect arguments", LUA_TNUMBER)) return 0;
    int sockfd = lua_tointeger(L, -1);

    timeval timer;
    fd_set readfds, writefds, exceptsfds;
    bool doread = false, dowrite = false, doexcepts = false;

    timer.tv_sec = 10;
    timer.tv_usec = 0;

    if(lua_checkfield(L, 1, "timeout", LUA_TNUMBER))
    {
        timer.tv_sec = floor(lua_tonumber(L, -1));
        timer.tv_usec = (lua_tonumber(L, -1) - timer.tv_sec)*1e6;
    }

    if(lua_checkfield(L, 1, "read", LUA_TBOOLEAN))
    {
        doread = true;
        if(lua_toboolean(L, -1)) FD_SET(sockfd, &readfds);
    }

    if(lua_checkfield(L, 1, "write", LUA_TBOOLEAN))
    {
        dowrite = true;
        if(lua_toboolean(L, -1)) FD_SET(sockfd, &writefds);
    }

    if(lua_checkfield(L, 1, "exception", LUA_TBOOLEAN))
    {
        doexcepts = true;
        if(lua_toboolean(L, -1)) FD_SET(sockfd, &exceptsfds);
    }

    int sel = select(maxSockFd+1, &readfds, &writefds, &exceptsfds, &timer);

    // == -1 means an error occured during the select()
    // == 0 means it timed out
    if(sel == -1)
    {
        lua_pushnil(L);
        return 1;
    }
    else if(sel == 0)
    {
        lua_pushnumber(L, 0);
        return 1;
    }
    else
    {
        if(doread)
        {
            lua_newtable(L);

            if(FD_ISSET(sockfd, &readfds))
            {
                lua_pushnumber(L, sockfd);
                lua_seti(L, -2, 1);
            }
        }
        else lua_pushnil(L);

        if(dowrite)
        {
            lua_newtable(L);

            if(FD_ISSET(sockfd, &writefds))
            {
                lua_pushnumber(L, sockfd);
                lua_seti(L, -2, 1);
            }
        }
        else lua_pushnil(L);

        if(doexcepts)
        {
            lua_newtable(L);

            if(FD_ISSET(sockfd, &exceptsfds))
            {
                lua_pushnumber(L, sockfd);
                lua_seti(L, -2, 1);
            }
        }
        else lua_pushnil(L);

        return 3;
    }
}

int LuaSocketListen(lua_State* L)
{
    if(!CheckLuaArgs(L, 1, true, "LuaSocketListen", LUA_TTABLE)) return 0;

    lua_getfield(L, 1, "sockfd");
    if(!CheckLuaArgs(L, -1, true, "LuaSocketListen arguments", LUA_TNUMBER)) return 0;
    int sockfd = lua_tointeger(L, -1);

    int maxQueue = 1;

    if(lua_checkfield(L, 1, "maxqueue", LUA_TNUMBER)) maxQueue = lua_tointeger(L, -1);

    int success = listen(sockfd, maxQueue);

    if(success == -1)
    {
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }

    lua_pushinteger(L, success);
    return 1;
}


int LuaSocketAccept(lua_State* L)
{
    if(!CheckLuaArgs(L, 1, true, "LuaSocketAccept", LUA_TTABLE)) return 0;

    lua_getfield(L, 1, "sockfd");
    if(!CheckLuaArgs(L, -1, true, "LuaSocketAccept arguments", LUA_TNUMBER)) return 0;
    int sockfd = lua_tointeger(L, -1);

    sockaddr_storage clients_addr;
    socklen_t addr_size;

    int new_fd = accept(sockfd, (sockaddr*) &clients_addr, &addr_size);

    if(new_fd < 0)
    {
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }

    lua_pushinteger(L, new_fd);
    return 1;
}

int LuaSocketReceive(lua_State* L)
{
    if(!CheckLuaArgs(L, 1, true, "LuaSocketReceive", LUA_TTABLE)) return 0;

    lua_getfield(L, 1, "sockfd");
    if(!CheckLuaArgs(L, -1, true, "LuaSocketReceive arguments", LUA_TNUMBER)) return 0;
    int sockfd = lua_tointeger(L, -1);

    lua_getfield(L, 1, "size");
    if(!CheckLuaArgs(L, -1, true, "LuaSocketReceive arguments", LUA_TNUMBER)) return 0;
    int data_length = lua_tointeger(L, -1);

    int flags = 0;
    if(lua_checkfield(L, 1, "flags", LUA_TNUMBER)) flags = lua_tointeger(L, -1);

    char* buffer = new char[data_length];

    int bytes_received = recv(sockfd, (void*) buffer, data_length, flags);

    lua_pushinteger(L, bytes_received);
    lua_pushstring(L, buffer);

    return 2;
}

int LuaSocketSend(lua_State* L)
{
    if(!CheckLuaArgs(L, -1, true, "LuaSocketSend", LUA_TTABLE)) return 0;

    lua_getfield(L, 1, "sockfd");
    if(!CheckLuaArgs(L, -1, true, "LuaSocketSend arguments", LUA_TNUMBER)) return 0;
    int sockfd = lua_tointeger(L, -1);

    lua_getfield(L, 1, "data");
    if(!CheckLuaArgs(L, -1, true, "LuaSocketSend arguments", LUA_TSTRING)) return 0;
    const char* data = lua_tostring(L, -1);

    int data_length = strlen(data);
    if(lua_checkfield(L, 1, "size", LUA_TNUMBER)) data_length = lua_tointeger(L, -1);

    int flags = 0;
    if(lua_checkfield(L, 1, "flags", LUA_TNUMBER)) flags = lua_tointeger(L, -1);

    int bytes_sent = send(sockfd, (void*) data, data_length, flags);

    lua_pushinteger(L, bytes_sent);

    return 1;
}

