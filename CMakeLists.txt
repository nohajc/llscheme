cmake_minimum_required(VERSION 3.4)
project(llscheme)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        include/debug.hpp
        src/driver.cpp
        include/driver.hpp
        src/parser.cpp
        include/parser.hpp
        src/reader.cpp
        include/reader.hpp
        src/types.cpp
        include/types.hpp)

add_executable(llscheme ${SOURCE_FILES})