#!/bin/sh
set -e
echo "set(CMAKE_MACOSX_BUNDLE OFF)" > /tmp/qhull-prepend.cmake
cat CMakeLists.txt >> /tmp/qhull-prepend.cmake
mv /tmp/qhull-prepend.cmake CMakeLists.txt
