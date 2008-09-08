ADD_EXECUTABLE(typelib_exec
    import.cc inspect.cc mode.cc plugin.cc preprocess.cc
    typelib.cc cimportplugin.cc tlbimportplugin.cc
)

SET_TARGET_PROPERTIES(typelib_exec
    PROPERTIES
        OUTPUT_NAME typelib
	INSTALL_RPATH_USE_LINK_PATH TRUE
)

TARGET_LINK_LIBRARIES(typelib_exec typeLib)

INSTALL(TARGETS typelib_exec
    RUNTIME DESTINATION bin)

