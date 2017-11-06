add_library(ROOTBINDER_OBJECT OBJECT LuaTGraph.cxx)
add_library(RootBinderLib SHARED $<TARGET_OBJECTS:ROOTBINDER_OBJECT>)
target_link_libraries(RootBinderLib LuaXRootlib)

install(TARGETS RootBinderLib DESTINATION ${CMAKE_INSTALL_PREFIX})