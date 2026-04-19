// odin_slicer_api.cpp — stub implementation of odin_slicer.h
//
// This stub returns ODIN_SLICER_ERR_NOT_AVAILABLE for every op except
// odin_slicer_is_linked() which returns 0 (i.e. "stub"). The real build
// target (odin_slicer_real, see ios-build/api_real.cmake) swaps this TU
// out with src/odin_slicer_api_real.cpp; a given libodin_slicer.a contains
// exactly one of the two, controlled at CMake-target level.
//
// License: AGPL-3.0 (inherits from OrcaSlicer / libslic3r).

#include "odin_slicer.h"
#include <stdlib.h>
#include <string.h>
#include <new>

struct odin_slicer_handle {
    // Bare-minimum state so we can round-trip last-error for the stub build.
    const char* last_error;
};

extern "C" {

odin_slicer_handle_t* odin_slicer_begin(const char* /*cache_dir*/) {
    auto* h = new (std::nothrow) odin_slicer_handle();
    if (h) {
        h->last_error = "";
    }
    return h;
}

int odin_slicer_load_mesh(odin_slicer_handle_t* h, const char* /*mesh_path*/) {
    if (!h) return ODIN_SLICER_ERR_ARG;
    h->last_error = "real libslic3r is not linked in this build";
    return ODIN_SLICER_ERR_NOT_AVAILABLE;
}

int odin_slicer_load_profile_json(odin_slicer_handle_t* h, const char* /*json*/) {
    if (!h) return ODIN_SLICER_ERR_ARG;
    h->last_error = "real libslic3r is not linked in this build";
    return ODIN_SLICER_ERR_NOT_AVAILABLE;
}

int odin_slicer_slice(odin_slicer_handle_t* h,
                      const char* /*out_gcode_path*/,
                      odin_slicer_progress_cb /*progress_cb*/,
                      void* /*user_data*/) {
    if (!h) return ODIN_SLICER_ERR_ARG;
    h->last_error = "real libslic3r is not linked in this build";
    return ODIN_SLICER_ERR_NOT_AVAILABLE;
}

void odin_slicer_cancel(odin_slicer_handle_t* /*h*/) {
    // no-op in stub
}

const char* odin_slicer_last_error(odin_slicer_handle_t* h) {
    return (h && h->last_error) ? h->last_error : "";
}

void odin_slicer_end(odin_slicer_handle_t* h) {
    delete h;
}

int odin_slicer_is_linked(void) {
    return 0;
}

int odin_slicer_add_mesh(odin_slicer_handle_t* h,
                         const char* /*mesh_path*/,
                         const float* /*transform_row_major16*/) {
    if (!h) return ODIN_SLICER_ERR_ARG;
    h->last_error = "real libslic3r is not linked in this build";
    return ODIN_SLICER_ERR_NOT_AVAILABLE;
}

int odin_slicer_get_stats(odin_slicer_handle_t* h, odin_slicer_stats_t* /*out*/) {
    if (!h) return ODIN_SLICER_ERR_ARG;
    h->last_error = "real libslic3r is not linked in this build";
    return ODIN_SLICER_ERR_NOT_AVAILABLE;
}

const char* odin_slicer_fork_commit(void) {
    return "unknown";
}

} // extern "C"
