cmake_minimum_required(VERSION 3.16)
project(tlc_test)

set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

if(NOT DEFINED DUT_DIR)
    message(FATAL_ERROR "DUT_DIR is not set!")
endif()
include_directories(${DUT_DIR})

if(DEFINED CHISELDB)
    add_definitions(-DENABLE_CHISEL_DB)
endif()

if(DEFINED DUALCORE)
    add_definitions(-DDUALCORE)
endif()

if(DEFINED NOCHECK)
    add_definitions(-DNOCHECK)
endif()

if(DEFINED THREAD)
    add_definitions(-DVL_THREADED)
    find_package(Threads)
endif()

find_package(verilator)
find_package(SQLite3)

add_subdirectory(Emu)
add_subdirectory(TLAgent)
add_subdirectory(Fuzzer)

include_directories(${SQLite3_INCLUDE_DIRS})
add_executable(tlc_test main.cpp)
if(DEFINED THREAD)
    target_link_libraries(tlc_test Emu TLAgent Fuzzer Threads::Threads ${SQLite3_LIBRARIES})
else()
    target_link_libraries(tlc_test Emu TLAgent Fuzzer ${SQLite3_LIBRARIES})
endif()
