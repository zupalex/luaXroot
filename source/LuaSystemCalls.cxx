#include "LuaSystemCalls.h"
#include <sys/ioctl.h>
#include <signal.h>

vector<int> childProcessTracker;

int GetFlagsFromOctalString(lua_State* L, string str)
{
	TryGetGlobalField(L, "_utilities.stringtoflags");
	lua_pushstring(L, str.c_str());
	lua_pcall(L, 1, 1, 0);
	int flagval = lua_tointeger(L, -1);
	lua_pop(L, 1);

	return flagval;
}

int LuaSysFork(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaSysFork argument table",
		{ "fn", "args", "preinit" },
		{ LUA_TFUNCTION, LUA_TTABLE, LUA_TFUNCTION },
		{ true, false, false });

	if (lua_type(L, -1) != LUA_TNIL)
	lua_pcall(L, 0, 0, 0);
	else
	lua_pop(L, 1);

	int nargs = lua_rawlen(L, -1);
	int args_stack_pos = lua_gettop(L);

	for (int i = 0; i < nargs; i++)
		lua_geti(L, args_stack_pos, i + 1);

	lua_remove(L, args_stack_pos);

	int childid;

	switch (childid = fork())
	{
		case 0:
			lua_getglobal(L, "theApp");
			lua_pushboolean(L, 1);
			lua_setfield(L, -2, "isforked");
			lua_pop(L, 1);

			lua_pcall(L, nargs, LUA_MULTRET, 0);

			exit(0);
	}

	childProcessTracker.push_back(childid);

	return 0;
}

int LuaSysExecvpe(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaSysExecvpe argument table",
		{ "file", "args", "env" },
		{ LUA_TSTRING, LUA_TTABLE, LUA_TTABLE },
		{ true, true, false });

	const char* file = lua_tostring(L, -3);

	unsigned int argv_size = lua_rawlen(L, -2);
	char** argv = new char*[argv_size + 1];

	for (unsigned int i = 0; i < argv_size; i++)
	{
		lua_geti(L, -2, i + 1);
		argv[i] = new char[1024];
		sprintf(argv[i], "%s", lua_tostring(L, -1));
		lua_pop(L, 1);
	}
	argv[argv_size] = NULL;

	vector<string> envp_str;
	unsigned int envp_size = 0;

	if (lua_type(L, -1) != LUA_TNIL)
	{
		envp_size = lua_rawlen(L, -1);

		for (unsigned int i = 0; i < envp_size; i++)
		{
			lua_geti(L, -1, i + 1);
			string env_var = lua_tostring(L, -1);
			string env_value = getenv(env_var.c_str());

			env_var += "=" + env_value;

			envp_str.push_back(env_var.c_str());
		}
	}

	char** envp = new char*[envp_size + 1];

	for (unsigned int i = 0; i < envp_size; i++)
	{
		envp[i] = new char[1024];
		sprintf(envp[i], "%s", envp_str[i].c_str());
	}
	envp[envp_size] = NULL;

	int success = execvpe(file, argv, envp);

	if (success == -1) cerr << "Failed to run execvpe => " << errno << endl;

	return 0;
}

int LuaRegisterSysOpenConsts(lua_State* L)
{
	lua_pushinteger(L, O_RDONLY);
	lua_setglobal(L, "O_RDONLY");

	lua_pushinteger(L, O_WRONLY);
	lua_setglobal(L, "O_WRONLY");

	lua_pushinteger(L, O_RDWR);
	lua_setglobal(L, "O_RDWR");

	lua_pushinteger(L, O_APPEND);
	lua_setglobal(L, "O_APPEND");

	lua_pushinteger(L, O_ASYNC);
	lua_setglobal(L, "O_ASYNC");

	lua_pushinteger(L, O_CLOEXEC);
	lua_setglobal(L, "O_CLOEXEC");

	lua_pushinteger(L, O_CREAT);
	lua_setglobal(L, "O_CREAT");

	lua_pushinteger(L, O_DIRECT);
	lua_setglobal(L, "O_DIRECT ");

	lua_pushinteger(L, O_DIRECTORY);
	lua_setglobal(L, "O_DIRECTORY");

	lua_pushinteger(L, O_DSYNC);
	lua_setglobal(L, "O_DSYNC");

	lua_pushinteger(L, O_EXCL);
	lua_setglobal(L, "O_EXCL");

	lua_pushinteger(L, O_LARGEFILE);
	lua_setglobal(L, "O_LARGEFILE");

	lua_pushinteger(L, O_NOATIME);
	lua_setglobal(L, "O_NOATIME");

	lua_pushinteger(L, O_NOCTTY);
	lua_setglobal(L, "O_NOCTTY");

	lua_pushinteger(L, O_NOFOLLOW);
	lua_setglobal(L, "O_NOFOLLOW");

	lua_pushinteger(L, O_NONBLOCK);
	lua_setglobal(L, "O_NONBLOCK");

	lua_pushinteger(L, O_NDELAY);
	lua_setglobal(L, "O_NDELAY");

	lua_pushinteger(L, O_PATH);
	lua_setglobal(L, "O_PATH");

	lua_pushinteger(L, O_SYNC);
	lua_setglobal(L, "O_SYNC");

	lua_pushinteger(L, O_TMPFILE);
	lua_setglobal(L, "O_TMPFILE");

	lua_pushinteger(L, O_TRUNC);
	lua_setglobal(L, "O_TRUNC");

	// MODE_T CONSTANTS

	lua_pushinteger(L, S_IRWXU);
	lua_setglobal(L, "S_IRWXU");

	lua_pushinteger(L, S_IRUSR);
	lua_setglobal(L, "S_IRUSR");

	lua_pushinteger(L, S_IWUSR);
	lua_setglobal(L, "S_IWUSR");

	lua_pushinteger(L, S_IXUSR);
	lua_setglobal(L, "S_IXUSR");

	lua_pushinteger(L, S_IRWXG);
	lua_setglobal(L, "S_IRWXG");

	lua_pushinteger(L, S_IRGRP);
	lua_setglobal(L, "S_IRGRP");

	lua_pushinteger(L, S_IWGRP);
	lua_setglobal(L, "S_IWGRP");

	lua_pushinteger(L, S_IXGRP);
	lua_setglobal(L, "S_IXGRP");

	lua_pushinteger(L, S_IRWXO);
	lua_setglobal(L, "S_IRWXO");

	lua_pushinteger(L, S_IROTH);
	lua_setglobal(L, "S_IROTH");

	lua_pushinteger(L, S_IWOTH);
	lua_setglobal(L, "S_IWOTH");

	lua_pushinteger(L, S_IXOTH);
	lua_setglobal(L, "S_IXOTH");

	lua_pushinteger(L, S_ISUID);
	lua_setglobal(L, "S_ISUID");

	lua_pushinteger(L, S_ISGID);
	lua_setglobal(L, "S_ISGID");

	lua_pushinteger(L, S_ISVTX);
	lua_setglobal(L, "S_ISVTX");

	// CLOCK CONSTS

	lua_pushinteger(L, CLOCK_REALTIME);
	lua_setglobal(L, "CLOCK_REALTIME");

	lua_pushinteger(L, CLOCK_MONOTONIC);
	lua_setglobal(L, "CLOCK_MONOTONIC");

	lua_pushinteger(L, CLOCK_PROCESS_CPUTIME_ID);
	lua_setglobal(L, "CLOCK_PROCESS_CPUTIME_ID");

	lua_pushinteger(L, CLOCK_THREAD_CPUTIME_ID);
	lua_setglobal(L, "CLOCK_THREAD_CPUTIME_ID");

	// SEEK CONSTS

	lua_pushinteger(L, SEEK_SET);
	lua_setglobal(L, "SEEK_SET");

	lua_pushinteger(L, SEEK_CUR);
	lua_setglobal(L, "SEEK_CUR");

	lua_pushinteger(L, SEEK_END);
	lua_setglobal(L, "SEEK_END");

	lua_pushinteger(L, SEEK_DATA);
	lua_setglobal(L, "SEEK_DATA");

	lua_pushinteger(L, SEEK_HOLE);
	lua_setglobal(L, "SEEK_HOLE");

	return 0;
}

int LuaSysOpen(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaSysOpen argument table",
		{ "name", "flags", "mode" },
		{ LUA_TSTRING, LUA_TSTRING, LUA_TSTRING },
		{ true, false, false });

	const char* name = lua_tostring(L, -3);
	string flags_str = lua_tostringx(L, -2);
	string mode_str = lua_tostringx(L, -1);

	if (flags_str.empty()) flags_str = "O_RDONLY | O_NONBLOCK";
	if (mode_str.empty()) mode_str = "0666";

	int flags = GetFlagsFromOctalString(L, flags_str);
	mode_t mode = GetFlagsFromOctalString(L, mode_str);

	int fd_open = open(name, flags, mode);
	if (fd_open > maxFd) maxFd = fd_open;

	if (fd_open == -1)
	{
		cerr << "Error opening " << name << " => " << errno << endl;
		return 0;
	}

	lua_pushinteger(L, fd_open);

	return 1;
}

int LuaSysFtruncate(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaSysFtruncate argument table",
		{ "fd", "size" },
		{ LUA_TNUMBER, LUA_TNUMBER },
		{ true, true });

	int fd = lua_tointeger(L, -2);
	int size = lua_tointeger(L, -1);

	int success = ftruncate(fd, size);

	if (success == -1) cerr << "Failed to truncate the file: " << errno << endl;

	return 0;
}

int LuaSysClose(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "LuaSysClose", LUA_TNUMBER)) return 0;

	int fd_close = lua_tointeger(L, 1);

	close(fd_close);
	fd_close = -1;

	return 0;
}

int LuaSysLSeek(lua_State* L)
{
	int fd = lua_tointeger(L, 1);
	int offset = lua_tointegerx(L, 2, nullptr);
	int whence = lua_tointeger(L, 3);

	int newpos = lseek(fd, offset, whence);

	lua_pushinteger(L, newpos);

	return 1;
}

int MakePipe(lua_State* L)
{
	int pfds[2];

	if (pipe(pfds) == -1)
	{
		cerr << "Unabled to make new pipe..." << endl;
		return 0;
	}

	if (pfds[1] > maxFd) maxFd = pfds[1];

	lua_pushinteger(L, pfds[1]);
	lua_pushinteger(L, pfds[0]);

	return 2;
}

int MakeFiFo(lua_State* L)
{
	lua_unpackarguments(L, 1, "MakeFiFo argument table",
		{ "name", "mode" },
		{ LUA_TSTRING, LUA_TSTRING },
		{ true, false });

	const char* name = lua_tostring(L, -2);
	string mode_str = lua_tostringx(L, -1);

	remove(name);

	if (mode_str.empty()) mode_str = "0777";

	mode_t mode = GetFlagsFromOctalString(L, mode_str);

	if (mkfifo(name, mode) == -1)
	{
		cerr << "Error while opening the fifo..." << endl;
	}

	return 0;
}

int LuaSysDup(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "SysDup", LUA_TNUMBER)) return 0;

	int oldfd = lua_tointeger(L, 1);

	int newfd = dup(oldfd);
	if (newfd > maxFd) maxFd = newfd;

	lua_pushinteger(L, newfd);

	return 1;
}

int LuaSysDup2(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "SysDup", LUA_TNUMBER, LUA_TNUMBER)) return 0;

	int oldfd = lua_tointeger(L, 1);
	int newfd = lua_tointeger(L, 2);

	dup2(oldfd, newfd);

	if (oldfd > maxFd) maxFd = oldfd;
	if (newfd > maxFd) maxFd = newfd;

	return 0;
}

int maxFd = -1;

int LuaSysUnlink(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "LuaSysUnlink", LUA_TSTRING))
	{
		return 0;
	}

	const char* name = lua_tostring(L, 1);

	unlink(name);

	return 0;
}

int LuaSysRead(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaSysRead argument table",
		{ "fd", "size" },
		{ LUA_TNUMBER, LUA_TNUMBER },
		{ true, false });

	int rfd = lua_tointeger(L, -2);
	int read_length = lua_tointegerx(L, -1, nullptr);
	if (read_length == 0) ioctl(rfd, FIONREAD, &read_length);

	char* buffer = new char[read_length];

	int rbytes = read(rfd, buffer, read_length);

	if (rbytes < 0) cerr << "Error while reading: " << errno << endl;

	lua_pushlstring(L, buffer, rbytes);
	lua_pushinteger(L, rbytes);

	return 2;
}

int LuaSysWrite(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaSysRead argument table",
		{ "fd", "data", "size" },
		{ LUA_TNUMBER, LUA_TSTRING, LUA_TNUMBER },
		{ true, true, false });

	int wfd = lua_tointeger(L, -3);
	string msg = lua_tostring(L, -2);

	int msg_length = lua_tointegerx(L, -1, nullptr);
	if (msg_length == 0) msg_length = msg.length();

	int wbytes = write(wfd, msg.c_str(), msg_length);

	lua_pushinteger(L, wbytes);

	return 1;
}

int LuaSysSelect(lua_State* L)
{
	if (!CheckLuaArgs(L, 1, true, "LuaSysSelect", LUA_TTABLE)) return 0;

	timeval timer;
	fd_set readfds, writefds, exceptsfds;
	vector<int> rfd_list, wfd_list, efd_list;
	bool doread = false, dowrite = false, doexcepts = false;

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptsfds);

	timer.tv_sec = 10;
	timer.tv_usec = 0;

	if (lua_checkfield(L, 1, "timeout", LUA_TNUMBER))
	{
		timer.tv_sec = floor(lua_tonumber(L, -1));
		timer.tv_usec = ( lua_tonumber ( L, -1 ) - timer.tv_sec) * 1e6;
	}

	if (lua_checkfield(L, 1, "read", LUA_TTABLE))
	{
		doread = true;
		int readfd_size = lua_rawlen(L, -1);

		for (int i = 0; i < readfd_size; i++)
		{
			lua_geti(L, -1, i + 1);
			int fd = lua_tointeger(L, -1);
			lua_pop(L, 1);
			rfd_list.push_back(fd);
			FD_SET(fd, &readfds);
		}
	}

	if (lua_checkfield(L, 1, "write", LUA_TTABLE))
	{
		dowrite = true;
		int writefd_size = lua_rawlen(L, -1);

		for (int i = 0; i < writefd_size; i++)
		{
			lua_geti(L, -1, i + 1);
			int fd = lua_tointeger(L, -1);
			lua_pop(L, 1);
			wfd_list.push_back(fd);
			FD_SET(fd, &writefds);
		}
	}

	if (lua_checkfield(L, 1, "exception", LUA_TTABLE))
	{
		doexcepts = true;
		int exceptfd_size = lua_rawlen(L, -1);

		for (int i = 0; i < exceptfd_size; i++)
		{
			lua_geti(L, -1, i + 1);
			int fd = lua_tointeger(L, -1);
			lua_pop(L, 1);
			efd_list.push_back(fd);
			FD_SET(fd, &exceptsfds);
		}
	}

	int sel = select(maxFd + 1, &readfds, &writefds, &exceptsfds, &timer);

	// == -1 means an error occured during the select()
	// == 0 means it timed out
	if (sel == -1)
	{
		lua_pushnil(L);
		return 1;
	}
	else if (sel == 0)
	{
		lua_pushnumber(L, 0);
		return 1;
	}
	else
	{
		if (doread)
		{
			lua_newtable(L);

			for (unsigned int i = 0; i < rfd_list.size(); i++)
			{
				if (FD_ISSET(rfd_list[i], &readfds))
				{
					lua_pushinteger(L, rfd_list[i]);
					lua_seti(L, -2, i + 1);
				}
			}
		}
		else lua_pushnil(L);

		if (dowrite)
		{
			lua_newtable(L);

			for (unsigned int i = 0; i < wfd_list.size(); i++)
			{
				if (FD_ISSET(wfd_list[i], &writefds))
				{
					lua_pushinteger(L, wfd_list[i]);
					lua_seti(L, -2, i + 1);
				}
			}
		}
		else lua_pushnil(L);

		if (doexcepts)
		{
			lua_newtable(L);

			for (unsigned int i = 0; i < efd_list.size(); i++)
			{
				if (FD_ISSET(efd_list[i], &exceptsfds))
				{
					lua_pushinteger(L, efd_list[i]);
					lua_seti(L, -2, i + 1);
				}
			}
		}
		else lua_pushnil(L);

		return 3;
	}
}

int LuaSysFtok(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaSemFtok argument table",
		{ "pathname", "id" },
		{ LUA_TSTRING, LUA_TNUMBER },
		{ true, false });

	const char* pathname = lua_tostring(L, -2);
	int proj_id = lua_tointegerx(L, -1, nullptr);

	key_t fkey = ftok(pathname, proj_id > 0 ? proj_id : 'Z');

	lua_pushinteger(L, fkey);

	return 1;
}

int LuaOpenNewSlaveTerminal(lua_State* L)
{
	lua_unpackarguments(L, 1, "LuaOpenNewSlaveTerminal argument table",
		{ "title", "label", "bgcolor", "fgcolor", "fontstyle", "fontsize", "geometry" },
		{ LUA_TSTRING, LUA_TSTRING, LUA_TSTRING, LUA_TSTRING, LUA_TSTRING, LUA_TNUMBER, LUA_TSTRING },
		{ false, false, false, false, false, false, false });

	string title = lua_tostringx(L, -7);
	string label = lua_tostringx(L, -6);
	string bgcolor = lua_tostringx(L, -5);
	string fgcolor = lua_tostringx(L, -4);
	string fontstyle = lua_tostringx(L, -3);
	int fontsize = lua_tointegerx(L, -2, nullptr);
	string geomopts = lua_tostringx(L, -1);

	int pt = posix_openpt(O_RDWR);
	if (pt == -1)
	{
		std::cerr << "Could not open pseudo terminal.\n";
		return 0;
	}
	char* ptname = ptsname(pt);
	if (!ptname)
	{
		std::cerr << "Could not get pseudo terminal device name.\n";
		close(pt);
		return 0;
	}

	if (unlockpt(pt) == -1)
	{
		std::cerr << "Could not get pseudo terminal device name.\n";
		close(pt);
		return 0;
	}

	string ptname_str = ptname;

	std::ostringstream oss;

	oss << "xterm -ah" << (bgcolor.empty() ? "" : (string) (" -bg '" + bgcolor + "'")) << (fgcolor.empty() ? "" : (string) (" -fg '" + fgcolor + "'"))
			<< (fontstyle.empty() ? "" : (string) (" -fa '" + fontstyle + "'")) << (fontsize > 0 ? (string) (" -fs " + to_string(fontsize)) : "")
			<< (title.empty() ? "" : (string) (" -T '" + title + "'")) << (label.empty() ? "" : (string) (" -n '" + label + "'"))
			<< (geomopts.empty() ? "" : (string) (" -geometry '" + geomopts + "'")) << " -S" << (ptname_str.find_last_of("/") + 1) << "/" << pt << " &";
	int success = system(oss.str().c_str());

	if (success == -1)
	{
		cerr << "Error while creating slave terminal..." << errno << endl;
		return 0;
	}

	int xterm_fd = open(ptname, O_RDWR);

	lua_pushinteger(L, xterm_fd);

	return 1;
}
