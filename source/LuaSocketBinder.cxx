#include "LuaSocketBinder.h"
#include <sys/ioctl.h>

// ************************************************************************************************ //
// ***************************************** Sockets Binder *************************************** //
// ************************************************************************************************ //

int LuaRegisterSocketConsts ( lua_State* L )
{
    lua_pushinteger ( L, AF_UNIX );
    lua_setglobal ( L, "AF_UNIX" );

    lua_pushinteger ( L, AF_INET );
    lua_setglobal ( L, "AF_INET" );

    lua_pushinteger ( L, AF_INET6 );
    lua_setglobal ( L, "AF_INET6" );

    lua_pushinteger ( L, AF_IPX );
    lua_setglobal ( L, "AF_IPX" );

    lua_pushinteger ( L, AF_NETLINK );
    lua_setglobal ( L, "AF_NETLINK" );

    lua_pushinteger ( L, AF_X25 );
    lua_setglobal ( L, "AF_X25" );

    lua_pushinteger ( L, AF_AX25 );
    lua_setglobal ( L, "AF_AX25" );

    lua_pushinteger ( L, AF_ATMPVC );
    lua_setglobal ( L, "AF_ATMPVC" );

    lua_pushinteger ( L, AF_APPLETALK );
    lua_setglobal ( L, "AF_APPLETALK" );

    lua_pushinteger ( L, AF_PACKET );
    lua_setglobal ( L, "AF_PACKET" );


    lua_pushinteger ( L, SOCK_STREAM );
    lua_setglobal ( L, "SOCK_STREAM" );

    lua_pushinteger ( L, SOCK_DGRAM );
    lua_setglobal ( L, "SOCK_DGRAM" );

    lua_pushinteger ( L, SOCK_SEQPACKET );
    lua_setglobal ( L, "SOCK_SEQPACKET" );

    lua_pushinteger ( L, SOCK_RAW );
    lua_setglobal ( L, "SOCK_RAW" );

    lua_pushinteger ( L, SOCK_RDM );
    lua_setglobal ( L, "SOCK_RDM" );

    lua_pushinteger ( L, SOCK_PACKET );
    lua_setglobal ( L, "SOCK_PACKET" );

    lua_pushinteger ( L, SOCK_NONBLOCK );
    lua_setglobal ( L, "SOCK_NONBLOCK" );

    lua_pushinteger ( L, SOCK_CLOEXEC );
    lua_setglobal ( L, "SOCK_CLOEXEC" );

    return 0;
}

map<int, SocketInfos> socketsList;

int LuaNewSocket ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "LuaNewSocket", LUA_TTABLE ) )
    {
        return 0;
    }

    lua_getfield ( L, 1, "domain" );
    lua_getfield ( L, 1, "type" );
    lua_getfield ( L, 1, "protocol" );

    if ( lua_type ( L, -3 ) != LUA_TNUMBER || lua_type ( L, -2 ) != LUA_TNUMBER )
    {
        cerr << "Missing or invalid fields: required { domain='d' , type='t' [, protocol='p'] }" << endl;
        return 0;
    }

    int sock_domain = lua_tointeger ( L, -3 );
    int sock_type = lua_tointeger ( L, -2 );
    int sock_protocol = 0;

    if ( lua_type ( L, -1 ) == LUA_TNUMBER ) sock_protocol = lua_tointeger ( L, -1 );

    lua_pop ( L, 3 );

    int sd = socket ( sock_domain, sock_type, sock_protocol );
    if ( sd > maxFd ) maxFd = sd;

    socketsList[sd] = SocketInfos ( sock_domain, sock_type );

    lua_pushinteger ( L, sd );

    return 1;
}

int LuaSocketBind ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "LuaSocketBind", LUA_TTABLE ) ) return 0;

    lua_getfield ( L, 1, "sockfd" );
    if ( !CheckLuaArgs ( L, -1, true, "LuaSocketBind arguments", LUA_TNUMBER ) ) return 0;
    int sockfd = lua_tointeger ( L, -1 );

    int sock_domain = socketsList[sockfd].domain;
    int sock_type = socketsList[sockfd].type;

    if ( sock_domain == AF_UNIX )
    {
        sockaddr_un addr;

        addr.sun_family = AF_UNIX;

        lua_getfield ( L, 1, "name" );
        if ( !CheckLuaArgs ( L, -1, true, "LuaSocketBind arguments", LUA_TSTRING ) ) return 0;

        const char* name = lua_tostring ( L, -1 );

        strcpy ( addr.sun_path, name );

        if ( bind ( sockfd, ( sockaddr* ) &addr, sizeof ( addr ) ) < 0 )
        {
            lua_pushboolean ( L, 0 );
            return 1;
        }
    }
    else if ( sock_domain == AF_INET )
    {
        addrinfo hints, *res;

        memset ( &hints, 0, sizeof hints );

        hints.ai_family = AF_INET;
        hints.ai_socktype = sock_type;
        hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

        lua_getfield ( L, 1, "port" );
        if ( !CheckLuaArgs ( L, -1, true, "LuaSocketBind arguments", LUA_TSTRING ) ) return 0;
        const char* portno = lua_tostring ( L, -1 );

        if ( lua_checkfield ( L, 1, "address", LUA_TSTRING ) ) getaddrinfo ( lua_tostring ( L, -1 ), portno, &hints, &res );
        else getaddrinfo ( NULL, portno, &hints, &res );



        if ( bind ( sockfd, res->ai_addr, res->ai_addrlen ) < 0 )
        {
            lua_pushboolean ( L, 0 );
            return 1;
        }
    }
    else if ( sock_domain == AF_INET6 )
    {
        sockaddr_in6 addr;

        addr.sin6_family = AF_INET6;
    }

    lua_pushboolean ( L, 1 );
    return 1;
}

int LuaSocketConnect ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "LuaSocketConnect", LUA_TTABLE ) ) return 0;

    lua_getfield ( L, 1, "sockfd" );
    if ( !CheckLuaArgs ( L, -1, true, "LuaSocketConnect arguments", LUA_TNUMBER ) ) return 0;
    int sockfd = lua_tointeger ( L, -1 );

    int sock_domain = socketsList[sockfd].domain;
    int sock_type = socketsList[sockfd].type;

    if ( sock_domain == AF_UNIX )
    {
        sockaddr_un addr;

        addr.sun_family = AF_UNIX;

        lua_getfield ( L, 1, "name" );
        if ( !CheckLuaArgs ( L, -1, true, "LuaSocketConnect arguments", LUA_TSTRING ) ) return 0;
        const char* name = lua_tostring ( L, -1 );

        strcpy ( addr.sun_path, name );

        if ( connect ( sockfd, ( sockaddr* ) &addr, sizeof ( sockaddr_un ) ) < 0 )
        {
            char err_msg[100];
            sprintf ( err_msg, "Failed to connect to %s", name );
            perror ( err_msg );
            lua_pushboolean ( L, 0 );
            lua_pushstring ( L, err_msg );
            return 2;
        }
    }
    else if ( sock_domain == AF_INET )
    {
        addrinfo hints, *res;

        memset ( &hints, 0, sizeof hints );
        hints.ai_family = AF_INET;
        hints.ai_socktype = sock_type;

        lua_getfield ( L, 1, "address" );
        if ( !CheckLuaArgs ( L, -1, true, "LuaSocketConnect arguments", LUA_TSTRING ) ) return 0;
        const char* address = lua_tostring ( L, -1 );

        lua_getfield ( L, 1, "port" );
        if ( !CheckLuaArgs ( L, -1, true, "LuaSocketConnect arguments", LUA_TSTRING ) ) return 0;
        const char* portno = lua_tostring ( L, -1 );

        getaddrinfo ( address, portno, &hints, &res );

        if ( connect ( sockfd, res->ai_addr, res->ai_addrlen ) < 0 )
        {
            char err_msg[100];
            sprintf ( err_msg, "Failed to connect to %s:%s", address, portno );
            perror ( err_msg );
            lua_pushboolean ( L, 0 );
            lua_pushstring ( L, err_msg );
            return 2;
        }
    }
    else if ( sock_domain == AF_INET6 )
    {

    }

    lua_pushboolean ( L, 1 );
    return 1;
}

int LuaSocketListen ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "LuaSocketListen", LUA_TTABLE ) ) return 0;

    lua_getfield ( L, 1, "sockfd" );
    if ( !CheckLuaArgs ( L, -1, true, "LuaSocketListen arguments", LUA_TNUMBER ) ) return 0;
    int sockfd = lua_tointeger ( L, -1 );

    int maxQueue = 1;

    if ( lua_checkfield ( L, 1, "maxqueue", LUA_TNUMBER ) ) maxQueue = lua_tointeger ( L, -1 );

    int success = listen ( sockfd, maxQueue );

    if ( success == -1 )
    {
        lua_pushnil ( L );
        lua_pushinteger ( L, errno );
        return 2;
    }

    lua_pushinteger ( L, success );
    return 1;
}


int LuaSocketAccept ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "LuaSocketAccept", LUA_TTABLE ) ) return 0;

    lua_getfield ( L, 1, "sockfd" );
    if ( !CheckLuaArgs ( L, -1, true, "LuaSocketAccept arguments", LUA_TNUMBER ) ) return 0;
    int sockfd = lua_tointeger ( L, -1 );

    sockaddr_storage clients_addr;
    socklen_t addr_size;

    int new_fd = accept ( sockfd, ( sockaddr* ) &clients_addr, &addr_size );

    if ( new_fd < 0 )
    {
        lua_pushnil ( L );
        lua_pushinteger ( L, errno );
        return 2;
    }

    lua_pushinteger ( L, new_fd );
    return 1;
}

int LuaSocketReceive ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "LuaSocketReceive", LUA_TTABLE ) ) return 0;

    lua_getfield ( L, 1, "sockfd" );
    if ( !CheckLuaArgs ( L, -1, true, "LuaSocketReceive argument sockfd", LUA_TNUMBER ) ) return 0;
    int sockfd = lua_tointeger ( L, -1 );

    lua_getfield ( L, 1, "size" );
    int data_length;
    if ( CheckLuaArgs ( L, -1, false, "LuaSocketReceive argument size", LUA_TNUMBER ) ) data_length = lua_tointeger ( L, -1 );
    else ioctl(sockfd, FIONREAD, &data_length);

    int flags = 0;
    if ( lua_checkfield ( L, 1, "flags", LUA_TNUMBER ) ) flags = lua_tointeger ( L, -1 );

    char* buffer = new char[data_length];

    int bytes_received = recv ( sockfd, ( void* ) buffer, data_length, flags );

    lua_pushinteger ( L, bytes_received );
    lua_pushstring ( L, buffer );

    return 2;
}

int LuaSocketSend ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, -1, true, "LuaSocketSend", LUA_TTABLE ) ) return 0;

    lua_getfield ( L, 1, "sockfd" );
    if ( !CheckLuaArgs ( L, -1, true, "LuaSocketSend arguments", LUA_TNUMBER ) ) return 0;
    int sockfd = lua_tointeger ( L, -1 );

    lua_getfield ( L, 1, "data" );
    if ( !CheckLuaArgs ( L, -1, true, "LuaSocketSend arguments", LUA_TSTRING ) ) return 0;
    const char* data = lua_tostring ( L, -1 );

    int data_length = strlen ( data );
    if ( lua_checkfield ( L, 1, "size", LUA_TNUMBER ) ) data_length = lua_tointeger ( L, -1 );

    int flags = 0;
    if ( lua_checkfield ( L, 1, "flags", LUA_TNUMBER ) ) flags = lua_tointeger ( L, -1 );

    int bytes_sent = send ( sockfd, ( void* ) data, data_length, flags );

    lua_pushinteger ( L, bytes_sent );

    return 1;
}
