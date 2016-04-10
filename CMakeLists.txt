cmake_minimum_required(VERSION 3.4)
project(llscheme)

find_library(LIB_UNIT_TEST_CPP NAMES UnitTest++)
find_library(LIB_BOEHM_GC gc)

find_package(LLVM REQUIRED CONFIG)

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
        include/libmetainfo.hpp src/libmetainfo.cpp)

set(EXEC_FILES
        src/driver.cpp
        include/driver.hpp)

set(TEST_FILES
        src/test/main.cpp src/test/environment.cpp src/test/any_ptr.cpp)

add_executable(schemec ${SOURCE_FILES} ${EXEC_FILES})
add_executable(unit_tests ${SOURCE_FILES} ${TEST_FILES})

set(RUNTIME_FILES
        include/runtime/types.hpp
        src/runtime/runtime.cpp include/runtime.h
        include/runtime/memory.h src/runtime/memory.cpp
        include/runtime/error.h src/runtime/error.cpp
        include/runtime/internal.hpp include/runtime/meta.hpp
        src/runtime/scmlib.o include/runtime/scmjit.hpp
        src/runtime/readlinestream.cpp include/runtime/readlinestream.hpp
        include/linenoise/linenoise.c include/linenoise/linenoise.h)

add_library(llscmrt SHARED ${RUNTIME_FILES} ${SOURCE_FILES})

llvm_map_components_to_libnames(llvm_libs support core irreader bitwriter
        analysis executionengine instcombine object runtimedyld scalaropts support native)

target_link_libraries(schemec ${llvm_libs} -Wl,-R -Wl,${llscheme_SOURCE_DIR}/bin/Debug)
target_link_libraries(llscmrt ${LIB_BOEHM_GC} ${llvm_libs})

target_link_libraries(unit_tests ${llvm_libs} ${LIB_UNIT_TEST_CPP})

add_custom_command(TARGET schemec POST_BUILD COMMAND "${PROJECT_SOURCE_DIR}/test/test.rb" "${PROJECT_SOURCE_DIR}/test/parser.tests")
