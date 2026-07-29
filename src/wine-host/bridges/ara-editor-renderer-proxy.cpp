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

#include "ara-editor-renderer-proxy.h"

#include <cstring>

#ifdef WITH_ARA
#include "ARAInterface.h"
#include "ARAPlug.h"
#endif

// Helper to convert our serialized types to ARA types
#ifdef WITH_ARA
static void copy_editor_renderer_properties(const ARAEditorRendererProperties& src, ARA::PlugIn::EditorRendererProperties& dst) {
    // No properties to copy for now
}
#endif

AraEditorRendererProxyImpl::AraEditorRendererProxyImpl(ARABridge& bridge, Ref ref)
    : bridge_(bridge), ref_(ref) {
#ifdef WITH_ARA
    // The editor renderer would be created through the document controller
    // For now, we just store the reference
#endif
}

AraEditorRendererProxyImpl::~AraEditorRendererProxyImpl() noexcept = default;

// EditorRenderer proxy methods
AraEditorRendererProxy::ConstructArgs AraEditorRendererProxyImpl::create_editor_renderer() {
    bridge_.logger_.log_info([&](auto& log) {
        log << "ARA: CreateEditorRenderer for ref " << ref_;
    });
    return ConstructArgs{ref_, "EditorRenderer"};
}

void AraEditorRendererProxyImpl::destroy_editor_renderer(Ref renderer_ref) {
    bridge_.logger_.log_info([&](auto& log) {
        log << "ARA: DestroyEditorRenderer for ref " << renderer_ref;
    });
}

AraEditorRendererProxy::EditorRendererPropertiesResponse AraEditorRendererProxyImpl::get_properties(
    const AraEditorRendererProxy::GetPropertiesRequest& request) {
    EditorRendererPropertiesResponse response;
#ifdef WITH_ARA
    if (renderer_) {
        ARA::PlugIn::EditorRendererProperties props;
        copy_editor_renderer_properties(response.properties, props);
        response.success = true;
    }
#endif
    return response;
}

ARANullResponse AraEditorRendererProxyImpl::set_properties(
    const AraEditorRendererProxy::SetPropertiesRequest& request) {
#ifdef WITH_ARA
    if (renderer_) {
        ARA::PlugIn::EditorRendererProperties props;
        copy_editor_renderer_properties(request.properties, props);
        renderer_->setProperties(renderer_, &props);
    }
#endif
    return ARANullResponse{};
}

AraEditorRendererProxy::EditorRegionRenderResponse AraEditorRendererProxyImpl::render_region(
    const AraEditorRendererProxy::RenderRegionRequest& request) {
    EditorRegionRenderResponse response;
#ifdef WITH_ARA
    if (renderer_) {
        response.success = renderer_->renderRegion(
            renderer_,
            reinterpret_cast<ARA::PlugIn::PlaybackRegionRef>(request.region_ref),
            request.sample_rate,
            request.max_block_size,
            request.position) == kARATrue;
    }
#endif
    return response;
}

ARANullResponse AraEditorRendererProxyImpl::set_regions(
    const AraEditorRendererProxy::SetRegionsRequest& request) {
#ifdef WITH_ARA
    if (renderer_) {
        // Convert vector of refs to array of ARA refs
        std::vector<ARA::PlugIn::PlaybackRegionRef> ara_refs;
        ara_refs.reserve(request.region_refs.size());
        for (auto ref : request.region_refs) {
            ara_refs.push_back(reinterpret_cast<ARA::PlugIn::PlaybackRegionRef>(ref));
        }
        renderer_->setRegions(renderer_, ara_refs.data(), ara_refs.size());
    }
#endif
    return ARANullResponse{};
}