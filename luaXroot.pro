# Created by and for Qt Creator. This file was created for editing the project sources only.
# You may attempt to use it for building too, by modifying this file here.

#TARGET = luaXroot

HEADERS = \
   $$PWD/include/LuaExtension.h \
   $$PWD/include/LuaRootBinder.h \
   $$PWD/include/LuaRootBinderLinkDef.h \
   $$PWD/include/LuaSocketBinder.h \
   $$PWD/include/UserClassBase.h \
   $$PWD/Lua/lapi.h \
   $$PWD/Lua/lauxlib.h \
   $$PWD/Lua/lcode.h \
   $$PWD/Lua/lctype.h \
   $$PWD/Lua/ldebug.h \
   $$PWD/Lua/ldo.h \
   $$PWD/Lua/lfunc.h \
   $$PWD/Lua/lgc.h \
   $$PWD/Lua/llex.h \
   $$PWD/Lua/llimits.h \
   $$PWD/Lua/lmem.h \
   $$PWD/Lua/lobject.h \
   $$PWD/Lua/lopcodes.h \
   $$PWD/Lua/lparser.h \
   $$PWD/Lua/lprefix.h \
   $$PWD/Lua/lstate.h \
   $$PWD/Lua/lstring.h \
   $$PWD/Lua/ltable.h \
   $$PWD/Lua/ltm.h \
   $$PWD/Lua/lua.h \
   $$PWD/Lua/lua.hpp \
   $$PWD/Lua/luaconf.h \
   $$PWD/Lua/lualib.h \
   $$PWD/Lua/lundump.h \
   $$PWD/Lua/lvm.h \
   $$PWD/Lua/lzio.h

SOURCES = \
   $$PWD/cmake/modules/ExportDYLD.cmake \
   $$PWD/cmake/modules/FindReadline.cmake \
   $$PWD/cmake/modules/FindROOT.cmake \
   $$PWD/exec/liblualib.so \
   $$PWD/exec/libLuaXRootlib.so \
   $$PWD/exec/lua \
   $$PWD/exec/LuaRootBinderDictionnary_rdict.pcm \
   $$PWD/Lua/CMakeLists.txt \
   $$PWD/Lua/lapi.c \
   $$PWD/Lua/lauxlib.c \
   $$PWD/Lua/lbaselib.c \
   $$PWD/Lua/lbitlib.c \
   $$PWD/Lua/lcode.c \
   $$PWD/Lua/lcorolib.c \
   $$PWD/Lua/lctype.c \
   $$PWD/Lua/ldblib.c \
   $$PWD/Lua/ldebug.c \
   $$PWD/Lua/ldo.c \
   $$PWD/Lua/ldump.c \
   $$PWD/Lua/lfunc.c \
   $$PWD/Lua/lgc.c \
   $$PWD/Lua/linit.c \
   $$PWD/Lua/liolib.c \
   $$PWD/Lua/llex.c \
   $$PWD/Lua/lmathlib.c \
   $$PWD/Lua/lmem.c \
   $$PWD/Lua/loadlib.c \
   $$PWD/Lua/lobject.c \
   $$PWD/Lua/lopcodes.c \
   $$PWD/Lua/loslib.c \
   $$PWD/Lua/lparser.c \
   $$PWD/Lua/lstate.c \
   $$PWD/Lua/lstring.c \
   $$PWD/Lua/lstrlib.c \
   $$PWD/Lua/ltable.c \
   $$PWD/Lua/ltablib.c \
   $$PWD/Lua/ltm.c \
   $$PWD/Lua/lua.c \
   $$PWD/Lua/luac.c \
   $$PWD/Lua/lundump.c \
   $$PWD/Lua/lutf8lib.c \
   $$PWD/Lua/lvm.c \
   $$PWD/Lua/lzio.c \
   $$PWD/scripts/lua_modules/binaryreader.lua \
   $$PWD/scripts/lua_modules/lua_helper.lua \
   $$PWD/scripts/lua_modules/lua_root_binder.lua \
   $$PWD/scripts/lua_modules/lua_tree.lua \
   $$PWD/scripts/lua_modules/serpent.lua \
   $$PWD/scripts/luaXrootlogon.lua \
   $$PWD/scripts/thisluaXroot.sh \
   $$PWD/source/CMakeLists.txt \
   $$PWD/source/LuaExtension.cxx \
   $$PWD/source/LuaRootBinder.cxx \
   $$PWD/source/LuaSocketBinder.cxx \
   $$PWD/source/UserClassBase.cxx \
   $$PWD/source/UserClassBase_cxx.d \
   $$PWD/source/UserClassBase_cxx.so \
   $$PWD/source/UserClassBase_cxx_ACLiC_dict_rdict.pcm \
   $$PWD/user/Examples/LateCompilation.cxx \
   $$PWD/user/Examples/LateCompilation_cxx.d \
   $$PWD/user/Examples/LateCompilation_cxx.so \
   $$PWD/user/Examples/LateCompilation_cxx_ACLiC_dict_rdict.pcm \
   $$PWD/user/nscl_unpacker/2011_declasses.cxx \
   $$PWD/user/nscl_unpacker/2011_declasses_cxx.d \
   $$PWD/user/nscl_unpacker/2011_declasses_cxx.so \
   $$PWD/user/nscl_unpacker/2011_declasses_cxx_ACLiC_dict_rdict.pcm \
   $$PWD/user/nscl_unpacker/debug_log.log \
   $$PWD/user/nscl_unpacker/nscl_itempackets.lua \
   $$PWD/user/nscl_unpacker/nscl_physicspackets.lua \
   $$PWD/user/nscl_unpacker/nscl_unpacker.lua \
   $$PWD/user/nscl_unpacker/test.root \
   $$PWD/user/nscl_unpacker/test_unpack.root.root \
   $$PWD/user/ldf_generator.lua \
   $$PWD/user/ldf_onlinereader.lua \
   $$PWD/user/test.ldf \
   $$PWD/user/userlogon.lua \
   $$PWD/CMakeLists.txt \
   $$PWD/README.md

INCLUDEPATH = \
    $$PWD/include \
    $$PWD/Lua

#DEFINES = 

