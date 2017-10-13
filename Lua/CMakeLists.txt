project(lua)

set(LUA_HEADERS lapi.h lauxlib.h lcode.h lctype.h ldebug.h ldo.h lfunc.h lgc.h llex.h llimits.h lmem.h lobject.c lopcodes.h lparser.h lprefix.h lstate.h lstring.h ltable.h ltm.h 
lua.h luaconf.h lualib.h lundump.h lvm.h lzio.h)

set(LUA_CORE_SOURCES lapi.c lcode.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c ltm.c lundump.c lvm.c lzio.c lctype.c)

set(LUA_LIB_SOURCES lauxlib.c lbaselib.c ldblib.c liolib.c lmathlib.c loslib.c ltablib.c lstrlib.c loadlib.c lbitlib.c lcorolib.c lutf8lib.c linit.c)

set_source_files_properties(${LUA_CORE_SOURCES} ${LUA_LIB_SOURCES} lua.c PROPERTIES LANGUAGE CXX)

add_library(LUALIB_OBJECTS OBJECT ${LUA_CORE_SOURCES} ${LUA_LIB_SOURCES})
target_compile_options(LUALIB_OBJECTS PUBLIC -DLUA_COMPAT_5_2 -DLUA_USE_LINUX)
add_library(lualib SHARED $<TARGET_OBJECTS:LUALIB_OBJECTS>)
target_link_libraries(lualib ${ROOT_LIBRARIES})

install(TARGETS lualib DESTINATION ${CMAKE_INSTALL_PREFIX})

add_executable (lua lua.c)
target_link_libraries (lua lualib)
target_link_libraries(lua -lm -Wl,-E -ldl)
target_compile_options(lua PUBLIC -DLUA_COMPAT_5_2 -DLUA_USE_LINUX)

if ( READLINE_FOUND )
  target_link_libraries(lua ${READLINE_LIBRARY})
endif ()
install(TARGETS lua DESTINATION ${CMAKE_INSTALL_PREFIX})