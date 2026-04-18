cmake_minimum_required(VERSION 3.24)
project(eigen_ios)
include(ExternalProject)
ExternalProject_Add(dep_Eigen
    URL https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz
    URL_HASH SHA256=8586084f71f9bde545ee7fa6d00288b264a2b7ac3607b974e54d13e7162c1c72
    CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/../cmake/iOS.cmake
        -DCMAKE_OSX_DEPLOYMENT_TARGET=17.0
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DEIGEN_BUILD_DOC=OFF
        -DBUILD_TESTING=OFF
        -DEIGEN_BUILD_PKGCONFIG=OFF
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
)
