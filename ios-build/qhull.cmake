cmake_minimum_required(VERSION 3.24)
project(qhull_ios)
include(ExternalProject)
ExternalProject_Add(dep_Qhull
    URL https://github.com/qhull/qhull/archive/v8.0.2.zip
    URL_HASH SHA256=a378e9a39e718e289102c20d45632f873bfdc58a7a5f924246ea4b176e185f1e
    PATCH_COMMAND ${CMAKE_SOURCE_DIR}/prepend_no_bundle.sh
    CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/../cmake/iOS.cmake
        -DCMAKE_OSX_DEPLOYMENT_TARGET=17.0
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
        -DBUILD_STATIC_LIBS=ON
        -DBUILD_SHARED_LIBS=OFF
)
