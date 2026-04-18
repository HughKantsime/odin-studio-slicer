cmake_minimum_required(VERSION 3.24)
project(nlopt_ios)
include(ExternalProject)
ExternalProject_Add(dep_NLopt
    URL https://github.com/stevengj/nlopt/archive/v2.5.0.tar.gz
    URL_HASH SHA256=c6dd7a5701fff8ad5ebb45a3dc8e757e61d52658de3918e38bab233e7fd3b4ae
    CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/../cmake/iOS.cmake
        -DCMAKE_OSX_DEPLOYMENT_TARGET=17.0
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
        -DNLOPT_PYTHON=OFF -DNLOPT_OCTAVE=OFF -DNLOPT_MATLAB=OFF
        -DNLOPT_GUILE=OFF -DNLOPT_SWIG=OFF -DNLOPT_TESTS=OFF
        -DBUILD_SHARED_LIBS=OFF
)
