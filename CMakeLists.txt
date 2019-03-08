cmake_minimum_required(VERSION 3.12)
project(xciterm CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13.4")

if (EXISTS ${CMAKE_BINARY_DIR}/conan_paths.cmake)
    include(${CMAKE_BINARY_DIR}/conan_paths.cmake)
endif()

find_package(xcikit REQUIRED)
find_package(Catch2)

include(XciBuildOptions)

add_executable(xciterm
    src/main.cpp
    src/Pty.cpp
    src/Shell.cpp
    src/Terminal.cpp
    src/utility.cpp
    src/utility.h)
target_link_libraries(xciterm
    xcikit::xci-widgets
    xcikit::xci-graphics-opengl)

if (Catch_FOUND)
    enable_testing()
    add_subdirectory(tests)
endif()
