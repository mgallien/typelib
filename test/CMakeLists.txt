FIND_PACKAGE(Antlr REQUIRED COMPONENTS CPP)

CONFIGURE_FILE(testsuite.hh.in ${CMAKE_BINARY_DIR}/test/testsuite.hh @ONLY)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../lang/cimport ${CMAKE_CURRENT_BINARY_DIR}/../lang/cimport ${Antlr_INCLUDE_DIRS})

ADD_EXECUTABLE(typelib_testsuite testsuite.cc
    test_registry.cc
    test_display.cc test_lang_c.cc test_lang_tlb.cc test_model.cc
    test_plugin.cc test_value.cc test_containers.cc
    test_memory_layout.cc test_marshalling.cc)
add_dependencies(typelib_testsuite typeLang_c)

TARGET_LINK_LIBRARIES(typelib_testsuite typeLib ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES} ${Boost_SYSTEM_LIBRARIES} ${Boost_FILESYSTEM_LIBRARIES} ${Antlr_PIC_LIBRARIES})
ADD_TEST(CxxSuite ${CMAKE_CURRENT_BINARY_DIR}/typelib_testsuite --catch_system_errors=no)

