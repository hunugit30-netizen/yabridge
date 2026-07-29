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

#include "ara-playback-renderer-proxy.h"

#include <cstring>

#ifdef WITH_ARA
#include "ARAInterface.h"
#include "ARAPlug.h"
#endif

// Helper to convert our serialized types to ARA types
#ifdef WITH_ARA
static void copy_playback_renderer_properties(const ARAPlaybackRendererProperties& src, ARA::PlugIn::PlaybackRendererProperties& dst) {
    dst.flags = static_cast<ARAPlaybackTransformationFlags>(src.flags);
    dst.sampleRate = src.sample_rate;
    dst.maxBlockSize = src.max_block_size;
    dst.offline = src.offline ? kARATrue : kARAFalse;
}
#endif

AraPlaybackRendererProxy::AraPlaybackRendererProxy(ARABridge& bridge, Ref ref)
    : bridge_(bridge), ref_(ref) {
#ifdef WITH_ARA
    // The actual ARA PlaybackRenderer would be created here
    if (bridge_.ara_factory_) {
        // We would need to create the playback renderer through the document controller
        // For now, this is a placeholder
    }
#endif
}

AraPlaybackRendererProxy::~AraPlaybackRendererProxy() noexcept {
#ifdef WITH_ARA
    if (renderer_) {
        // The renderer will be destroyed by the document controller
    }
#endif
}

PlaybackRendererPropertiesResponse AraPlaybackRendererProxy::get_properties(const GetPropertiesRequest& request) {
#ifdef WITH_ARA
    if (renderer_) {
        ARA::PlugIn::PlaybackRendererProperties props;
        renderer_->getProperties(renderer_, &props);
        ARAPlaybackRendererProperties our_props;
        our_props.flags = static_cast<ARAPlaybackTransformationFlags>(props.flags);
        our_props.sample_rate = props.sampleRate;
        our_props.max_block_size = props.maxBlockSize;
        our_props.offline = props.offline == kARATrue;
        return PlaybackRendererPropertiesResponse{our_props, true};
    }
#endif
    return PlaybackRendererPropertiesResponse{{}, false};
}

ARANullResponse AraPlaybackRendererProxy::set_properties(const SetPropertiesRequest& request) {
#ifdef WITH_ARA
    if (renderer_) {
        ARA::PlugIn::PlaybackRendererProperties props;
        copy_playback_renderer_properties(request.properties, props);
        renderer_->setProperties(renderer_, &props);
    }
#endif
    return ARANullResponse{};
}

PlaybackRegionRenderResponse AraPlaybackRendererProxy::render_region(const RenderRegionRequest& request) {
#ifdef WITH_ARA
    if (renderer_) {
        double head_time = 0.0, tail_time = 0.0;
        renderer_->renderRegion(renderer_,
            reinterpret_cast<ARA::PlugIn::PlaybackRegionRef>(request.region_ref),
            request.sample_rate,
            request.max_block_size,
            request.position,
            &head_time,
            &tail_time);
        return PlaybackRegionRenderResponse{true, head_time, tail_time};
    }
#endif
    return PlaybackRegionRenderResponse{false, 0.0, 0.0};
}

ARANullResponse AraPlaybackRendererProxy::set_regions(const SetRegionsRequest& request) {
#ifdef WITH_ARA
    if (renderer_) {
        // Convert our refs to ARA refs
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