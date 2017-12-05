set (LUAROOTCLASSES_SRCS LuaRootClasses.cxx LuaTCanvas.cxx LuaTFile.cxx LuaTF1.cxx LuaTGraph.cxx LuaTHist.cxx LuaTSpectrum.cxx LuaTCutG.cxx LuaTTree.cxx)

add_library(ROOTBINDER_OBJECT OBJECT ${LUAROOTCLASSES_SRCS})
add_library(RootBinderLib SHARED $<TARGET_OBJECTS:ROOTBINDER_OBJECT>)
target_link_libraries(RootBinderLib LuaXRootlib)

install(TARGETS RootBinderLib DESTINATION ${CMAKE_INSTALL_PREFIX})