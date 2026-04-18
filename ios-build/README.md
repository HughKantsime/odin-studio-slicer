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

## Upstream patches landed (this fork)

All patches gated by `if (CMAKE_SYSTEM_NAME STREQUAL "iOS")` or `#if defined(SLIC3R_IOS)`:

- **Top-level CMakeLists.txt**: made OpenSSL/CURL/Freetype/PNG/OpenGL/glfw3/OpenVDB optional on iOS; stubbed `libcurl` INTERFACE target; added `IS_CROSS_COMPILE=TRUE` when `CMAKE_SYSTEM_NAME=iOS`.
- **src/libslic3r/CMakeLists.txt**: made OpenCV/OpenCASCADE/JPEG/draco optional on iOS; stubbed `libslic3r_cgal` as INTERFACE target (Boost 1.84 + CGAL 5.6.1 iterator-`.base()` mismatch in graph/iterator.h); split `target_link_libraries(libslic3r)` into iOS/other branches dropping opencv_world/OCCT/draco/JPEG/PNG/OpenSSL/Freetype.
- **src/libslic3r/Utils.hpp**, **src/libslic3r/Format/bbs_3mf.cpp**: swapped `#include <openssl/md5.h>` → `CommonCrypto` with `COMMON_DIGEST_FOR_OPENSSL` shim (API-compat for MD5_CTX/Init/Update/Final).
- **src/libslic3r/Format/STEP.hpp**: iOS-stub body with forward-decl `class Step;` + typedefs so callers in Model.hpp compile.
- **src/libslic3r/Format/{STEP,svg,DRC}.cpp**: entire TU wrapped in `#if !defined(SLIC3R_IOS)`.
- **src/libslic3r/EdgeGrid.cpp**: gated `#include <png.h>` (header never actually used in body).
- **src/libslic3r/GCode/Thumbnails.cpp**: `compress_thumbnail_jpg()` returns `nullptr` on iOS; jpeglib headers gated.

## Configure + build status

Configure against `/tmp/odin-slicer-ios-deps` prefix: **GREEN**.

Build progresses to ~34% before hitting the remaining wall in libslic3r core:

- **Model.cpp** uses `Slic3r::Step` as value-type in several places (not just references) — needs either a non-forward-declared stub Step class with the right shape, OR all Model::load_step paths ifdef'd. Roughly 5–10 call sites.
- **ObjColorUtils.hpp** includes `<opencv2/opencv.hpp>` — uses `cv::Mat` inline. Either cross-compile OpenCV core (single-session undertaking) or guard the two fn signatures that use it.

Neither is conceptual; both are mechanical editing.

## Resume command

```bash
# Deps must already be staged at /tmp/odin-slicer-ios-deps (see sections above).
cd <fork>
cmake -S . -B build-libslic3r-ios \
    -DCMAKE_TOOLCHAIN_FILE=cmake/iOS.cmake \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=17.0 \
    -DCMAKE_FIND_ROOT_PATH=/tmp/odin-slicer-ios-deps \
    -DCMAKE_PREFIX_PATH=/tmp/odin-slicer-ios-deps \
    -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DCMAKE_MACOSX_BUNDLE=OFF \
    -DSLIC3R_GUI=OFF -DSLIC3R_BUILD_TESTS=OFF -DSLIC3R_STATIC=ON \
    -DSLIC3R_ENC_CHECK=OFF
cmake --build build-libslic3r-ios --target libslic3r -- -j8
```



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
