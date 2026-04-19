# cmake/iOS-simulator.cmake — toolchain for building libslic3r as a static
# archive for arm64-apple-ios-simulator. Sibling of cmake/iOS.cmake;
# identical behaviour except the SDK sysroot + target triple.
#
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/iOS-simulator.cmake \
#         -DCMAKE_OSX_DEPLOYMENT_TARGET=17.0 \
#         -S . -B build-ios-sim
#
# Output objects carry LC_BUILD_VERSION platform 7 (iOS simulator). All
# SLIC3R_IOS patches apply unchanged — the platform distinction is only
# about which SDK we link against, not how the slicer itself behaves.

set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_SYSTEM_PROCESSOR arm64)

# iOS simulator
set(CMAKE_OSX_SYSROOT iphonesimulator)
set(CMAKE_OSX_ARCHITECTURES arm64)
if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 17.0)
endif()

# Explicit triple drops `-miphoneos-version-min` (wrong for simulator) and
# emits `-target arm64-apple-ios<ver>-simulator` so Clang tags the Mach-O
# with the simulator platform, not device.
set(CMAKE_C_COMPILER_TARGET   "arm64-apple-ios${CMAKE_OSX_DEPLOYMENT_TARGET}-simulator")
set(CMAKE_CXX_COMPILER_TARGET "arm64-apple-ios${CMAKE_OSX_DEPLOYMENT_TARGET}-simulator")

set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE NO)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_CROSSCOMPILING ON)

add_compile_definitions(
    SLIC3R_NO_FORK=1
    SLIC3R_NO_SYSTEM=1
    SLIC3R_IOS=1
    BOOST_LOG_NO_THREAD_ATTR=1
    _LIBCPP_DISABLE_AVAILABILITY=1
)

message(STATUS "iOS simulator toolchain active: sysroot=${CMAKE_OSX_SYSROOT} arch=${CMAKE_OSX_ARCHITECTURES} triple=${CMAKE_CXX_COMPILER_TARGET}")
