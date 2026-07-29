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

#include <optional>
#include <string>

#include "../../common/serialization/ara.h"

#ifdef WITH_ARA
#include "ARAInterface.h"
#include "ARAPlug.h"
#endif

/**
 * ARA Factory Proxy - wraps ARA::PlugIn::Factory on the Wine host side
 * and serializes its information for the native plugin side.
 */
class AraFactoryProxy {
   public:
    explicit AraFactoryProxy(void* ara_factory_raw) noexcept;
    ~AraFactoryProxy() noexcept = default;

    /**
     * Serialize the factory information for the native plugin side.
     */
    AraPluginFactoryProxy::ConstructArgs serialize_factory_info() const;

    /**
     * Check if the factory is valid.
     */
    bool is_valid() const { return valid_; }

   private:
    bool valid_ = false;
    
    // Factory information (from ARAFactory)
    std::string factory_id_;
    std::string vendor_name_;
    std::string product_name_;
    std::string version_string_;
    uint64_t factory_id_int_ = 0;
    uint64_t version_ = 0;
    uint64_t vendor_id_ = 0;
    uint64_t vtable_ptr_ = 0;
};