/*
 * odin_slicer.h — C API facade for ODIN Studio's consumption of libslic3r.
 *
 * Design goals:
 * 1. Zero C++ exposure. Swift imports this as a clang module.
 * 2. String passing via `const char*` (UTF-8, NUL-terminated). No wide chars.
 * 3. Progress + cancel via callbacks; the implementation hops to whatever
 *    threading model libslic3r uses internally, then posts progress events.
 * 4. `odin_slicer_last_error()` returns a pointer valid until the next API
 *    call on the same handle — caller must copy if they need to keep it.
 *
 * AGPL-3.0: implementation links libslic3r; per §13 the combined binary
 * carries obligations. ODIN Studio satisfies §13 via in-app source-offer
 * pointing at github.com/HughKantsime/odin-studio-slicer.
 */

#ifndef ODIN_SLICER_H
#define ODIN_SLICER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct odin_slicer_handle odin_slicer_handle_t;

/* Error codes. Negative = error, 0 = ok, positive reserved. */
#define ODIN_SLICER_OK                  0
#define ODIN_SLICER_ERR_ARG            -1
#define ODIN_SLICER_ERR_MESH           -2
#define ODIN_SLICER_ERR_PROFILE        -3
#define ODIN_SLICER_ERR_SLICE          -4
#define ODIN_SLICER_ERR_IO             -5
#define ODIN_SLICER_ERR_CANCELED       -6
#define ODIN_SLICER_ERR_NOT_AVAILABLE  -99  /* stub until libslic3r links */

/**
 * Progress callback. `stage` is one of:
 *   "prep", "supports", "layers", "emit"
 * `progress` is 0.0…1.0 globally.
 * `user_data` is whatever was passed to `odin_slicer_slice()`.
 */
typedef void (*odin_slicer_progress_cb)(double progress,
                                        const char* stage,
                                        void* user_data);

/**
 * Create a new slicer session. `cache_dir` is a directory the implementation
 * may use for intermediate files; must exist and be writable.
 * Returns NULL on allocation failure.
 */
odin_slicer_handle_t* odin_slicer_begin(const char* cache_dir);

/**
 * Load a mesh from a path. STL + 3MF + OBJ supported by libslic3r's Model IO.
 * Returns 0 on success, negative error code on failure.
 */
int odin_slicer_load_mesh(odin_slicer_handle_t* h, const char* mesh_path);

/**
 * Load a profile — JSON string following OrcaSlicer's machine/filament/process
 * schema (see resources/profiles/BBL profile JSONs for examples).
 * The three profile layers must be merged into one flat JSON object by the
 * caller (ODIN Studio does this via its `raw` field on Profile).
 */
int odin_slicer_load_profile_json(odin_slicer_handle_t* h, const char* json);

/**
 * Run the slice. Blocks until complete, canceled, or errored.
 * Progress events fire on an internal thread — callback must be thread-safe.
 * Output G-code is written to `out_gcode_path`.
 */
int odin_slicer_slice(odin_slicer_handle_t* h,
                      const char* out_gcode_path,
                      odin_slicer_progress_cb progress_cb,
                      void* user_data);

/** Signal an in-flight slice to cancel. Returns once cancellation is observed. */
void odin_slicer_cancel(odin_slicer_handle_t* h);

/**
 * Human-readable error for the most recent failed call on this handle.
 * Returns "" when no error is recorded. Pointer valid until the next call.
 */
const char* odin_slicer_last_error(odin_slicer_handle_t* h);

/** Release all resources owned by the handle. */
void odin_slicer_end(odin_slicer_handle_t* h);

/**
 * Compile-time probe: does this build link real libslic3r, or is it a stub?
 * Returns 1 when real, 0 when stub. Swift uses this to choose the engine.
 */
int odin_slicer_is_linked(void);

#ifdef __cplusplus
}
#endif

#endif /* ODIN_SLICER_H */
