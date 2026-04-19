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

// libslic3r TUs reference nsvgParse/nsvgDelete but never define the
// nanosvg implementation — the desktop build gets it from the GUI layer.
// Since iOS doesn't build the GUI, provide the impl here in exactly one TU.
#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvg/nanosvg.h"

#include "libslic3r/Print.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/GCode/GCodeProcessor.hpp"

#include <nlohmann/json.hpp>
#include <boost/filesystem.hpp>

#include <atomic>
#include <condition_variable>
#include <memory>
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

    // Populated by odin_slicer_slice on success; consumed by get_stats.
    bool stats_valid{false};
    odin_slicer_stats_t stats{};

    // Active Print, set by odin_slicer_slice for the duration of process() +
    // export_gcode(). Guarded by `print_mu`; `odin_slicer_cancel` can reach
    // it to signal libslic3r's cancel_status. `slice_done_cv` wakes cancel
    // once the slice loop has fully unwound.
    std::mutex print_mu;
    Slic3r::Print* active_print{nullptr};
    std::condition_variable slice_done_cv;
    std::atomic<bool> slice_in_flight{false};
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

int odin_slicer_add_mesh(odin_slicer_handle_t* h,
                         const char* mesh_path,
                         const float* transform_row_major16) {
    if (!h || !mesh_path) return ODIN_SLICER_ERR_ARG;
    if (h->model.objects.size() >= 16) {
        set_error(h, "add_mesh: instance cap (16) reached");
        return ODIN_SLICER_ERR_ARG;
    }
    try {
        Slic3r::Model loaded = Slic3r::Model::read_from_file(
            std::string(mesh_path),
            /*config=*/nullptr,
            /*config_substitutions=*/nullptr,
            Slic3r::LoadStrategy::AddDefaultInstances
        );
        if (loaded.objects.empty()) {
            set_error(h, "add_mesh: mesh file contained no objects");
            return ODIN_SLICER_ERR_MESH;
        }

        // Pre-compute the transform once; apply to every imported instance.
        Slic3r::Transform3d xform = Slic3r::Transform3d::Identity();
        if (transform_row_major16) {
            Eigen::Matrix4d m;
            for (int r = 0; r < 4; ++r)
                for (int c = 0; c < 4; ++c)
                    m(r, c) = static_cast<double>(transform_row_major16[r * 4 + c]);
            xform.matrix() = m;
        }

        for (Slic3r::ModelObject* src : loaded.objects) {
            // add_object copies volumes/instances deep enough to survive the
            // source Model going out of scope. Then stamp the transform onto
            // every instance (usually one).
            Slic3r::ModelObject* dst = h->model.add_object(*src);
            Slic3r::Geometry::Transformation geo;
            geo.set_matrix(xform);
            for (Slic3r::ModelInstance* inst : dst->instances) {
                inst->set_transformation(geo);
            }
        }
        return ODIN_SLICER_OK;
    } catch (const std::exception& e) {
        set_error(h, std::string("add_mesh: ") + e.what());
        return ODIN_SLICER_ERR_MESH;
    } catch (...) {
        set_error(h, "add_mesh: unknown error");
        return ODIN_SLICER_ERR_MESH;
    }
}

static std::string jval_to_string(const nlohmann::json& val) {
    if (val.is_string()) return val.get<std::string>();
    if (val.is_boolean()) return val.get<bool>() ? "1" : "0";
    if (val.is_number_integer()) return std::to_string(val.get<long long>());
    if (val.is_number_float()) return std::to_string(val.get<double>());
    if (val.is_number()) return std::to_string(val.get<double>());
    return {};
}

// OrcaSlicer's profile schema wraps every scalar in a 1-element array so a
// single serializer works across multi-extruder configs. Flatten arrays into
// libslic3r's comma-delimited convention (ConfigOptionFloats/Ints/Strings).
static std::string jarray_to_string(const nlohmann::json& arr) {
    std::string out;
    bool first = true;
    for (const auto& v : arr) {
        std::string s = jval_to_string(v);
        if (s.empty() && !v.is_string()) continue;
        if (!first) out.push_back(',');
        out += s;
        first = false;
    }
    return out;
}

int odin_slicer_load_profile_json(odin_slicer_handle_t* h, const char* json) {
    if (!h || !json) return ODIN_SLICER_ERR_ARG;
    try {
        auto j = nlohmann::json::parse(json);
        // Accept the merged flat dict from the caller (process + machine +
        // filament already unified). We also support a `raw` sub-object and a
        // compat-shape `{process, machine, filament}` for callers that pre-
        // stage the three profile layers without merging.
        nlohmann::json merged = nlohmann::json::object();
        auto consume = [&](const nlohmann::json& src) {
            if (!src.is_object()) return;
            for (auto it = src.begin(); it != src.end(); ++it) {
                merged[it.key()] = it.value();
            }
        };
        if (j.contains("raw") && j["raw"].is_object()) {
            consume(j["raw"]);
        }
        // Priority: filament → machine → process (process wins on conflicts,
        // because it carries the user's active slicing intent).
        if (j.contains("filament")) consume(j["filament"]);
        if (j.contains("machine"))  consume(j["machine"]);
        if (j.contains("process"))  consume(j["process"]);
        if (merged.empty()) consume(j); // flat already

        Slic3r::DynamicPrintConfig& cfg = h->config;
        size_t applied = 0;
        size_t skipped = 0;
        for (auto it = merged.begin(); it != merged.end(); ++it) {
            const std::string& key = it.key();
            const nlohmann::json& val = it.value();
            // OrcaSlicer meta-fields; libslic3r doesn't know about them.
            if (key == "type" || key == "name" || key == "inherits" ||
                key == "from" || key == "filament_id" || key == "setting_id" ||
                key == "instantiation" || key == "version" || key == "is_custom_defined") {
                continue;
            }
            std::string str;
            if (val.is_array()) {
                str = jarray_to_string(val);
            } else {
                str = jval_to_string(val);
            }
            if (str.empty() && !val.is_string()) continue;
            try {
                cfg.set_deserialize_strict(key, str);
                ++applied;
            } catch (...) {
                // Unknown key / bad shape — skip. Profile may carry
                // OrcaSlicer-specific fields libslic3r doesn't recognise.
                ++skipped;
            }
        }
        if (applied == 0) {
            set_error(h, "load_profile_json: no keys applied (" + std::to_string(skipped) + " skipped)");
            return ODIN_SLICER_ERR_PROFILE;
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
    h->slice_in_flight.store(true);
    struct SliceGuard {
        odin_slicer_handle_t* h;
        ~SliceGuard() {
            {
                std::lock_guard<std::mutex> g(h->print_mu);
                h->active_print = nullptr;
            }
            h->slice_in_flight.store(false);
            h->slice_done_cv.notify_all();
        }
    } guard{h};

    try {
        if (progress_cb) progress_cb(0.02, "prep", user_data);

        Slic3r::Print print;
        // Wire libslic3r's own cancel channel so workers unwind cleanly via
        // throw_if_canceled() instead of relying on our post-process atomic
        // check. Must be set before apply() so PrintObject sees it.
        print.set_cancel_callback([h]{
            if (h->cancelled.load()) {
                // Best effort — libslic3r checks cancel_status atomically.
            }
        });
        {
            std::lock_guard<std::mutex> g(h->print_mu);
            h->active_print = &print;
            if (h->cancelled.load()) {
                print.cancel();
            }
        }
        // Full-config transfer into print.
        print.apply(h->model, h->config);

        if (h->cancelled.load()) { print.cancel(); return ODIN_SLICER_ERR_CANCELED; }

        // Run the slicer pipeline. Internally libslic3r parallelises via TBB;
        // PrintObject steps check Print::throw_if_canceled() and unwind via
        // CanceledException, which we catch below.
        if (progress_cb) progress_cb(0.10, "layers", user_data);
        print.process();
        if (h->cancelled.load() || print.canceled()) return ODIN_SLICER_ERR_CANCELED;

        if (progress_cb) progress_cb(0.90, "emit", user_data);
        Slic3r::GCodeProcessorResult result;
        std::string produced = print.export_gcode(out_gcode_path, &result);
        if (produced.empty() || !boost::filesystem::exists(produced)) {
            set_error(h, "export_gcode produced no file");
            return ODIN_SLICER_ERR_IO;
        }

        // Stats distillation from GCodeProcessorResult. Normal mode (idx 0),
        // Stealth mode (idx 1) is ignored for the stats surface.
        h->stats = odin_slicer_stats_t{};
        h->stats.total_time_sec = static_cast<double>(
            result.print_statistics.modes[0].time);
        double total_volume_mm3 = 0.0;
        for (const auto& kv : result.print_statistics.total_volumes_per_extruder) {
            total_volume_mm3 += kv.second;
        }
        h->stats.filament_volume_mm3 = total_volume_mm3;
        // Length = volume / cross-section area. Area = pi * (d/2)^2.
        // Use first extruder's diameter if available (multi-material averages
        // are out of scope for v1 stats).
        double filament_length_mm = 0.0;
        double filament_weight_g = 0.0;
        if (!result.filament_diameters.empty()) {
            for (const auto& kv : result.print_statistics.total_volumes_per_extruder) {
                size_t ext = kv.first;
                double d = (ext < result.filament_diameters.size())
                    ? static_cast<double>(result.filament_diameters[ext])
                    : static_cast<double>(result.filament_diameters[0]);
                double area_mm2 = 3.14159265358979323846 * (d * 0.5) * (d * 0.5);
                if (area_mm2 > 0.0) filament_length_mm += kv.second / area_mm2;
                if (ext < result.filament_densities.size()) {
                    // density is g/cm^3; volume is mm^3 → divide by 1000 for cm^3.
                    filament_weight_g += (kv.second / 1000.0)
                        * static_cast<double>(result.filament_densities[ext]);
                }
            }
        }
        h->stats.filament_length_mm = filament_length_mm;
        h->stats.filament_weight_g = filament_weight_g;
        // Layer count: max layer_id + 1, skipping the sentinel 0 on the first
        // move vertex.
        uint32_t max_layer = 0;
        for (const auto& mv : result.moves) {
            if (mv.layer_id > max_layer) max_layer = mv.layer_id;
        }
        h->stats.layer_count = max_layer + (result.moves.empty() ? 0u : 1u);
        h->stats.move_count = static_cast<uint32_t>(result.moves.size());
        h->stats_valid = true;

        if (progress_cb) progress_cb(1.0, "emit", user_data);
        return ODIN_SLICER_OK;
    } catch (const Slic3r::CanceledException&) {
        return ODIN_SLICER_ERR_CANCELED;
    } catch (const std::exception& e) {
        set_error(h, std::string("slice: ") + e.what());
        return ODIN_SLICER_ERR_SLICE;
    } catch (...) {
        set_error(h, "slice: unknown error");
        return ODIN_SLICER_ERR_SLICE;
    }
}

void odin_slicer_cancel(odin_slicer_handle_t* h) {
    if (!h) return;
    h->cancelled.store(true);
    // Hand the cancel to libslic3r's own channel if a slice is in flight —
    // worker threads observe this via Print::throw_if_canceled() and unwind
    // with a CanceledException instead of running to completion.
    {
        std::lock_guard<std::mutex> g(h->print_mu);
        if (h->active_print) h->active_print->cancel();
    }
    // Block until the slice loop exits, per the header contract. If no slice
    // is running this returns immediately.
    std::unique_lock<std::mutex> lk(h->print_mu);
    h->slice_done_cv.wait(lk, [h]{ return !h->slice_in_flight.load(); });
}

int odin_slicer_get_stats(odin_slicer_handle_t* h, odin_slicer_stats_t* out) {
    if (!h || !out) return ODIN_SLICER_ERR_ARG;
    if (!h->stats_valid) {
        set_error(h, "no stats available — slice() has not succeeded");
        return ODIN_SLICER_ERR_ARG;
    }
    *out = h->stats;
    return ODIN_SLICER_OK;
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

#ifndef ODIN_SLICER_FORK_COMMIT
#define ODIN_SLICER_FORK_COMMIT "unknown"
#endif

const char* odin_slicer_fork_commit(void) {
    return ODIN_SLICER_FORK_COMMIT;
}

} // extern "C"
