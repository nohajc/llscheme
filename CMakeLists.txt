cmake_minimum_required(VERSION 3.4)
project(llscheme)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -DDEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(SOURCE_FILES
        include/debug.hpp
        src/driver.cpp
        include/driver.hpp
        src/parser.cpp
        include/parser.hpp
        src/reader.cpp
        include/reader.hpp
        src/ast.cpp
        include/ast.hpp)

add_executable(schemec ${SOURCE_FILES})

llvm_map_components_to_libnames(llvm_libs support core irreader)

target_link_libraries(schemec ${llvm_libs})

add_custom_command(TARGET schemec POST_BUILD COMMAND "${PROJECT_SOURCE_DIR}/test/test.rb" "${PROJECT_SOURCE_DIR}/test/parser.tests")
