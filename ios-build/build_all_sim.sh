#!/bin/sh
# build_all_sim.sh — cross-compile every libslic3r dep for iOS simulator
# (arm64-apple-ios-simulator). Companion to build_all_device.sh (implicit:
# the per-dep drivers default to device toolchain). Deps are staged into
# /tmp/odin-slicer-ios-sim-deps/ mirroring the device prefix layout.
#
# Usage:
#   ./ios-build/build_all_sim.sh
#
# Idempotent — skips already-built artifacts via CMake's stamp files.
set -eu

SLICER_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TOOLCHAIN="$SLICER_ROOT/cmake/iOS-simulator.cmake"
PREFIX=/tmp/odin-slicer-ios-sim-deps
mkdir -p "$PREFIX"

export PATH="/Users/ollama/homebrew/bin:$PATH"

run_cmake_driver() {
    DRIVER="$1"
    NAME="$2"
    WORK="/tmp/build-${NAME}-ios-sim"
    rm -rf "$WORK"
    mkdir -p "$WORK"
    cp "$SLICER_ROOT/ios-build/${DRIVER}" "$WORK/CMakeLists.txt"
    # Qhull driver shells out to prepend_no_bundle.sh; copy it alongside.
    if [ -f "$SLICER_ROOT/ios-build/prepend_no_bundle.sh" ]; then
        cp "$SLICER_ROOT/ios-build/prepend_no_bundle.sh" "$WORK/"
        chmod +x "$WORK/prepend_no_bundle.sh"
    fi
    # Rewrite the driver's internal toolchain-path reference to the simulator
    # toolchain. Drivers use ExternalProject_Add, which doesn't inherit our
    # outer toolchain — the path is baked into the nested CMAKE_ARGS.
    sed -i.bak "s|\${CMAKE_SOURCE_DIR}/../cmake/iOS.cmake|${TOOLCHAIN}|" "$WORK/CMakeLists.txt"
    (
        cd "$WORK" && \
        cmake -S . -B build -GNinja \
            -DCMAKE_INSTALL_PREFIX="$PREFIX" \
            -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
            -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" && \
        cmake --build build -- -j4
    )
    # Stage this dep's ExternalProject install tree into the combined prefix.
    # ExternalProject defaults to <build>/<target>-prefix/{lib,include,...}.
    for pre in "$WORK"/build/dep_*-prefix; do
        # `[ -d X ] && rsync ...` dies under `set -e` when the test is false.
        # Use explicit if/then so a missing subdir is a no-op, not a failure.
        if [ -d "$pre/lib" ];     then rsync -a "$pre/lib/"     "$PREFIX/lib/";     fi
        if [ -d "$pre/include" ]; then rsync -a "$pre/include/" "$PREFIX/include/"; fi
        if [ -d "$pre/share" ];   then rsync -a "$pre/share/"   "$PREFIX/share/";   fi
    done
}

echo "[1/8] TBB (simulator, static)"
run_cmake_driver tbb.cmake tbb

echo "[2/8] Boost 1.84 (simulator)"
run_cmake_driver boost.cmake boost

echo "[3/8] Eigen 3.4 (headers)"
run_cmake_driver eigen.cmake eigen

echo "[4/8] NLopt (simulator)"
run_cmake_driver nlopt.cmake nlopt

echo "[5/8] Qhull 8.0.2 (simulator)"
run_cmake_driver qhull.cmake qhull

echo "[6/8] GMP 6.3.0 (simulator, autoconf)"
sh "$SLICER_ROOT/ios-build/gmp-sim.sh" /tmp/gmp-ios-sim-build "$PREFIX"

echo "[7/8] MPFR 4.2.1 (simulator, autoconf)"
sh "$SLICER_ROOT/ios-build/mpfr-sim.sh" /tmp/mpfr-ios-sim-build "$PREFIX" "$PREFIX"

echo "[8/8] CGAL 5.6.1 (headers, sim)"
run_cmake_driver cgal.cmake cgal

echo "[extra] Cereal (headers)"
run_cmake_driver cereal.cmake cereal

echo "[extra] libnoise (simulator)"
run_cmake_driver libnoise.cmake libnoise

echo "All simulator deps installed at $PREFIX"
