// yabridge: a Wine plugin bridge
// Copyright (C) 2020-2026 Robbert van der Helm
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <atomic>
#include <cassert>
#include <mutex>

#include <dlfcn.h>

// Generated inside of the build directory
#include <config.h>

#include "../common/linking.h"
#include "../common/utils.h"
#include "utils.h"

// These chainloader libraries are tiny, mostly dependencyless libraries that
// `dlopen()` the actual `libyabridge-{clap,vst2,vst3,ara}.so` files and forward
// the entry point function calls from this library to those. Or technically,
// these libraries use dedicated entry point functions because multiple chainloader
// libraries may all dynamically link to the exact same plugin library, so we
// can't store any bridge information in a global there. This approach avoids
// wasting disk space on copies on file systems that don't support reflinking,
// but more importantly it also avoids the need to rerun `yabridgectl sync`
// whenever yabridge is updated. This is even more important when considering
// distro packaging, because updates to Boost might require the package to be
// rebuilt, which in turn would also require a resync.

namespace fs = ghc::filesystem;

// These functions are loaded from `libyabridge-ara.so` the first time
// `GetPluginFactory` gets called
AraPluginBridge* (*yabridge_module_init)(const char* plugin_path) = nullptr;
void (*yabridge_module_free)(AraPluginBridge* instance) = nullptr;
void* (*yabridge_module_get_factory)(AraPluginBridge* instance) = nullptr;

// This bridges the `yabridge_version()` call from the plugin library. This
// function was added later, so through weird version mixing it may be missing
// on the yabridge library.
const char* (*remote_yabridge_version)() = nullptr;

/**
 * The first time one of the exported functions from this library gets called,
 * we'll need to load the corresponding `libyabridge-*.so` file and fetch the
 * the entry point functions from that file.
 */
bool initialize_library() {
    static void* library_handle = nullptr;
    static std::mutex library_handle_mutex;

    std::lock_guard lock(library_handle_mutex);

    // There should be no situation where this library gets loaded and then two
    // threads immediately start calling functions, but we'll handle that
    // situation just in case it does happen
    if (library_handle) {
        return true;
    }

    library_handle = find_plugin_library(yabridge_ara_plugin_name);
    if (!library_handle) {
        return false;
    }

#define LOAD_FUNCTION(name)                                                 \
    do {                                                                    \
        (name) =                                                            \
            reinterpret_cast<decltype(name)>(dlsym(library_handle, #name)); \
        if (!(name)) {                                                      \
            log_failing_dlsym(yabridge_ara_plugin_name, #name);             \
            return false;                                                   \
        }                                                                   \
    } while (false)

    LOAD_FUNCTION(yabridge_module_init);
    LOAD_FUNCTION(yabridge_module_free);
    LOAD_FUNCTION(yabridge_module_get_factory);

    // This one can be a null pointer if the function does not yet exist in this
    // yabridge version
    remote_yabridge_version =
        reinterpret_cast<decltype(remote_yabridge_version)>(
            dlsym(library_handle, "yabridge_version"));

#undef LOAD_FUNCTION

    return true;
}

/**
 * Our ARA plugin's entry point. When building the plugin factory we'll host
 * the plugin in our Wine application, retrieve its information and supported
 * classes, and then recreate it here.
 */
extern "C" YABRIDGE_EXPORT void* PLUGIN_API
GetPluginFactory() {
    if (!initialize_library()) {
        return nullptr;
    }

    const fs::path this_plugin_path = get_this_file_location();

    // Initialize a new bridge instance
    AraPluginBridge* bridge = yabridge_module_init(this_plugin_path.c_str());
    if (!bridge) {
        return nullptr;
    }

    // Get the plugin factory from the bridge instance
    return yabridge_module_get_factory(bridge);
}

/**
 * This returns the actual yabridge library's version through
 * `yabridge_version()`. Reporting the version associated with this chainloader
 * wouldn't be very useful, and that would also cause the chainloader to be
 * rebuilt on every git commit in development.
 */
extern "C" YABRIDGE_EXPORT const char* yabridge_version() {
    if (!initialize_library() || !remote_yabridge_version) {
        return nullptr;
    }

    return remote_yabridge_version();
}
