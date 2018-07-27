cmake_minimum_required(VERSION 3.7)
project(xciterm CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(xcikit_DIR /usr/local/lib/cmake/xci)
find_package(xcikit REQUIRED)

add_executable(xciterm src/main.cpp src/Pty.cpp)
target_link_libraries(xciterm xci-widgets)
