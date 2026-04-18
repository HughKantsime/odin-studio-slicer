cmake_minimum_required(VERSION 3.24)
project(boost_ios)
include(ExternalProject)
ExternalProject_Add(dep_Boost
    URL https://github.com/boostorg/boost/releases/download/boost-1.84.0/boost-1.84.0.tar.gz
    URL_HASH SHA256=4d27e9efed0f6f152dc28db6430b9d3dfb40c0345da7342eaa5a987dde57bd95
    LIST_SEPARATOR |
    CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/../cmake/iOS.cmake
        -DCMAKE_OSX_DEPLOYMENT_TARGET=17.0
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
        -DBUILD_SHARED_LIBS=OFF
        -DBUILD_TESTING=OFF
        -DBOOST_EXCLUDE_LIBRARIES:STRING=contract|fiber|numpy|stacktrace|wave|test|mysql|redis|cobalt|url
        -DBOOST_LOCALE_ENABLE_ICU:BOOL=OFF
        -DBOOST_IOSTREAMS_ENABLE_BZIP2:BOOL=OFF
        -DBOOST_IOSTREAMS_ENABLE_ZSTD:BOOL=OFF
        -DBOOST_IOSTREAMS_ENABLE_LZMA:BOOL=OFF
        -DBOOST_CONTEXT_ABI:STRING=aapcs
        -DBOOST_CONTEXT_ARCHITECTURE:STRING=arm64
)
