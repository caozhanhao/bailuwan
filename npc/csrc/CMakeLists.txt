# This CMakeLists is only used to configure CLion.
# It is not used for NPC building. For NPC, see ../Makefile
cmake_minimum_required(VERSION 4.0)
project(csrc)

set(CMAKE_CXX_STANDARD 14)

include_directories(../build/obj_dir)
include_directories(common)
include_directories($ENV{VERILATOR_HOME}/include)

add_compile_definitions(TOP_NAME=VTop TRACE TRACE_fst)

add_executable(debug debug/main.cpp)
add_executable(nvboard nvboard/main.cpp)
add_executable(sdb sdb/main.cpp sdb/cpu-exec.cpp sdb/expr.cpp sdb/watchpoint.cpp)