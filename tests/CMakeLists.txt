add_executable(test_util
    test_util.cpp
    ../src/utility.cpp)
target_link_libraries(test_util Catch2::Catch2 xcikit::xci-core)
target_include_directories(test_util PRIVATE ../src)
add_test(NAME test_util COMMAND test_util)
