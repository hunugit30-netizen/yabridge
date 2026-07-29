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

#include "ara.h"

#include <windows.h>
#include <string>
#include <iostream>

ARABridge::ARABridge(MainContext& main_context,
                     std::string plugin_dll_path,
                     std::string endpoint_base_dir,
                     pid_t parent_pid)
    : HostBridge(main_context, std::move(plugin_dll_path), parent_pid),
      sockets_(main_context_,
               ghc::filesystem::path(endpoint_base_dir),
               /*listen=*/false),
      current_instance_id_(0) {
    // Connect to the native plugin
    sockets_.connect();
    
    // TODO: Load the Windows VST3/ARA plugin here
    // This would involve:
    // 1. Loading the plugin DLL
    // 2. Getting the VST3 factory
    // 3. Querying for ARA factory through IPlugInEntryPoint
    // 4. Setting up ARA-specific sockets
}

ARABridge::~ARABridge() noexcept = default;

bool ARABridge::inhibits_event_loop() noexcept {
    // Return true if not fully initialized yet
    return !ara_factory_;
}

void ARABridge::run() {
    // Listen for incoming control messages
    sockets_.plugin_host_control_.receive_messages(
        std::nullopt,
        [this](auto&& request) {
            // Handle ARA control requests
            return handle_control_request(std::forward<decltype(request)>(request));
        });
}

void ARABridge::close_sockets() {
    sockets_.close();
}

#ifdef WITH_ARA
ARA::PlugIn::Factory* ARABridge::get_ara_factory() {
    return ara_factory_;
}
#endif

// TODO: Implement handle_control_request method
template <typename T>
typename T::Response ARABridge::handle_control_request(const T& request) {
    // This is a stub - the actual implementation would dispatch
    // to the appropriate ARA interface method
    typename T::Response response;
    return response;
}

// Explicit template instantiation for the control request types
// (These would be defined in the serialization header)