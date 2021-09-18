cmake_minimum_required(VERSION 3.16)
project(xciterm LANGUAGES CXX)

option(WITH_XCIKIT_PACKAGE "Use packaged xcikit. Otherwise, use Git submodule." OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (WITH_XCIKIT_PACKAGE)
    find_package(xcikit CONFIG REQUIRED)
else()
    add_subdirectory(external/xcikit)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/external/xcikit/cmake)
    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_BINARY_DIR}/external/xcikit)
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink external/xcikit/share share)
endif()

find_package(Catch2)

include(XciBuildOptions)

add_executable(xciterm
    src/main.cpp
    src/Pty.cpp
    src/Shell.cpp
    src/Terminal.cpp
    src/utility.cpp
    )
target_link_libraries(xciterm xcikit::xci-widgets)

if (Catch2_FOUND)
    enable_testing()
    add_subdirectory(tests)
endif()
