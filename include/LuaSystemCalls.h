#ifndef __LUASYSTEMCALLS__
#define __LUASYSTEMCALLS__

#include <sys/types.h>
#include <fcntl.h>

#include "LuaExtension.h"

extern int maxFd;

int LuaRegisterSysOpenConsts ( lua_State* L );

int LuaGetEnv(lua_State* L);
int LuaSysFork(lua_State* L);
int LuaSysExecvpe(lua_State* L);

int LuaSysOpen ( lua_State* L );
int LuaSysClose ( lua_State* L );
int MakePipe ( lua_State* L );
int MakeFiFo ( lua_State* L );
int LuaSysDup ( lua_State* L );
int LuaSysDup2 ( lua_State* L );

int LuaSysUnlink ( lua_State* L );
int LuaSysRead ( lua_State* L );
int LuaSysWrite ( lua_State* L );

int LuaSysSelect ( lua_State* L );

#endif
