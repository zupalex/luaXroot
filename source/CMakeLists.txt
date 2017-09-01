root_generate_dictionary(LuaRootBinderDictionnary LuaRootBinder.h LINKDEF LuaRootBinderLinkDef.h)

set (LUAXROOT_SRCS LuaExtension.cxx LuaRootBinder.cxx LuaRootBinderDictionnary.cxx)

add_library(LUAXROOT_OBJECT OBJECT ${LUAXROOT_SRCS})
add_library(LuaXRootlib SHARED $<TARGET_OBJECTS:LUAXROOT_OBJECT>)
target_link_libraries(LuaXRootlib ${ROOT_LIBRARIES} lualib)

install(TARGETS LuaXRootlib DESTINATION ${CMAKE_INSTALL_PREFIX})

if (${ROOT_VERSION} VERSION_GREATER "6.0")
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/LuaRootBinderDictionnary_rdict.pcm  DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()