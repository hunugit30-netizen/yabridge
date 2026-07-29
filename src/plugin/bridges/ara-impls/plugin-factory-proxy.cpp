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

#include "plugin-factory-proxy.h"

#include <cstring>

#include "../../../common/serialization/ara.h"
#include "../ara.h"

AraPluginFactoryProxyImpl::AraPluginFactoryProxyImpl(
    AraPluginBridge& bridge,
    AraPluginFactoryProxy::ConstructArgs&& args) noexcept
    : AraPluginFactoryProxy(std::move(args)), bridge_(bridge) {}

const void* AraPluginFactoryProxyImpl::get_factory(const char* factory_id) {
    // Check if the host is requesting the ARA factory
    // ARA factory ID is typically "ARAFactory" or a specific CLSID
    if (factory_id && arguments_.supports_ara_factory && 
        (std::strcmp(factory_id, "ARAFactory") == 0 || 
         std::strcmp(factory_id, arguments_.ara_factory_id.c_str()) == 0)) {
        // Return the ARA factory from the Wine host
        // This will be a pointer to the ARA::PlugIn::Factory interface
        return reinterpret_cast<const void*>(arguments_.ara_factory_vtable);
    }
    
    return nullptr;
}

