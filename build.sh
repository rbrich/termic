#!/usr/bin/env bash

set -e
cd "$(dirname "$0")"

ROOT_DIR="$PWD"
BUILD_TYPE="Release"
BUILD_CONFIG="${BUILD_TYPE}"
BUILD_DIR="${ROOT_DIR}/build/${BUILD_CONFIG}"
INSTALL_DIR="${ROOT_DIR}/artifacts/${BUILD_CONFIG}"
JOBS_ARGS=()
CMAKE_ARGS=()
CONAN_ARGS=()

print_usage()
{
    echo "Usage: ./build.sh [--profile, -pr PROFILE] [--toolchain FILE]"
}

while [[ $# -gt 0 ]] ; do
    case "$1" in
        -pr | --profile )
            CONAN_ARGS+=('--profile' "$2")
            shift 2 ;;
        --toolchain )
            CMAKE_ARGS+=(-D"CMAKE_TOOLCHAIN_FILE=$2")
            shift 2 ;;
        -h | --help )
            print_usage
            exit 0 ;;
        * )
            printf 'Error: Unknown option: %s\n\n' "$1"
            print_usage
            exit 1 ;;
    esac
done

echo "CONAN_ARGS:   ${CONAN_ARGS[*]}"
echo "CMAKE_ARGS:   ${CMAKE_ARGS[*]}"
echo "BUILD_DIR:    ${BUILD_DIR}"
echo "INSTALL_DIR:  ${INSTALL_DIR}"

conan export external/xcikit rbrich/stable

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

conan install "${ROOT_DIR}" \
    --build missing \
    -s "build_type=${BUILD_TYPE}" \
    "${CONAN_ARGS[@]}"

cmake "${ROOT_DIR}" \
    "${CMAKE_ARGS[@]}" \
    -D"CMAKE_BUILD_TYPE=${BUILD_TYPE}" \
    -D"CMAKE_INSTALL_PREFIX=${INSTALL_DIR}"

cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}"
ctest --progress --output-on-failure --build-config "${BUILD_TYPE}" "${JOBS_ARGS[@]}"
