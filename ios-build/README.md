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
