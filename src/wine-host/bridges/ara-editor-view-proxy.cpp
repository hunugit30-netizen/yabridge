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

#include "ara-editor-view-proxy.h"

#include <cstring>

#ifdef WITH_ARA
#include "ARAInterface.h"
#include "ARAPlug.h"
#endif

// Helper to convert our serialized types to ARA types
#ifdef WITH_ARA
static void copy_editor_view_properties(const AraEditorViewProxy::ARAEditorViewProperties& src, ARA::PlugIn::EditorViewProperties& dst) {
    dst.name = src.name.c_str();
    dst.width = src.width;
    dst.height = src.height;
    dst.canResize = src.can_resize ? kARATrue : kARAFalse;
}
#endif

AraEditorViewProxyImpl::AraEditorViewProxyImpl(ARABridge& bridge, Ref ref)
    : bridge_(bridge), ref_(ref) {
#ifdef WITH_ARA
    // The editor view would be created through the document controller
    // For now, we just store the reference
#endif
}

AraEditorViewProxyImpl::~AraEditorViewProxyImpl() noexcept = default;

// EditorView proxy methods
AraEditorViewProxy::ConstructArgs AraEditorViewProxyImpl::create_editor_view() {
    bridge_.logger_.log_info([&](auto& log) {
        log << "ARA: CreateEditorView for ref " << ref_;
    });
    return ConstructArgs{ref_, "EditorView"};
}

void AraEditorViewProxyImpl::destroy_editor_view(Ref view_ref) {
    bridge_.logger_.log_info([&](auto& log) {
        log << "ARA: DestroyEditorView for ref " << view_ref;
    });
}

AraEditorViewProxy::EditorViewPropertiesResponse AraEditorViewProxyImpl::get_properties(
    const AraEditorViewProxy::GetPropertiesRequest& request) {
    EditorViewPropertiesResponse response;
#ifdef WITH_ARA
    if (view_) {
        ARA::PlugIn::EditorViewProperties props;
        view_->getProperties(view_, &props);
        AraEditorViewProxy::ARAEditorViewProperties our_props;
        our_props.name = props.name ? props.name : "";
        our_props.width = props.width;
        our_props.height = props.height;
        our_props.can_resize = props.canResize == kARATrue;
        response.properties = our_props;
        response.success = true;
    }
#endif
    return response;
}

ARANullResponse AraEditorViewProxyImpl::set_properties(
    const AraEditorViewProxy::SetPropertiesRequest& request) {
#ifdef WITH_ARA
    if (view_) {
        ARA::PlugIn::EditorViewProperties props;
        copy_editor_view_properties(request.properties, props);
        view_->setProperties(view_, &props);
    }
#endif
    return ARANullResponse{};
}

ARANullResponse AraEditorViewProxyImpl::notify_selection(
    const AraEditorViewProxy::NotifySelectionRequest& request) {
#ifdef WITH_ARA
    if (view_) {
        // Convert selection refs to ARA refs
        std::vector<ARA::PlugIn::PlaybackRegionRef> selected_refs;
        std::vector<ARA::PlugIn::PlaybackRegionRef> deselected_refs;
        
        selected_refs.reserve(request.selected_region_refs.size());
        for (auto ref : request.selected_region_refs) {
            selected_refs.push_back(reinterpret_cast<ARA::PlugIn::PlaybackRegionRef>(ref));
        }
        
        deselected_refs.reserve(request.deselected_region_refs.size());
        for (auto ref : request.deselected_region_refs) {
            deselected_refs.push_back(reinterpret_cast<ARA::PlugIn::PlaybackRegionRef>(ref));
        }
        
        view_->notifySelection(view_, 
            selected_refs.data(), selected_refs.size(),
            deselected_refs.data(), deselected_refs.size());
    }
#endif
    return ARANullResponse{};
}

ARANullResponse AraEditorViewProxyImpl::notify_visibility(
    const AraEditorViewProxy::NotifyVisibilityRequest& request) {
#ifdef WITH_ARA
    if (view_) {
        // Convert visibility refs to ARA refs
        std::vector<ARA::PlugIn::PlaybackRegionRef> visible_refs;
        std::vector<ARA::PlugIn::PlaybackRegionRef> hidden_refs;
        
        visible_refs.reserve(request.visible_region_refs.size());
        for (auto ref : request.visible_region_refs) {
            visible_refs.push_back(reinterpret_cast<ARA::PlugIn::PlaybackRegionRef>(ref));
        }
        
        hidden_refs.reserve(request.hidden_region_refs.size());
        for (auto ref : request.hidden_region_refs) {
            hidden_refs.push_back(reinterpret_cast<ARA::PlugIn::PlaybackRegionRef>(ref));
        }
        
        view_->notifyVisibility(view_, 
            visible_refs.data(), visible_refs.size(),
            hidden_refs.data(), hidden_refs.size());
    }
#endif
    return ARANullResponse{};
}