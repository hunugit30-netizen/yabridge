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

#include "ara-impls/plugin-factory-proxy.h"

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
    
    // Initialize the ARA plugin factory proxy by querying the Wine host
    // The Wine host will load the Windows VST3/ARA plugin and query its ARA factory
    AraPluginFactoryProxy::ConstructArgs factory_args =
        sockets_.plugin_host_control_.send_message(
            AraPluginFactoryProxy::Construct{},
            std::pair<ARALogger&, bool>(logger_, true));
    
    plugin_factory_ = std::make_unique<AraPluginFactoryProxyImpl>(*this, std::move(factory_args));
}

AraPluginBridge::~AraPluginBridge() noexcept = default;

const void* AraPluginBridge::get_factory(const char* factory_id) {
    // Delegate to the plugin factory proxy
    if (plugin_factory_) {
        return plugin_factory_->get_factory(factory_id);
    }
    
    return nullptr;
}

