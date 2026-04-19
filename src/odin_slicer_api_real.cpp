// odin_slicer_api_real.cpp — real implementation of odin_slicer.h
// driving Slic3r::Print + friends. Links against liblibslic3r.a.
//
// This TU replaces odin_slicer_api.cpp (the stub) when the
// `odin_slicer_real` CMake target is built. The two files are mutually
// exclusive at link time — a given static archive contains one or the
// other. `odin_slicer_is_linked()` returns 1 here, 0 in the stub.
//
// License: AGPL-3.0 (inherits from libslic3r).

#include "odin_slicer.h"

#include "libslic3r/Print.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/GCode/GCodeProcessor.hpp"

#include <nlohmann/json.hpp>
#include <boost/filesystem.hpp>

#include <atomic>
#include <mutex>
#include <string>
#include <string.h>
#include <new>

struct odin_slicer_handle {
    std::string cache_dir;
    std::string last_error;
    Slic3r::Model model;
    Slic3r::DynamicPrintConfig config;
    std::atomic<bool> cancelled{false};

    // Progress callback cached between load + slice.
    odin_slicer_progress_cb progress_cb{nullptr};
    void* user_data{nullptr};
};

static void set_error(odin_slicer_handle_t* h, const std::string& msg) {
    if (h) h->last_error = msg;
}

extern "C" {

odin_slicer_handle_t* odin_slicer_begin(const char* cache_dir) {
    auto* h = new (std::nothrow) odin_slicer_handle();
    if (!h) return nullptr;
    h->cache_dir = cache_dir ? cache_dir : "/tmp/odin-slicer";
    try {
        boost::filesystem::create_directories(h->cache_dir);
    } catch (...) {
        // Caller will see failure on first load.
    }
    h->last_error.clear();
    return h;
}

int odin_slicer_load_mesh(odin_slicer_handle_t* h, const char* mesh_path) {
    if (!h || !mesh_path) return ODIN_SLICER_ERR_ARG;
    try {
        // Slic3r::Model::read_from_file has a broad surface. Use the simple
        // overload that infers type from extension.
        h->model = Slic3r::Model::read_from_file(
            std::string(mesh_path),
            /*config=*/nullptr,
            /*config_substitutions=*/nullptr,
            Slic3r::LoadStrategy::AddDefaultInstances
        );
        if (h->model.objects.empty()) {
            set_error(h, "mesh file contained no objects");
            return ODIN_SLICER_ERR_MESH;
        }
        return ODIN_SLICER_OK;
    } catch (const std::exception& e) {
        set_error(h, std::string("load_mesh: ") + e.what());
        return ODIN_SLICER_ERR_MESH;
    } catch (...) {
        set_error(h, "load_mesh: unknown error");
        return ODIN_SLICER_ERR_MESH;
    }
}

int odin_slicer_load_profile_json(odin_slicer_handle_t* h, const char* json) {
    if (!h || !json) return ODIN_SLICER_ERR_ARG;
    try {
        auto j = nlohmann::json::parse(json);
        // OrcaSlicer's DynamicPrintConfig accepts a flat key → value dict.
        // The json we receive has nested "display"/"raw" from ODIN Studio;
        // we only feed the flat "raw" OrcaSlicer-schema subset.
        nlohmann::json flat = j.contains("raw") ? j["raw"] : j;
        Slic3r::DynamicPrintConfig& cfg = h->config;
        for (auto it = flat.begin(); it != flat.end(); ++it) {
            const std::string& key = it.key();
            const nlohmann::json& val = it.value();
            std::string str;
            if (val.is_string()) str = val.get<std::string>();
            else if (val.is_number()) str = std::to_string(val.get<double>());
            else if (val.is_boolean()) str = val.get<bool>() ? "1" : "0";
            else continue;
            try {
                cfg.set_deserialize_strict(key, str);
            } catch (...) {
                // Unknown key — skip, profile may have OrcaSlicer-specific
                // fields libslic3r doesn't recognise. Not fatal.
            }
        }
        return ODIN_SLICER_OK;
    } catch (const std::exception& e) {
        set_error(h, std::string("load_profile_json: ") + e.what());
        return ODIN_SLICER_ERR_PROFILE;
    } catch (...) {
        set_error(h, "load_profile_json: unknown error");
        return ODIN_SLICER_ERR_PROFILE;
    }
}

int odin_slicer_slice(odin_slicer_handle_t* h,
                      const char* out_gcode_path,
                      odin_slicer_progress_cb progress_cb,
                      void* user_data) {
    if (!h || !out_gcode_path) return ODIN_SLICER_ERR_ARG;
    if (h->model.objects.empty()) {
        set_error(h, "no mesh loaded");
        return ODIN_SLICER_ERR_MESH;
    }

    h->progress_cb = progress_cb;
    h->user_data = user_data;
    h->cancelled.store(false);

    try {
        if (progress_cb) progress_cb(0.02, "prep", user_data);

        Slic3r::Print print;
        // Full-config transfer into print.
        print.apply(h->model, h->config);

        if (h->cancelled.load()) return ODIN_SLICER_ERR_CANCELED;

        // Run the slicer pipeline. Internally libslic3r parallelises via TBB;
        // there's no fine-grained progress API exposed to consumers, so we
        // emit coarse milestones.
        if (progress_cb) progress_cb(0.10, "layers", user_data);
        print.process();
        if (h->cancelled.load()) return ODIN_SLICER_ERR_CANCELED;

        if (progress_cb) progress_cb(0.90, "emit", user_data);
        Slic3r::GCodeProcessorResult result;
        std::string produced = print.export_gcode(out_gcode_path, &result);
        if (produced.empty() || !boost::filesystem::exists(produced)) {
            set_error(h, "export_gcode produced no file");
            return ODIN_SLICER_ERR_IO;
        }

        if (progress_cb) progress_cb(1.0, "emit", user_data);
        return ODIN_SLICER_OK;
    } catch (const std::exception& e) {
        set_error(h, std::string("slice: ") + e.what());
        return ODIN_SLICER_ERR_SLICE;
    } catch (...) {
        set_error(h, "slice: unknown error");
        return ODIN_SLICER_ERR_SLICE;
    }
}

void odin_slicer_cancel(odin_slicer_handle_t* h) {
    if (h) h->cancelled.store(true);
}

const char* odin_slicer_last_error(odin_slicer_handle_t* h) {
    return (h && !h->last_error.empty()) ? h->last_error.c_str() : "";
}

void odin_slicer_end(odin_slicer_handle_t* h) {
    delete h;
}

int odin_slicer_is_linked(void) {
    return 1;
}

} // extern "C"
