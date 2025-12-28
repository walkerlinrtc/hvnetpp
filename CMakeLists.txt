cmake_minimum_required(VERSION 3.10)

project(hvnetpp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # Generate position independent code (fPIC)

# Find dependencies
find_package(Threads REQUIRED)
find_package(Backtrace)

# Include directories
# Add src/thirdparty/rtclog so we can include "rtclog.h"
include_directories(include)
include_directories(src/thirdparty/rtclog)

# Source files
# Note: Added .cc extension to include easylogging++.cc
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c" "src/*.cc")
file(GLOB_RECURSE HEADERS "include/hvnetpp/*.h" "src/thirdparty/rtclog/*.h")

# Create the static library
add_library(hvnetpp STATIC ${SOURCES} ${HEADERS})

# Configure target include directories
target_include_directories(hvnetpp PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/thirdparty/rtclog>
    $<INSTALL_INTERFACE:include>
)

# Link libraries
target_link_libraries(hvnetpp PUBLIC Threads::Threads)

if(Backtrace_FOUND)
    target_link_libraries(hvnetpp PRIVATE ${Backtrace_LIBRARIES})
    target_include_directories(hvnetpp PRIVATE ${Backtrace_INCLUDE_DIRS})
    add_definitions(-DHAVE_BACKTRACE) # Optional: Define macro if code uses it
endif()

# Test executable
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_build.cpp")
    add_executable(test_build test_build.cpp)
    target_link_libraries(test_build PRIVATE hvnetpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/demo_timer.cpp")
    add_executable(demo_timer demo_timer.cpp)
    target_link_libraries(demo_timer PRIVATE hvnetpp)
endif()
