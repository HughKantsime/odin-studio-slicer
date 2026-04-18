cmake_minimum_required(VERSION 3.24)
project(cereal_ios)
include(ExternalProject)
ExternalProject_Add(dep_Cereal
    URL https://github.com/USCiLab/cereal/archive/refs/tags/v1.3.2.tar.gz
    URL_HASH SHA256=16a7ad9b31ba5880dac55d62b5d6f243c3ebc8d46a3514149e56b5e7ea81f85f
    CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/../cmake/iOS.cmake
        -DCMAKE_OSX_DEPLOYMENT_TARGET=17.0
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
        -DJUST_INSTALL_CEREAL=ON
)
