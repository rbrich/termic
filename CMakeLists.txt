cmake_minimum_required(VERSION 3.12)
project(xciterm CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13.4")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(ConanInit)

find_package(xcikit CONFIG REQUIRED)
find_package(Catch2)

include(XciBuildOptions)

add_executable(xciterm
    src/main.cpp
    src/Pty.cpp
    src/Shell.cpp
    src/Terminal.cpp
    src/utility.cpp
    )
target_link_libraries(xciterm
    xcikit::xci-widgets
    xcikit::xci-graphics)

if (Catch2_FOUND)
    enable_testing()
    add_subdirectory(tests)
endif()
