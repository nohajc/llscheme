cmake_minimum_required(VERSION 3.4)
project(llscheme)

find_library(LIB_UNIT_TEST_CPP NAMES UnitTest++)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

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
        src/ast_visitor.cpp)

set(EXEC_FILES
        src/driver.cpp
        include/driver.hpp src/ast_visitor.cpp)

set(TEST_FILES
        src/test/main.cpp src/test/environment.cpp src/test/any_ptr.cpp)

add_executable(schemec ${SOURCE_FILES} ${EXEC_FILES})
add_executable(unit_tests ${SOURCE_FILES} ${TEST_FILES})

llvm_map_components_to_libnames(llvm_libs support core irreader)

target_link_libraries(schemec ${llvm_libs})
target_link_libraries(unit_tests ${llvm_libs} ${LIB_UNIT_TEST_CPP})

add_custom_command(TARGET schemec POST_BUILD COMMAND "${PROJECT_SOURCE_DIR}/test/test.rb" "${PROJECT_SOURCE_DIR}/test/parser.tests")
