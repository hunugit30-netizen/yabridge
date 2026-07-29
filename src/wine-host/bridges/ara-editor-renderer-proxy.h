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

#include <string>
#include <vector>
#include <optional>

#include "../../common/serialization/ara.h"

#ifdef WITH_ARA
#include "ARAInterface.h"
#include "ARAPlug.h"
#endif

/**
 * EditorRenderer Proxy - wraps ARA::PlugIn::EditorRenderer
 */
class AraEditorRendererProxy {
   public:
    using Ref = uint64_t;

    struct ConstructArgs {
        ConstructArgs() noexcept = default;
        ConstructArgs(Ref ref, const std::string& id) noexcept : renderer_ref(ref), renderer_id(id) {}

        Ref renderer_ref = 0;
        std::string renderer_id;

        template <typename S>
        void serialize(S& s) {
            s.value8b(renderer_ref);
            s.text1b(renderer_id, 256);
        }
    };

    struct GetPropertiesRequest {
        using Response = EditorRendererPropertiesResponse;

        Ref renderer_ref = 0;
        template <typename S>
        void serialize(S& s) { s.value8b(renderer_ref); }
    };

    struct SetPropertiesRequest {
        using Response = ARANullResponse;

        Ref renderer_ref = 0;
        ARAEditorRendererProperties properties;
        template <typename S>
        void serialize(S& s) { s.value8b(renderer_ref); s.object(properties); }
    };

    struct RenderRegionRequest {
        using Response = EditorRegionRenderResponse;

        Ref renderer_ref = 0;
        ARAPlaybackRegionRef region_ref = 0;
        double sample_rate = 44100.0;
        int32_t max_block_size = 512;
        double position = 0.0;
        template <typename S>
        void serialize(S& s) {
            s.value8b(renderer_ref);
            s.value8b(region_ref);
            s.value8b(sample_rate);
            s.value4b(max_block_size);
            s.value8b(position);
        }
    };

    struct SetRegionsRequest {
        using Response = ARANullResponse;

        Ref renderer_ref = 0;
        std::vector<ARAPlaybackRegionRef> region_refs;
        template <typename S>
        void serialize(S& s) {
            s.value8b(renderer_ref);
            s.container1b(region_refs, 256, [](S& s, ARAPlaybackRegionRef& ref) { s.value8b(ref); });
        }
    };

    struct EditorRendererPropertiesResponse {
        ARAEditorRendererProperties properties;
        bool success = false;
        template <typename S>
        void serialize(S& s) { s.object(properties); s.value1b(success); }
    };

    struct EditorRegionRenderResponse {
        bool success = false;
        template <typename S>
        void serialize(S& s) { s.value1b(success); }
    };

    AraEditorRendererProxy() = default;
    virtual ~AraEditorRendererProxy() noexcept = default;

    virtual ConstructArgs create_editor_renderer() = 0;
    virtual void destroy_editor_renderer(Ref renderer_ref) = 0;

    virtual EditorRendererPropertiesResponse get_properties(const GetPropertiesRequest& request) = 0;
    virtual ARANullResponse set_properties(const SetPropertiesRequest& request) = 0;
    virtual EditorRegionRenderResponse render_region(const RenderRegionRequest& request) = 0;
    virtual ARANullResponse set_regions(const SetRegionsRequest& request) = 0;
};