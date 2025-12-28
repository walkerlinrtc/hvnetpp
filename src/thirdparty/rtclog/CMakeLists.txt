cmake_minimum_required(VERSION 3.10)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package(Threads)
find_package(Backtrace)

add_library(rtclog
  easylogging++.cc
  rtclog.cpp)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

target_link_libraries(rtclog
  PRIVATE
    ${CMAKE_THREAD_LIBS_INIT}
    ${Backtrace_LIBRARIES})

if (BUILD_GUI_DEPS)
    install(TARGETS rtclog
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib)
endif()

set_property(TARGET rtclog APPEND PROPERTY COMPILE_FLAGS "-fPIC")

# Demo executable
option(BUILD_DEMO "Build demo application" OFF)

if(BUILD_DEMO)
    add_executable(rtclog_demo demo.cpp)
    target_link_libraries(rtclog_demo rtclog ${CMAKE_THREAD_LIBS_INIT})
endif()