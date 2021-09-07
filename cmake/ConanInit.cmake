option(CONAN_INSTALL "Run 'conan install' from CMake (this may be more convenient than separate command)" OFF)
set(CONAN_PROFILE "default" CACHE STRING "Conan profile ot use in 'conan install'")

# Run conan install directly
# See https://github.com/conan-io/cmake-conan
if (CONAN_INSTALL)
    if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
        message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
        file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.16.1/conan.cmake"
            "${CMAKE_BINARY_DIR}/conan.cmake"
            TLS_VERIFY ON
            LOG dl_log
            STATUS dl_status)
        if (NOT 0 IN_LIST dl_status)
            file(REMOVE "${CMAKE_BINARY_DIR}/conan.cmake")
            message(STATUS "Download: ${dl_log}")
            message(SEND_ERROR "Download failed: ${dl_status}")
        endif()
    endif()

    include(${CMAKE_BINARY_DIR}/conan.cmake)

    conan_cmake_install(
        PATH_OR_REFERENCE ${CMAKE_SOURCE_DIR}
        PROFILE ${CONAN_PROFILE}
        SETTINGS
            build_type=${CMAKE_BUILD_TYPE}
        BUILD missing)
endif()

# Enable lookup for Conan dependencies
if (EXISTS ${CMAKE_BINARY_DIR}/conan_paths.cmake)
    include(${CMAKE_BINARY_DIR}/conan_paths.cmake)
endif()
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
