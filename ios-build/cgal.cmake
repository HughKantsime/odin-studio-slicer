cmake_minimum_required(VERSION 3.24)
project(cgal_ios)
include(ExternalProject)
ExternalProject_Add(dep_CGAL
    URL https://github.com/CGAL/cgal/releases/download/v5.6.1/CGAL-5.6.1.tar.xz
    URL_HASH SHA256=cdb15e7ee31e0663589d3107a79988a37b7b1719df3d24f2058545d1bcdd5837
    CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/../cmake/iOS.cmake
        -DCMAKE_OSX_DEPLOYMENT_TARGET=17.0
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
        -DWITH_examples=OFF
        -DWITH_tests=OFF
        -DCGAL_HEADER_ONLY=ON
        -DBOOST_ROOT=/Users/ollama/Documents/Claude/odin-studio-slicer/build-boost-ios/build/dep_Boost-prefix
        -DGMP_INCLUDE_DIR=/tmp/gmp-ios-install/include
        -DGMP_LIBRARIES=/tmp/gmp-ios-install/lib/libgmp.a
        -DMPFR_INCLUDE_DIR=/tmp/mpfr-ios-install/include
        -DMPFR_LIBRARIES=/tmp/mpfr-ios-install/lib/libmpfr.a
)
