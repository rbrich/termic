#!/usr/bin/env bash

set -e
cd "$(dirname "$0")"

ROOT_DIR="$PWD"
BUILD_TYPE="Release"
BUILD_CONFIG="${BUILD_TYPE}"
BUILD_DIR="${ROOT_DIR}/build/${BUILD_CONFIG}"
INSTALL_DIR="${ROOT_DIR}/artifacts/${BUILD_CONFIG}"
JOBS_ARGS=()
CMAKE_ARGS=(-G Ninja
    -DXCI_DATA=OFF -DXCI_SCRIPT=OFF -DXCI_BUILD_TOOLS=OFF -DXCI_BUILD_EXAMPLES=OFF -DXCI_BUILD_BENCHMARKS=OFF
    -DXCI_INSTALL_DEVEL=OFF)
CONAN_ARGS=()
CONAN_PROFILE="default"

run() { echo "➤➤➤ $@"; "$@"; }

print_usage()
{
    echo "Usage: ./build.sh [--profile, -pr PROFILE] [--toolchain FILE]"
}

while [[ $# -gt 0 ]] ; do
    case "$1" in
        -pr | --profile )
            CONAN_PROFILE="$2"
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

CONAN_ARGS+=("-pr=$CONAN_PROFILE" "-pr:b=$CONAN_PROFILE")

echo "CONAN_ARGS:   ${CONAN_ARGS[*]}"
echo "CMAKE_ARGS:   ${CMAKE_ARGS[*]}"
echo "BUILD_DIR:    ${BUILD_DIR}"
echo "INSTALL_DIR:  ${INSTALL_DIR}"

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

if [[ ! -f 'system_deps.txt' ]] ; then
    DETECT_ARGS=(graphics text widgets tests)
    "${ROOT_DIR}/external/xcikit/detect_system_deps.py" "${DETECT_ARGS[@]}" | tee 'system_deps.txt'
fi
CONAN_ARGS+=($(tail -n1 'system_deps.txt'))
CMAKE_ARGS+=($(tail -n2 'system_deps.txt' | head -n1))

run conan install "${ROOT_DIR}/external/xcikit" \
    --build missing \
    -s "build_type=${BUILD_TYPE}" \
    "${CONAN_ARGS[@]}"

cmake "${ROOT_DIR}" \
    "${CMAKE_ARGS[@]}" \
    -D"CMAKE_BUILD_TYPE=${BUILD_TYPE}" \
    -D"CMAKE_INSTALL_PREFIX=${INSTALL_DIR}"

cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}"
ctest --progress --output-on-failure --build-config "${BUILD_TYPE}" "${JOBS_ARGS[@]}"
