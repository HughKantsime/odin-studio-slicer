# iOS Cross-Compile Drivers

Each `*.cmake` file here is a standalone CMake project that fetches, configures, and builds one dep of libslic3r for `arm64-apple-ios`.

## Usage

```bash
# Example: build TBB
mkdir -p /tmp/tbb-ios-build && cd /tmp/tbb-ios-build
cmake -DCMAKE_BUILD_PARENT_DIR=<slicer-repo-root> \
      -S <slicer-repo-root>/ios-build \
      -C <slicer-repo-root>/ios-build/tbb.cmake
```

In practice the easier path is to copy the driver into a scratch dir as `CMakeLists.txt`, fix relative paths if needed, then:

```bash
cmake -S . -B build
cmake --build build
```

## Verified (2026-04-18)

| Dep | Driver | Output | Arch check |
|-----|--------|--------|------------|
| TBB 2021.5 | tbb.cmake | `libtbb.12.5.dylib`, `libtbbmalloc.2.5.dylib` | Mach-O arm64, `LC_BUILD_VERSION platform 2` (iOS) ✓ |
| Boost 1.84 | boost.cmake | 22 `.a` files (system, filesystem, thread, log, locale, regex, chrono, atomic, date_time, iostreams, program_options, nowide, context, coroutine, …) | Mach-O arm64, platform 2 ✓ |
| Eigen 3.4.0 | eigen.cmake | Headers-only install under `include/eigen3/Eigen/` | N/A |
| NLopt 2.5.0 | nlopt.cmake | `libnlopt.a` | Mach-O arm64, platform 2 ✓ |
| Qhull 8.0.2 | qhull.cmake + prepend_no_bundle.sh | `libqhullstatic{,_r}.a`, `libqhullcpp.a` | Mach-O arm64 (patched CMAKE_MACOSX_BUNDLE=OFF) ✓ |
| GMP 6.3.0 | gmp.sh (autoconf, `--host=aarch64-apple-darwin`, `--disable-assembly`) | `libgmp.a` (1.2 MB) | Mach-O arm64, platform 2 ✓ |
| MPFR 4.2.1 | mpfr.sh (autoconf, `--with-gmp=$GMP_PREFIX`) | `libmpfr.a` (1.1 MB) | Mach-O arm64, platform 2 ✓ |
| CGAL 5.6.1 | cgal.cmake (header-only install, GMP+MPFR pointed in) | Headers + CMake config at `lib/cmake/CGAL` | N/A (header-only) |

## Staged combined prefix

`/tmp/odin-slicer-ios-deps/` — all 8 cross-compiled deps merged into one prefix (237 MB, 28 static libs + 9 TBB dylibs + headers for Boost/Eigen/CGAL/NLopt/Qhull/GMP/MPFR). Usable directly via `-DCMAKE_PREFIX_PATH=/tmp/odin-slicer-ios-deps -DCMAKE_FIND_ROOT_PATH=/tmp/odin-slicer-ios-deps -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH`.

## The real wall (Stage 2b start)

libslic3r/CMakeLists.txt hard-requires two more deps that OrcaSlicer treats as non-optional, used only by GUI-adjacent paths:

- **OpenCV** — `find_package(OpenCV REQUIRED core)` at libslic3r/CMakeLists.txt:499. Used for camera calibration + image inspection. Cross-compiling OpenCV (3.4 or 4.x) for iOS ARM64 is well-documented but non-trivial (~1–2 sessions).
- **OpenCASCADE** — STEP file import, required. Massive library. Potentially stubbable if we gate STEP import on a compile-flag.

These two need either:
1. Patching `src/libslic3r/CMakeLists.txt` to make them `optional` + guarding their callers with `#ifdef SLIC3R_IOS_NO_OPENCV`/etc. — surgical but touches upstream code.
2. Cross-compile them too — ~2 more sessions.

Option 1 is the pragmatic path for a first iOS slice-able binary. It trades off texture inspection + STEP import, both of which ODIN Studio can live without for v1.

## Configure-pass result (verification)

`cmake -S . -B build-libslic3r-ios -DCMAKE_TOOLCHAIN_FILE=cmake/iOS.cmake -DCMAKE_PREFIX_PATH=/tmp/odin-slicer-ios-deps -DCMAKE_FIND_ROOT_PATH=/tmp/odin-slicer-ios-deps -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH -DSLIC3R_GUI=OFF -DSLIC3R_BUILD_TESTS=OFF -DSLIC3R_STATIC=ON`

resolves through:
- Boost ✓ (found 1.84 via BoostConfig.cmake)
- TBB ✓
- Threads ✓
- Iconv ✓ (SDK)
- ZLIB ✓ (SDK)
- Fails at: OpenSSL at top-level (used by CURL in desktop networking — needs iOS guard in top-level CMakeLists.txt).

The top-level CMakeLists's `find_package(OpenSSL REQUIRED)` at line 621 also needs the same `optional` treatment. All three (OpenSSL/CURL/Freetype) are used exclusively by the GUI + preset updater, never by slicer core, so patches guarding them behind `if(NOT SLIC3R_IOS)` are clean.


## Remaining deps (priority order)

### Header-only (trivial, no build)
- Eigen3 3.4.0
- Cereal 1.3.2
- nlohmann/json 3.11.2
- tl::optional

### Small CMake-native (should be quick)
- Expat 2.5.0 (ZLIB too)
- Clipper2 1.4+
- Qhull 8.0.x
- NLopt 2.7+
- Blosc 1.x

### The hard ones
- GMP 6.3.0 (autoconf, cross-compile needs manual config)
- MPFR 4.2.1 (depends on GMP)
- CGAL 5.x (header-only for polyhedron ops but test harness tries to link GMP/MPFR)

### SLA-only (skippable for FDM-first MVP)
- OpenVDB 11.x
- OpenEXR 3.x

### Desktop-only (already excluded from iOS build)
- wxWidgets, GLEW, GLFW, OpenCSG, OCCT, CURL, OpenSSL, DBus, Freetype

## Why not FetchContent all in one drive?

A unified `deps/CMakeLists.ios.txt` is the goal. Getting there in one pass is fragile — each dep surfaces different cross-compile quirks. The per-dep driver approach lets us verify each piece works (and commit it) before rolling it up. `ios-build/all.cmake` (future) will be the one-command entry point once every individual dep is proven.
