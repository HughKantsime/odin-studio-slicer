# iOS Build Status — `odin-studio-slicer`

This fork of OrcaSlicer targets static-library output for iOS ARM64, so ODIN Studio (and any future ODIN-ecosystem iOS app) can slice on-device.

## Current state

**Stage 1 (shipped 2026-04-18):** stub C API that returns `ODIN_SLICER_ERR_NOT_AVAILABLE` for every operation. The Swift side of ODIN Studio links against this stub (via the Obj-C++ bridge in `App/Slicer/OrcaSlicerBridge.mm`) and falls back to `SyntheticSlicerEngine`. Enough to validate:
- The fork exists + LICENSE preserved (AGPL-3.0 inherits cleanly)
- The C API surface compiles against `arm64-apple-ios`
- The Swift bridge + module map link correctly
- The runtime branch (real vs synthetic) works end-to-end

**Stage 2 (TODO):** replace the stub with real libslic3r-driven slicing.

## Stage 2 — what's blocking

libslic3r's direct deps per `deps/CMakeLists.txt`:
- Boost (Filesystem, System, Thread, Log, Regex, Beast)
- TBB (Intel Threading Building Blocks)
- Eigen3 (header-only — trivial)
- nlohmann/json (header-only — trivial)
- cereal (header-only — trivial)
- CGAL + GMP + MPFR (CGAL is the worst on iOS)
- polyclipping (small, self-contained — doable)
- libigl (header-only variant — doable)
- mcut (mesh cutting — small C++, doable)
- qoi (trivial)
- miniz (trivial)

### The hard ones

**Boost.** iOS cross-compile used to require boost-for-iOS shell scripts. Modern approach: B2 with user-config.jam targeting the iOS SDK, or fetch a prebuilt XCFramework. Plan: pin to Boost 1.86 and use an XCFramework release — or build specific libs via `b2 toolset=clang-darwin target-os=iphone`.

**CGAL + GMP + MPFR.** CGAL is header-only for most of what libslic3r uses (mesh booleans via Polyhedron / Nef), but it still needs GMP + MPFR for exact arithmetic. GMP has been cross-compiled to iOS (see the [gmp-ios](https://github.com/oceanswave/gmp-ios) project and similar). Pin a prebuilt `.a`. Plan: ship GMP + MPFR as pre-built XCFramework alongside the slicer fork.

**TBB.** Has an iOS-compatible target in recent versions. Build from source with `cmake -DTBB_TEST=OFF -DCMAKE_TOOLCHAIN_FILE=…/iOS.cmake`.

### The fork() / exec() audit

libslic3r itself does not call `fork()`/`exec()` in the slicing path. The GUI (`src/slic3r/`) does, via dialog-launchers. We never link the GUI, so this is a non-issue for Stage 2 — but we add `#define SLIC3R_NO_FORK 1` in the toolchain to guard any transitively-included headers that still reference it.

## Build invocation (Stage 1 stub)

```bash
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/iOS.cmake \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=17.0 \
      -S . -B build-ios \
      -C CMakeLists.ios.txt

cmake --build build-ios --config Release
# -> build-ios/libodin_slicer_stub.a
```

## Integration with ODIN Studio

ODIN Studio's `App/Slicer/OrcaSlicerBridge.mm` includes `<odin_slicer.h>` via a module map (`App/Slicer/module.modulemap`). At first-run, `OrcaSlicerEngine` calls `odin_slicer_is_linked()` — returns 0 in stub, 1 when real — and either sets itself as the preferred engine in `SlicerService` or lets the synthetic engine win.

## AGPL-3.0 compliance

`LICENSE.txt` (the OrcaSlicer AGPL-3 text) is preserved verbatim at the fork root. ODIN Studio's `SettingsView` → "Legal" surface links here publicly. §13 is satisfied by the offer-of-source shipped in-app.
