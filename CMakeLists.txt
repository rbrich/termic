cmake_minimum_required(VERSION 3.16)
project(xciterm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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
