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

#include <iostream>
#include <filesystem>

namespace fs = ghc::filesystem;

AraPluginBridge::AraPluginBridge(const fs::path& plugin_path)
    : PluginBridge<ARASockets<std::jthread>>(
          PluginType::ara,
          plugin_path,
          [&](asio::io_context& io_context, const PluginInfo& info) {
              return ARASockets<std::jthread>(io_context,
                                              info.endpoint_base_dir_,
                                              /*listen=*/true);
          }) {
    log_init_message();
    connect_sockets_guarded();
    warn_on_version_mismatch(sockets_.host_version_.get());
    
    // TODO: Implement ARA-specific initialization
    // This would connect to the Wine host and query for ARA factory
}

AraPluginBridge::~AraPluginBridge() noexcept = default;

const void* AraPluginBridge::get_factory(const char* factory_id) {
    // TODO: Implement ARA factory query
    // For now return nullptr - the host will fall back to VST3 factory
    return nullptr;
}