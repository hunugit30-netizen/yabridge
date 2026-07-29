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

#include "ara-factory-proxy.h"

#include <cstring>

AraFactoryProxy::AraFactoryProxy(void* ara_factory_raw) noexcept {
    if (!ara_factory_raw) {
        return;
    }

#ifdef WITH_ARA
    // Cast to the ARA factory type
    const ARA::PlugIn::Factory* factory = 
        static_cast<const ARA::PlugIn::Factory*>(ara_factory_raw);
    
    if (!factory) {
        return;
    }

    // Validate the factory struct size
    if (factory->structSize < sizeof(ARA::PlugIn::Factory)) {
        return;
    }

    valid_ = true;
    vtable_ptr_ = reinterpret_cast<uint64_t>(factory);
    factory_id_int_ = factory->factoryID ? 
        std::hash<std::string>{}(factory->factoryID) : 0;
    version_ = factory->highestSupportedApiGeneration;
    vendor_id_ = std::hash<std::string>{}(factory->manufacturerName ? factory->manufacturerName : "");
    
    if (factory->factoryID) {
        factory_id_ = factory->factoryID;
    }
    if (factory->manufacturerName) {
        vendor_name_ = factory->manufacturerName;
    }
    if (factory->plugInName) {
        product_name_ = factory->plugInName;
    }
    if (factory->version) {
        version_string_ = factory->version;
    }
#else
    // Without ARA SDK, just store the raw pointer
    valid_ = (ara_factory_raw != nullptr);
    vtable_ptr_ = reinterpret_cast<uint64_t>(ara_factory_raw);
    factory_id_ = "ARAFactory";
    vendor_name_ = "Unknown";
    product_name_ = "Unknown";
    version_string_ = "Unknown";
#endif
}

AraPluginFactoryProxy::ConstructArgs AraFactoryProxy::serialize_factory_info() const {
    AraPluginFactoryProxy::ConstructArgs args;
    args.supports_ara_factory = valid_;
    args.ara_factory_id = factory_id_.empty() ? "ARAFactory" : factory_id_;
    args.ara_factory_vtable = vtable_ptr_;

    if (valid_) {
        AraPluginFactoryProxy::ConstructArgs::FactoryInfo info;
        info.factory_id = factory_id_int_;
        info.version = version_;
        info.vendor_id = vendor_id_;
        info.vendor_name = vendor_name_;
        info.product_name = product_name_;
        info.version_string = version_string_;
        args.factory_info = std::move(info);
    }

    return args;
}