#include "LuaSocketBinder.h"

// ************************************************************************************************ //
// ***************************************** Sockets Binder *************************************** //
// ************************************************************************************************ //

int LuaRegisterSysOpenConsts ( lua_State* L )
{
    lua_pushinteger ( L, O_RDONLY );
    lua_setglobal ( L, "O_RDONLY" );

    lua_pushinteger ( L, O_WRONLY );
    lua_setglobal ( L, "O_WRONLY" );

    lua_pushinteger ( L, O_RDWR );
    lua_setglobal ( L, "O_RDWR" );

    lua_pushinteger ( L, O_APPEND );
    lua_setglobal ( L, "O_APPEND" );

    lua_pushinteger ( L, O_ASYNC );
    lua_setglobal ( L, "O_ASYNC" );

    lua_pushinteger ( L, O_CLOEXEC );
    lua_setglobal ( L, "O_CLOEXEC" );

    lua_pushinteger ( L, O_CREAT );
    lua_setglobal ( L, "O_CREAT" );

    lua_pushinteger ( L, O_DIRECT );
    lua_setglobal ( L, "O_DIRECT " );

    lua_pushinteger ( L, O_DIRECTORY );
    lua_setglobal ( L, "O_DIRECTORY" );

    lua_pushinteger ( L, O_DSYNC );
    lua_setglobal ( L, "O_DSYNC" );

    lua_pushinteger ( L, O_EXCL );
    lua_setglobal ( L, "O_EXCL" );

    lua_pushinteger ( L, O_LARGEFILE );
    lua_setglobal ( L, "O_LARGEFILE" );

    lua_pushinteger ( L, O_NOATIME );
    lua_setglobal ( L, "O_NOATIME" );

    lua_pushinteger ( L, O_NOCTTY );
    lua_setglobal ( L, "O_NOCTTY" );

    lua_pushinteger ( L, O_NOFOLLOW );
    lua_setglobal ( L, "O_NOFOLLOW" );

    lua_pushinteger ( L, O_NONBLOCK );
    lua_setglobal ( L, "O_NONBLOCK" );

    lua_pushinteger ( L, O_NDELAY );
    lua_setglobal ( L, "O_NDELAY" );

    lua_pushinteger ( L, O_PATH );
    lua_setglobal ( L, "O_PATH" );

    lua_pushinteger ( L, O_SYNC );
    lua_setglobal ( L, "O_SYNC" );

    lua_pushinteger ( L, O_TMPFILE );
    lua_setglobal ( L, "O_TMPFILE" );

    lua_pushinteger ( L, O_TRUNC );
    lua_setglobal ( L, "O_TRUNC" );

    // MODE_T CONSTANTS

    lua_pushinteger ( L, S_IRWXU );
    lua_setglobal ( L, "S_IRWXU" );

    lua_pushinteger ( L, S_IRUSR );
    lua_setglobal ( L, "S_IRUSR" );

    lua_pushinteger ( L, S_IWUSR );
    lua_setglobal ( L, "S_IWUSR" );

    lua_pushinteger ( L, S_IXUSR );
    lua_setglobal ( L, "S_IXUSR" );

    lua_pushinteger ( L, S_IRWXG );
    lua_setglobal ( L, "S_IRWXG" );

    lua_pushinteger ( L, S_IRGRP );
    lua_setglobal ( L, "S_IRGRP" );

    lua_pushinteger ( L, S_IWGRP );
    lua_setglobal ( L, "S_IWGRP" );

    lua_pushinteger ( L, S_IXGRP );
    lua_setglobal ( L, "S_IXGRP" );

    lua_pushinteger ( L, S_IRWXO );
    lua_setglobal ( L, "S_IRWXO" );

    lua_pushinteger ( L, S_IROTH );
    lua_setglobal ( L, "S_IROTH" );

    lua_pushinteger ( L, S_IWOTH );
    lua_setglobal ( L, "S_IWOTH" );

    lua_pushinteger ( L, S_IXOTH );
    lua_setglobal ( L, "S_IXOTH" );

    lua_pushinteger ( L, S_ISUID );
    lua_setglobal ( L, "S_ISUID" );

    lua_pushinteger ( L, S_ISGID );
    lua_setglobal ( L, "S_ISGID" );

    lua_pushinteger ( L, S_ISVTX );
    lua_setglobal ( L, "S_ISVTX" );

    return 0;
}

int LuaSysOpen ( lua_State* L )
{
    lua_unpackarguments ( L, 1, "LuaSysRead argument table",
    {"name", "flags", "mode"},
    {LUA_TSTRING, LUA_TNUMBER, LUA_TSTRING},
    {true, false, false} );

    const char* name = lua_tostring ( L, -3 );
    int flags = lua_tointegerx ( L, -2, nullptr );
    string mode_str = lua_tostringx ( L, -1 );

    if ( flags == 0 ) flags = O_RDONLY | O_NONBLOCK ;
    if ( mode_str.empty() ) mode_str = "0666";

    mode_t mode = strtol ( mode_str.c_str(), nullptr, 8 );

    int fd_open = open ( name, flags, mode );
    if ( fd_open > maxFd ) maxFd = fd_open;

    if ( fd_open == -1 )
    {
        cerr << "Error opening " << name << " => " << errno << endl;
        return 0;
    }

    lua_pushinteger ( L, fd_open );

    return 1;
}

int LuaSysClose ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "LuaSysClose", LUA_TNUMBER ) ) return 0;

    int fd_close = lua_tointeger ( L, 1 );

    close ( fd_close );

    return 0;
}

int MakePipe ( lua_State* L )
{
    int pfds[2];

    if ( pipe ( pfds ) == -1 )
    {
        cerr << "Unabled to make new pipe..." << endl;
        return 0;
    }

    if ( pfds[1] > maxFd ) maxFd = pfds[1];

    lua_pushinteger ( L, pfds[1] );
    lua_pushinteger ( L, pfds[0] );

    return 2;
}

int MakeFiFo ( lua_State* L )
{
    lua_unpackarguments ( L, 1, "LuaSysRead argument table",
    {"name", "mode"},
    {LUA_TSTRING, LUA_TSTRING},
    {true, false} );

    const char* name = lua_tostring ( L, -2 );
    string mode_str = lua_tostringx ( L, -1 );

    remove ( name );

    if ( mode_str.empty() ) mode_str = "0777";

    mode_t mode = strtol ( mode_str.c_str(), nullptr, 8 );

    if ( mkfifo ( name, mode ) == -1 )
    {
        cerr << "Error while opening the fifo..." << endl;
    }

    return 0;
}

int LuaSysDup ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "SysDup", LUA_TNUMBER ) ) return 0;

    int oldfd = lua_tointeger ( L, 1 );

    int newfd = dup ( oldfd );
    if ( newfd > maxFd ) maxFd = newfd;

    lua_pushinteger ( L, newfd );

    return 1;
}

int LuaSysDup2 ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "SysDup", LUA_TNUMBER, LUA_TNUMBER ) ) return 0;

    int oldfd = lua_tointeger ( L, 1 );
    int newfd = lua_tointeger ( L, 2 );

    dup2 ( oldfd, newfd );

    if ( oldfd > maxFd ) maxFd = oldfd;
    if ( newfd > maxFd ) maxFd = newfd;

    return 0;
}

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
int maxFd = -1;

int LuaSysUnlink ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "LuaSysUnlink", LUA_TSTRING ) )
    {
        return 0;
    }

    const char* name = lua_tostring ( L, 1 );

    unlink ( name );

    return 0;
}

int LuaSysRead ( lua_State* L )
{
    lua_unpackarguments ( L, 1, "LuaSysRead argument table",
    {"fd", "length"},
    {LUA_TNUMBER, LUA_TNUMBER},
    {true, true} );

    int rfd = lua_tointeger ( L, -2 );
    int read_length = lua_tointeger ( L, -1 );

    char* buffer = new char[read_length];

    int rbytes = read ( rfd, buffer, read_length );

    lua_pushstring ( L, buffer );
    lua_pushinteger ( L, rbytes );

    return 2;
}

int LuaSysWrite ( lua_State* L )
{
    lua_unpackarguments ( L, 1, "LuaSysRead argument table",
    {"fd", "data"},
    {LUA_TNUMBER, LUA_TSTRING},
    {true, true} );

    int wfd = lua_tointeger ( L, -2 );
    string msg = lua_tostring ( L, -1 );

    int msg_length = msg.length();

    int wbytes = write ( wfd, msg.c_str(), msg_length );

    lua_pushinteger ( L, wbytes );

    return 1;
}

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

int LuaSysSelect ( lua_State* L )
{
    if ( !CheckLuaArgs ( L, 1, true, "LuaSocketSelect", LUA_TTABLE ) ) return 0;

    timeval timer;
    fd_set readfds, writefds, exceptsfds;
    vector<int> rfd_list, wfd_list, efd_list;
    bool doread = false, dowrite = false, doexcepts = false;

    FD_ZERO ( &readfds );
    FD_ZERO ( &writefds );
    FD_ZERO ( &exceptsfds );

    timer.tv_sec = 10;
    timer.tv_usec = 0;

    if ( lua_checkfield ( L, 1, "timeout", LUA_TNUMBER ) )
    {
        timer.tv_sec = floor ( lua_tonumber ( L, -1 ) );
        timer.tv_usec = ( lua_tonumber ( L, -1 ) - timer.tv_sec ) *1e6;
    }

    if ( lua_checkfield ( L, 1, "read", LUA_TTABLE ) )
    {
        doread = true;
        int readfd_size = lua_rawlen ( L, -1 );

        for ( int i = 0; i < readfd_size; i++ )
        {
            lua_geti ( L, -1, i+1 );
            int fd = lua_tointeger ( L, -1 );
            lua_pop ( L, 1 );
            rfd_list.push_back ( fd );
            FD_SET ( fd, &readfds );
        }
    }

    if ( lua_checkfield ( L, 1, "write", LUA_TTABLE ) )
    {
        dowrite = true;
        int writefd_size = lua_rawlen ( L, -1 );

        for ( int i = 0; i < writefd_size; i++ )
        {
            lua_geti ( L, -1, i+1 );
            int fd = lua_tointeger ( L, -1 );
            lua_pop ( L, 1 );
            wfd_list.push_back ( fd );
            FD_SET ( fd, &writefds );
        }
    }

    if ( lua_checkfield ( L, 1, "exception", LUA_TTABLE ) )
    {
        doexcepts = true;
        int exceptfd_size = lua_rawlen ( L, -1 );

        for ( int i = 0; i < exceptfd_size; i++ )
        {
            lua_geti ( L, -1, i+1 );
            int fd = lua_tointeger ( L, -1 );
            lua_pop ( L, 1 );
            efd_list.push_back ( fd );
            FD_SET ( fd, &exceptsfds );
        }
    }

    int sel = select ( maxFd+1, &readfds, &writefds, &exceptsfds, &timer );

    // == -1 means an error occured during the select()
    // == 0 means it timed out
    if ( sel == -1 )
    {
        lua_pushnil ( L );
        return 1;
    }
    else if ( sel == 0 )
    {
        lua_pushnumber ( L, 0 );
        return 1;
    }
    else
    {
        if ( doread )
        {
            lua_newtable ( L );

            for ( unsigned int i = 0; i < rfd_list.size(); i++ )
            {
                if ( FD_ISSET ( rfd_list[i], &readfds ) )
                {
                    lua_pushinteger ( L, rfd_list[i] );
                    lua_seti ( L, -2, i+1 );
                }
            }
        }
        else lua_pushnil ( L );

        if ( dowrite )
        {
            lua_newtable ( L );

            for ( unsigned int i = 0; i < wfd_list.size(); i++ )
            {
                if ( FD_ISSET ( wfd_list[i], &writefds ) )
                {
                    lua_pushinteger ( L, wfd_list[i] );
                    lua_seti ( L, -2, i+1 );
                }
            }
        }
        else lua_pushnil ( L );

        if ( doexcepts )
        {
            lua_newtable ( L );

            for ( unsigned int i = 0; i < efd_list.size(); i++ )
            {
                if ( FD_ISSET ( efd_list[i], &exceptsfds ) )
                {
                    lua_pushinteger ( L, efd_list[i] );
                    lua_seti ( L, -2, i+1 );
                }
            }
        }
        else lua_pushnil ( L );

        return 3;
    }
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
    if ( !CheckLuaArgs ( L, -1, true, "LuaSocketReceive arguments", LUA_TNUMBER ) ) return 0;
    int sockfd = lua_tointeger ( L, -1 );

    lua_getfield ( L, 1, "size" );
    if ( !CheckLuaArgs ( L, -1, true, "LuaSocketReceive arguments", LUA_TNUMBER ) ) return 0;
    int data_length = lua_tointeger ( L, -1 );

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
