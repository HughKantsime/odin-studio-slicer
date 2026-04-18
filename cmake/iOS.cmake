# cmake/iOS.cmake — toolchain for building libslic3r as a static archive
# for arm64-apple-ios. Invoke via:
#
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/iOS.cmake \
#         -DCMAKE_OSX_DEPLOYMENT_TARGET=17.0 \
#         -S . -B build-ios
#
# Notes:
# - iOS forbids fork()/exec(). libslic3r doesn't actually call those directly
#   but some deps (notably CGAL test harnesses, CURL's default config) do.
#   We compile with -DSLIC3R_NO_FORK to gate the offenders where the fork
#   patches live (see src/odin_slicer_ios_compat.cpp).
# - No Metal/Accelerate bindings needed at this layer — slicer is pure CPU.
# - Simulator builds are out of scope for v1; device-only (arm64-apple-ios).

set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_SYSTEM_PROCESSOR arm64)

# iOS device
set(CMAKE_OSX_SYSROOT iphoneos)
set(CMAKE_OSX_ARCHITECTURES arm64)
if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 17.0)
endif()

# Bitcode is deprecated by Apple; don't emit.
set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE NO)
# Position-independent code — required for static lib consumed by an app bundle.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Cross-compile guardrails
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# libslic3r uses try_run() in some dep bootstraps — disable for cross-compile.
set(CMAKE_CROSSCOMPILING ON)

# Strip unavailable-on-iOS APIs
add_compile_definitions(
    SLIC3R_NO_FORK=1
    SLIC3R_NO_SYSTEM=1
    SLIC3R_IOS=1
    BOOST_LOG_NO_THREAD_ATTR=1
    _LIBCPP_DISABLE_AVAILABILITY=1
)

# TBB + Eigen hints — built externally and passed via CMAKE_PREFIX_PATH,
# or via FetchContent from deps-ios.cmake (see sibling file).

message(STATUS "iOS toolchain active: sysroot=${CMAKE_OSX_SYSROOT} arch=${CMAKE_OSX_ARCHITECTURES} min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
