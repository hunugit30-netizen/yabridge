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
 * EditorView Proxy - wraps ARA::PlugIn::EditorView
 */
class AraEditorViewProxy {
   public:
    using Ref = uint64_t;

    struct ARAEditorViewProperties {
        std::string name;
        int32_t width = 800;
        int32_t height = 600;
        bool can_resize = false;

        template <typename S>
        void serialize(S& s) {
            s.text1b(name, 256);
            s.value4b(width);
            s.value4b(height);
            s.value1b(can_resize);
        }
    };

    struct ConstructArgs {
        ConstructArgs() noexcept = default;
        ConstructArgs(Ref ref, const std::string& id) noexcept : view_ref(ref), view_id(id) {}

        Ref view_ref = 0;
        std::string view_id;

        template <typename S>
        void serialize(S& s) {
            s.value8b(view_ref);
            s.text1b(view_id, 256);
        }
    };

    struct NotifySelectionRequest {
        using Response = ARANullResponse;

        Ref view_ref = 0;
        std::vector<ARAPlaybackRegionRef> selected_region_refs;
        std::vector<ARAPlaybackRegionRef> deselected_region_refs;
        template <typename S>
        void serialize(S& s) {
            s.value8b(view_ref);
            s.container1b(selected_region_refs, 256, [](S& s, ARAPlaybackRegionRef& ref) { s.value8b(ref); });
            s.container1b(deselected_region_refs, 256, [](S& s, ARAPlaybackRegionRef& ref) { s.value8b(ref); });
        }
    };

    struct NotifyVisibilityRequest {
        using Response = ARANullResponse;

        Ref view_ref = 0;
        std::vector<ARAPlaybackRegionRef> visible_region_refs;
        std::vector<ARAPlaybackRegionRef> hidden_region_refs;
        template <typename S>
        void serialize(S& s) {
            s.value8b(view_ref);
            s.container1b(visible_region_refs, 256, [](S& s, ARAPlaybackRegionRef& ref) { s.value8b(ref); });
            s.container1b(hidden_region_refs, 256, [](S& s, ARAPlaybackRegionRef& ref) { s.value8b(ref); });
        }
    };

    struct GetPropertiesRequest {
        using Response = EditorViewPropertiesResponse;

        Ref view_ref = 0;
        template <typename S>
        void serialize(S& s) { s.value8b(view_ref); }
    };

    struct SetPropertiesRequest {
        using Response = ARANullResponse;

        Ref view_ref = 0;
        ARAEditorViewProperties properties;
        template <typename S>
        void serialize(S& s) { s.value8b(view_ref); s.object(properties); }
    };

    struct EditorViewPropertiesResponse {
        ARAEditorViewProperties properties;
        bool success = false;
        template <typename S>
        void serialize(S& s) { s.object(properties); s.value1b(success); }
    };

    AraEditorViewProxy() = default;
    virtual ~AraEditorViewProxy() noexcept = default;

    virtual ConstructArgs create_editor_view() = 0;
    virtual void destroy_editor_view(Ref view_ref) = 0;

    virtual ARANullResponse notify_selection(const NotifySelectionRequest& request) = 0;
    virtual ARANullResponse notify_visibility(const NotifyVisibilityRequest& request) = 0;
    virtual EditorViewPropertiesResponse get_properties(const GetPropertiesRequest& request) = 0;
    virtual ARANullResponse set_properties(const SetPropertiesRequest& request) = 0;
};