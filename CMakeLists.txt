cmake_minimum_required(VERSION 3.0)
project(llscheme)

find_library(LIB_UNIT_TEST_CPP NAMES UnitTest++)
find_library(LIB_BOEHM_GC gc)

find_library(LIB_BOOST_SYS boost_system)
find_library(LIB_BOOST_FS boost_filesystem)

find_package(LLVM REQUIRED CONFIG)

# Optional, needed for the basic post-build tests
find_program(RUBY ruby)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -DDBGBUILD")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

#set(LLVM_TARGETS_TO_BUILD all)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(SOURCE_FILES
        include/debug.hpp
        src/parser.cpp
        include/parser.hpp
        src/reader.cpp
        include/reader.hpp
        src/ast.cpp
        include/ast.hpp
        src/environment.cpp
        include/environment.hpp
        include/common.hpp
        src/codegen.cpp
        include/codegen.hpp
        include/any_ptr.hpp
        include/ast_visitor.hpp
        src/ast_visitor.cpp
        include/runtime/types.hpp
        include/libmetainfo.hpp src/libmetainfo.cpp
        src/fs_helpers.cpp include/fs_helpers.hpp
        src/lib_reader.cpp include/lib_reader.hpp
        include/elfio/elfio.hpp)

set(EXEC_FILES
        src/driver.cpp
        include/driver.hpp)

set(TEST_FILES
        src/test/main.cpp src/test/environment.cpp src/test/any_ptr.cpp)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(schemec ${SOURCE_FILES} ${EXEC_FILES})

# Add a custom rule to generate the Scheme part of our runtime library
add_custom_command(OUTPUT scmlib.o
        COMMAND schemec "${PROJECT_SOURCE_DIR}/src/runtime/scmlib.scm" -o "scmlib.o" -b lib)

set(RUNTIME_FILES
        include/runtime/types.hpp
        src/runtime/runtime.cpp include/runtime.h
        include/runtime/memory.h src/runtime/memory.cpp
        include/runtime/error.h src/runtime/error.cpp
        include/runtime/internal.hpp include/runtime/meta.hpp
        scmlib.o include/runtime/scmjit.hpp
        src/runtime/readlinestream.cpp include/runtime/readlinestream.hpp
        include/linenoise/linenoise.c include/linenoise/linenoise.h)

add_library(llscmrt SHARED ${RUNTIME_FILES} ${SOURCE_FILES})
set_target_properties(llscmrt PROPERTIES LINKER_LANGUAGE CXX)

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${PROJECT_SOURCE_DIR}/test/schemec)

llvm_map_components_to_libnames(llvm_libs support core irreader bitwriter
        analysis executionengine instcombine object runtimedyld scalaropts support native)

target_link_libraries(schemec ${llvm_libs} ${LIB_BOOST_SYS} ${LIB_BOOST_FS})

IF(CMAKE_CFG_INTDIR MATCHES "Debug" OR CMAKE_BUILD_TYPE MATCHES DEBUG)
    set(EXEC_DIR ${PROJECT_SOURCE_DIR}/bin/Debug)
ELSE()
    set(EXEC_DIR ${PROJECT_SOURCE_DIR}/bin/Release)
ENDIF()

target_link_libraries(schemec -Wl,-R -Wl,${EXEC_DIR})
target_link_libraries(llscmrt ${LIB_BOEHM_GC} ${llvm_libs} ${LIB_BOOST_SYS} ${LIB_BOOST_FS})

IF(LIB_UNIT_TEST_CPP)
    add_executable(unit_tests EXCLUDE_FROM_ALL ${SOURCE_FILES} ${TEST_FILES})
    target_link_libraries(unit_tests ${llvm_libs} ${LIB_UNIT_TEST_CPP})
ENDIF()

add_custom_command(TARGET schemec POST_BUILD
        COMMAND mkdir -p ${EXEC_DIR}
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${EXEC_DIR}/schemec ${PROJECT_SOURCE_DIR}/test/schemec)

# Run tests after the compiler is built
IF(RUBY)
    add_custom_command(TARGET schemec POST_BUILD COMMAND "${PROJECT_SOURCE_DIR}/test/test.rb" "${PROJECT_SOURCE_DIR}/test/parser.tests")
ENDIF()

add_custom_command(TARGET schemec POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:schemec> ${EXEC_DIR}/)
add_custom_command(TARGET llscmrt POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy  $<TARGET_FILE:llscmrt> ${EXEC_DIR}/)

add_dependencies(llscmrt schemec)
