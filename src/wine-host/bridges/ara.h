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

#pragma once

#include <map>
#include <shared_mutex>
#include <string>

#include <clap/entry.h>
#include <clap/factory/plugin-factory.h>
#include <clap/plugin.h>

#include "../../common/audio-shm.h"
#include "../../common/communication/ara.h"
#include "../../common/configuration.h"
#include "../../common/mutual-recursion.h"
#include "../editor.h"
#include "common.h"

// ARA SDK headers - when available
#ifdef WITH_ARA
#include "ARAInterface.h"
#include "ARAPlug.h"
#endif

/**
 * This hosts the ARA functionality of a Windows VST3 plugin. ARA plugins are
 * VST3 plugins that implement additional ARA interfaces. This bridge forwards
 * ARA-specific messages between the native plugin and the Wine plugin host.
 */
class ARABridge : public HostBridge {
   public:
    /**
     * Initializes the Windows ARA plugin and sets up communication with the
     * native Linux plugin.
     *
     * @param main_context The main IO context for this application. Most events
     *   will be dispatched to this context, and the event handling loop should
     *   also be run from this context.
     * @param plugin_dll_path A (Unix style) path to the Windows plugin to
     *   load. For VST3/ARA plugins, this is typically a directory path.
     * @param endpoint_base_dir The base directory used for the socket
     *   endpoints. See `Sockets` for more information.
     * @param parent_pid The process ID of the native plugin host this bridge is
     *   supposed to communicate with. Used as part of our watchdog to prevent
     *   dangling Wine processes.
     *
     * @note The object has to be constructed from the same thread that calls
     *   `main_context.run()`.
     *
     * @throw std::runtime_error Thrown when the ARA plugin could not be loaded,
     *   or if communication could not be set up.
     */
    ARABridge(MainContext& main_context,
              std::string plugin_dll_path,
              std::string endpoint_base_dir,
              pid_t parent_pid);

    /**
     * This returns `true` if the ARA plugin has not yet been fully initialized.
     * We inhibit the event loop during early initialization to prevent race
     * conditions.
     */
    bool inhibits_event_loop() noexcept override;

    /**
     * Listen for and handle incoming control messages until the sockets get
     * closed.
     */
    void run() override;

   protected:
    void close_sockets() override;

   public:
    /**
     * Send a callback message to the host and return the response.
     */
    template <typename T>
    typename T::Response send_main_thread_message(const T& object) {
        return sockets_.plugin_host_callback_.send_message(object, std::nullopt);
    }

    /**
     * Get the ARA factory from the plugin, if it implements ARA.
     */
#ifdef WITH_ARA
    ARA::PlugIn::Factory* get_ara_factory();
#else
    void* get_ara_factory() { return nullptr; }
#endif

   private:
    /**
     * The configuration for this instance of yabridge based on the path to the
     * `.so` (or `.clap`) file that got loaded by the host.
     */
    Configuration config_;

    /**
     * A logger instance for ARA-specific messages.
     */
    ARALogger logger_;

    /**
     * All sockets used for communicating with this specific plugin.
     */
    ARASockets<Win32Thread> sockets_;

    /**
     * Used to assign a unique identifier to created ARA objects so they can be
     * referred to later.
     */
    std::atomic_size_t current_instance_id_;

    /**
     * The shared library handle of the VST3 plugin (which may contain ARA).
     */
    std::unique_ptr<std::remove_pointer_t<HMODULE>, decltype(&FreeLibrary)>
        plugin_handle_;

    /**
     * The Windows VST3 plugin's entry point (which may contain ARA factory).
     */
    std::unique_ptr<Steinberg::IPluginFactory, void (*)(Steinberg::IPluginFactory*)>
        plugin_factory_;

    /**
     * ARA factory from the plugin, if available.
     */
#ifdef WITH_ARA
    ARA::PlugIn::Factory* ara_factory_ = nullptr;
#endif

    /**
     * Used in `send_mutually_recursive_main_thread_message()` to be able to
     * execute functions from that same calling thread while we're waiting for a
     * response.
     */
    MutualRecursionHelper<Win32Thread> mutual_recursion_;
};