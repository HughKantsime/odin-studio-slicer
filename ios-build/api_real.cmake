cmake_minimum_required(VERSION 3.24)
project(odin_slicer_api_real LANGUAGES CXX)
#
# Build the real odin_slicer C API that links against libslic3r.
# Assumes libslic3r was already built (see main iOS build flow).
# Run from a scratch dir with -DSLICER_ROOT=<fork root> -DLIBSLIC3R_BUILD_DIR=<fork>/build-libslic3r-ios
#

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (NOT DEFINED SLICER_ROOT)
    set(SLICER_ROOT "${CMAKE_SOURCE_DIR}/..")
endif()
if (NOT DEFINED LIBSLIC3R_BUILD_DIR)
    set(LIBSLIC3R_BUILD_DIR "${SLICER_ROOT}/build-libslic3r-ios")
endif()
if (NOT DEFINED DEPS_PREFIX)
    set(DEPS_PREFIX "/tmp/odin-slicer-ios-deps")
endif()

add_library(odin_slicer_real STATIC
    ${SLICER_ROOT}/src/odin_slicer_api_real.cpp
)

target_include_directories(odin_slicer_real PUBLIC
    ${SLICER_ROOT}/include
)

target_include_directories(odin_slicer_real PRIVATE
    ${SLICER_ROOT}/src
    ${SLICER_ROOT}/deps_src
    ${LIBSLIC3R_BUILD_DIR}/src
    ${DEPS_PREFIX}/include
    ${DEPS_PREFIX}/include/eigen3
    ${LIBSLIC3R_BUILD_DIR}/src/libslic3r
)

target_compile_definitions(odin_slicer_real PRIVATE
    SLIC3R_IOS=1
    SLIC3R_NO_FORK=1
    USE_TBB=1
    TBB_USE_CAPTURED_EXCEPTION=0
    BOOST_LOG_NO_THREAD_ATTR=1
)

# Bake the fork commit SHA into the library so `odin_slicer_fork_commit()`
# returns an immutable identifier at runtime. Falls back to "unknown" when
# git isn't available (source archive builds).
execute_process(
    COMMAND git -C ${SLICER_ROOT} rev-parse --short=12 HEAD
    OUTPUT_VARIABLE ODIN_SLICER_FORK_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE GIT_RESULT
)
if (NOT GIT_RESULT EQUAL 0 OR ODIN_SLICER_FORK_COMMIT STREQUAL "")
    set(ODIN_SLICER_FORK_COMMIT "unknown")
endif()
target_compile_definitions(odin_slicer_real PRIVATE
    ODIN_SLICER_FORK_COMMIT="${ODIN_SLICER_FORK_COMMIT}"
)
message(STATUS "odin_slicer fork commit baked in: ${ODIN_SLICER_FORK_COMMIT}")

set_target_properties(odin_slicer_real PROPERTIES
    OUTPUT_NAME odin_slicer
    XCODE_ATTRIBUTE_ENABLE_BITCODE NO
)
