ADD_EXECUTABLE(typelib_exec
    import.cc inspect.cc mode.cc plugin.cc preprocess.cc
    typelib.cc cimportplugin.cc tlbimportplugin.cc
)

TARGET_LINK_LIBRARIES(typelib_exec typeLib)

INSTALL(TARGETS typelib_exec
    RUNTIME DESTINATION bin)

