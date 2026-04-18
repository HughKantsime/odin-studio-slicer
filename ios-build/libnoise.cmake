cmake_minimum_required(VERSION 3.24)
project(libnoise_ios)
include(ExternalProject)
ExternalProject_Add(dep_libnoise
    URL https://github.com/SoftFever/Orca-deps-libnoise/archive/refs/tags/1.0.zip
    URL_HASH SHA256=96ffd6cc47898dd8147aab53d7d1b1911b507d9dbaecd5613ca2649468afd8b6
    CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/../cmake/iOS.cmake
        -DCMAKE_OSX_DEPLOYMENT_TARGET=17.0
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
        -DCMAKE_MACOSX_BUNDLE=OFF
        -DBUILD_SHARED_LIBS=OFF
)
