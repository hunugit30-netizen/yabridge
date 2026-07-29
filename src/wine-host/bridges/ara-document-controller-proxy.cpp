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

#include <cstring>
#include <algorithm>

#ifdef WITH_ARA
#include "ARAInterface.h"
#include "ARAPlug.h"
#endif

// Helper to convert our serialized types to ARA types
#ifdef WITH_ARA
static void copy_document_properties(const ARADocumentProperties& src, ARA::PlugIn::DocumentProperties& dst) {
    dst.name = src.name.c_str();
    dst.sampleRate = src.sample_rate;
}

static void copy_musical_context_properties(const ARAMusicalContextProperties& src, ARA::PlugIn::MusicalContextProperties& dst) {
    dst.name = src.name.c_str();
    dst.orderIndex = src.orderIndex;
    if (src.color) {
        ARAColor color = {src.color->r, src.color->g, src.color->b};
        dst.color = &color;
    } else {
        dst.color = nullptr;
    }
}

static void copy_audio_source_properties(const ARAAudioSourceProperties& src, ARA::PlugIn::AudioSourceProperties& dst) {
    dst.name = src.name.c_str();
    dst.persistentID = src.persistent_id;
    dst.sampleCount = src.sample_count;
    dst.sampleRate = src.sample_rate;
    dst.channelCount = src.channel_count;
    dst.merits64BitSamples = src.merits_64_bit_samples;
    dst.channelArrangementDataType = static_cast<ARAChannelArrangementDataType>(src.channel_arrangement_data_type);
    dst.channelArrangement = src.channel_arrangement;
}

static void copy_audio_modification_properties(const ARAAudioModificationProperties& src, ARA::PlugIn::AudioModificationProperties& dst) {
    dst.name = src.name.c_str();
    dst.persistentID = src.persistent_id;
}

static void copy_playback_region_properties(const ARAPlaybackRegionProperties& src, ARA::PlugIn::PlaybackRegionProperties& dst) {
    dst.transformationFlags = static_cast<ARAPlaybackTransformationFlags>(src.transformation_flags);
    dst.startInModificationTime = src.start_in_modification_time;
    dst.durationInModificationTime = src.duration_in_modification_time;
    dst.startInPlaybackTime = src.start_in_playback_time;
    dst.durationInPlaybackTime = src.duration_in_playback_time;
    dst.regionSequenceRef = src.region_sequence_ref;
    dst.name = src.name.c_str();
    if (src.color) {
        ARAColor color = {src.color->r, src.color->g, src.color->b};
        dst.color = &color;
    } else {
        dst.color = nullptr;
    }
}

static void copy_region_sequence_properties(const ARARegionSequenceProperties& src, ARA::PlugIn::RegionSequenceProperties& dst) {
    dst.name = src.name.c_str();
    dst.orderIndex = src.order_index;
    dst.musicalContextRef = src.musical_context_ref;
    if (src.color) {
        ARAColor color = {src.color->r, src.color->g, src.color->b};
        dst.color = &color;
    } else {
        dst.color = nullptr;
    }
}
#endif

AraDocumentControllerProxyImpl::AraDocumentControllerProxyImpl(ARABridge& bridge, Ref ref)
    : bridge_(bridge), ref_(ref) {
#ifdef WITH_ARA
    // Get the ARA factory and create a document controller
    if (bridge_.ara_factory_) {
        // Create host instance for the document controller
        ARA::PlugIn::DocumentControllerHostInstance host_instance;
        host_instance.hostRef = reinterpret_cast<ARAHostRef>(ref);
        host_instance.documentControllerHostRef = nullptr; // Will be set by host
        
        ARADocumentProperties properties;
        properties.name = "yabridge Document";
        properties.sample_rate = 44100.0;
        
        controller_ = bridge_.ara_factory_->createDocumentControllerWithDocument(
            &host_instance, &properties);
    }
#endif
}

AraDocumentControllerProxyImpl::~AraDocumentControllerProxyImpl() noexcept {
#ifdef WITH_ARA
    if (controller_) {
        controller_->destroyDocumentController(controller_);
    }
#endif
}

// Musical context operations
MusicalContextRefResponse AraDocumentControllerProxyImpl::create_musical_context(
    const AraDocumentControllerProxy::CreateMusicalContextRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        ARA::PlugIn::MusicalContextProperties props;
        copy_musical_context_properties(request.properties, props);
        
        ARA::PlugIn::MusicalContextHostRef host_ref = 
            reinterpret_cast<ARA::PlugIn::MusicalContextHostRef>(
                std::hash<std::string>{}(request.host_ref_id));
        
        ARA::PlugIn::MusicalContextRef context_ref = 
            controller_->createMusicalContext(controller_, host_ref, &props);
        
        return MusicalContextRefResponse{
            .context_ref = reinterpret_cast<uint64_t>(context_ref),
            .success = (context_ref != nullptr)
        };
    }
#endif
    return MusicalContextRefResponse{0, false};
}

ARANullResponse AraDocumentControllerProxyImpl::update_musical_context_properties(
    const AraDocumentControllerProxy::UpdateMusicalContextPropertiesRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        ARA::PlugIn::MusicalContextProperties props;
        copy_musical_context_properties(request.properties, props);
        
        controller_->updateMusicalContextProperties(
            controller_, 
            reinterpret_cast<ARA::PlugIn::MusicalContextRef>(request.context_ref),
            &props);
    }
#endif
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::update_musical_context_content(
    const AraDocumentControllerProxy::UpdateMusicalContextContentRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        const ARAContentTimeRange* range = nullptr;
        if (request.range.has_value()) {
            // Convert our range to ARA range
            static ARAContentTimeRange ara_range;
            ara_range.start = request.range->start;
            ara_range.duration = request.range->duration;
            range = &ara_range;
        }
        
        controller_->updateMusicalContextContent(
            controller_,
            reinterpret_cast<ARA::PlugIn::MusicalContextRef>(request.context_ref),
            range,
            static_cast<ARAContentUpdateFlags>(request.flags));
    }
#endif
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::destroy_musical_context(
    const AraDocumentControllerProxy::DestroyMusicalContextRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        controller_->destroyMusicalContext(
            controller_,
            reinterpret_cast<ARA::PlugIn::MusicalContextRef>(request.context_ref));
    }
#endif
    return ARANullResponse{};
}

// Audio source operations
AudioSourceRefResponse AraDocumentControllerProxyImpl::create_audio_source(
    const AraDocumentControllerProxy::CreateAudioSourceRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        ARA::PlugIn::AudioSourceProperties props;
        copy_audio_source_properties(request.properties, props);
        
        ARA::PlugIn::AudioSourceHostRef host_ref = 
            reinterpret_cast<ARA::PlugIn::AudioSourceHostRef>(
                std::hash<std::string>{}(request.host_ref_id));
        
        ARA::PlugIn::AudioSourceRef source_ref = 
            controller_->createAudioSource(controller_, host_ref, &props);
        
        return AudioSourceRefResponse{
            .source_ref = reinterpret_cast<uint64_t>(source_ref),
            .success = (source_ref != nullptr)
        };
    }
#endif
    return AudioSourceRefResponse{0, false};
}

ARANullResponse AraDocumentControllerProxyImpl::update_audio_source_properties(
    const AraDocumentControllerProxy::UpdateAudioSourcePropertiesRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        ARA::PlugIn::AudioSourceProperties props;
        copy_audio_source_properties(request.properties, props);
        
        controller_->updateAudioSourceProperties(
            controller_,
            reinterpret_cast<ARA::PlugIn::AudioSourceRef>(request.source_ref),
            &props);
    }
#endif
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::update_audio_source_content(
    const AraDocumentControllerProxy::UpdateAudioSourceContentRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        const ARAContentTimeRange* range = nullptr;
        if (request.range.has_value()) {
            static ARAContentTimeRange ara_range;
            ara_range.start = request.range->start;
            ara_range.duration = request.range->duration;
            range = &ara_range;
        }
        
        controller_->updateAudioSourceContent(
            controller_,
            reinterpret_cast<ARA::PlugIn::AudioSourceRef>(request.source_ref),
            range,
            static_cast<ARAContentUpdateFlags>(request.flags));
    }
#endif
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::enable_audio_source_samples_access(
    const AraDocumentControllerProxy::EnableAudioSourceSamplesAccessRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        controller_->enableAudioSourceSamplesAccess(
            controller_,
            reinterpret_cast<ARA::PlugIn::AudioSourceRef>(request.source_ref),
            request.enable ? kARATrue : kARAFalse);
    }
#endif
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::deactivate_audio_source_for_undo_history(
    const AraDocumentControllerProxy::DeactivateAudioSourceForUndoHistoryRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        controller_->deactivateAudioSourceForUndoHistory(
            controller_,
            reinterpret_cast<ARA::PlugIn::AudioSourceRef>(request.source_ref),
            request.deactivate ? kARATrue : kARAFalse);
    }
#endif
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::destroy_audio_source(
    const AraDocumentControllerProxy::DestroyAudioSourceRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        controller_->destroyAudioSource(
            controller_,
            reinterpret_cast<ARA::PlugIn::AudioSourceRef>(request.source_ref));
    }
#endif
    return ARANullResponse{};
}

// Audio modification operations
AudioModificationRefResponse AraDocumentControllerProxyImpl::create_audio_modification(
    const AraDocumentControllerProxy::CreateAudioModificationRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        ARA::PlugIn::AudioModificationProperties props;
        copy_audio_modification_properties(request.properties, props);
        
        ARA::PlugIn::AudioModificationHostRef host_ref = 
            reinterpret_cast<ARA::PlugIn::AudioModificationHostRef>(
                std::hash<std::string>{}(request.host_ref_id));
        
        ARA::PlugIn::AudioModificationRef mod_ref = 
            controller_->createAudioModification(
                controller_,
                reinterpret_cast<ARA::PlugIn::AudioSourceRef>(request.audio_source_ref),
                host_ref,
                &props);
        
        return AudioModificationRefResponse{
            .modification_ref = reinterpret_cast<uint64_t>(mod_ref),
            .success = (mod_ref != nullptr)
        };
    }
#endif
    return AudioModificationRefResponse{0, false};
}

AudioModificationRefResponse AraDocumentControllerProxyImpl::clone_audio_modification(
    const AraDocumentControllerProxy::CloneAudioModificationRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        ARA::PlugIn::AudioModificationProperties props;
        copy_audio_modification_properties(request.properties, props);
        
        ARA::PlugIn::AudioModificationHostRef host_ref = 
            reinterpret_cast<ARA::PlugIn::AudioModificationHostRef>(
                std::hash<std::string>{}(request.host_ref_id));
        
        ARA::PlugIn::AudioModificationRef mod_ref = 
            controller_->cloneAudioModification(
                controller_,
                reinterpret_cast<ARA::PlugIn::AudioModificationRef>(request.source_modification_ref),
                host_ref,
                &props);
        
        return AudioModificationRefResponse{
            .modification_ref = reinterpret_cast<uint64_t>(mod_ref),
            .success = (mod_ref != nullptr)
        };
    }
#endif
    return AudioModificationRefResponse{0, false};
}

ARANullResponse AraDocumentControllerProxyImpl::update_audio_modification_properties(
    const AraDocumentControllerProxy::UpdateAudioModificationPropertiesRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        ARA::PlugIn::AudioModificationProperties props;
        copy_audio_modification_properties(request.properties, props);
        
        controller_->updateAudioModificationProperties(
            controller_,
            reinterpret_cast<ARA::PlugIn::AudioModificationRef>(request.modification_ref),
            &props);
    }
#endif
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::deactivate_audio_modification_for_undo_history(
    const AraDocumentControllerProxy::DeactivateAudioModificationForUndoHistoryRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        controller_->deactivateAudioModificationForUndoHistory(
            controller_,
            reinterpret_cast<ARA::PlugIn::AudioModificationRef>(request.modification_ref),
            request.deactivate ? kARATrue : kARAFalse);
    }
#endif
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::destroy_audio_modification(
    const AraDocumentControllerProxy::DestroyAudioModificationRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        controller_->destroyAudioModification(
            controller_,
            reinterpret_cast<ARA::PlugIn::AudioModificationRef>(request.modification_ref));
    }
#endif
    return ARANullResponse{};
}

// Playback region operations
PlaybackRegionRefResponse AraDocumentControllerProxyImpl::create_playback_region(
    const AraDocumentControllerProxy::CreatePlaybackRegionRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        ARA::PlugIn::PlaybackRegionProperties props;
        copy_playback_region_properties(request.properties, props);
        
        ARA::PlugIn::PlaybackRegionHostRef host_ref = 
            reinterpret_cast<ARA::PlugIn::PlaybackRegionHostRef>(
                std::hash<std::string>{}(request.host_ref_id));
        
        ARA::PlugIn::PlaybackRegionRef region_ref = 
            controller_->createPlaybackRegion(
                controller_,
                reinterpret_cast<ARA::PlugIn::AudioModificationRef>(request.audio_modification_ref),
                host_ref,
                &props);
        
        return PlaybackRegionRefResponse{
            .region_ref = reinterpret_cast<uint64_t>(region_ref),
            .success = (region_ref != nullptr)
        };
    }
#endif
    return PlaybackRegionRefResponse{0, false};
}

ARANullResponse AraDocumentControllerProxyImpl::update_playback_region_properties(
    const AraDocumentControllerProxy::UpdatePlaybackRegionPropertiesRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        ARA::PlugIn::PlaybackRegionProperties props;
        copy_playback_region_properties(request.properties, props);
        
        controller_->updatePlaybackRegionProperties(
            controller_,
            reinterpret_cast<ARA::PlugIn::PlaybackRegionRef>(request.region_ref),
            &props);
    }
#endif
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::destroy_playback_region(
    const AraDocumentControllerProxy::DestroyPlaybackRegionRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        controller_->destroyPlaybackRegion(
            controller_,
            reinterpret_cast<ARA::PlugIn::PlaybackRegionRef>(request.region_ref));
    }
#endif
    return ARANullResponse{};
}

// Region sequence operations
RegionSequenceRefResponse AraDocumentControllerProxyImpl::create_region_sequence(
    const AraDocumentControllerProxy::CreateRegionSequenceRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        ARA::PlugIn::RegionSequenceProperties props;
        copy_region_sequence_properties(request.properties, props);
        
        ARA::PlugIn::RegionSequenceHostRef host_ref = 
            reinterpret_cast<ARA::PlugIn::RegionSequenceHostRef>(
                std::hash<std::string>{}(request.host_ref_id));
        
        ARA::PlugIn::RegionSequenceRef seq_ref = 
            controller_->createRegionSequence(controller_, host_ref, &props);
        
        return RegionSequenceRefResponse{
            .sequence_ref = reinterpret_cast<uint64_t>(seq_ref),
            .success = (seq_ref != nullptr)
        };
    }
#endif
    return RegionSequenceRefResponse{0, false};
}

CountResponse AraDocumentControllerProxyImpl::get_region_sequence_count(
    const AraDocumentControllerProxy::GetRegionSequenceCountRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        ARASize count = controller_->getRegionSequenceCount(controller_);
        return CountResponse{static_cast<uint32_t>(count)};
    }
#endif
    return CountResponse{0};
}

RegionSequencePropertiesResponse AraDocumentControllerProxyImpl::get_region_sequence_properties(
    const AraDocumentControllerProxy::GetRegionSequencePropertiesRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        ARA::PlugIn::RegionSequenceProperties props;
        if (controller_->getRegionSequenceProperties(
                controller_,
                reinterpret_cast<ARA::PlugIn::RegionSequenceRef>(request.sequence_ref),
                &props)) {
            ARARegionSequenceProperties our_props;
            our_props.name = props.name ? props.name : "";
            our_props.order_index = props.orderIndex;
            our_props.musical_context_ref = reinterpret_cast<uint64_t>(props.musicalContextRef);
            if (props.color) {
                ARAColor color = {props.color->r, props.color->g, props.color->b};
                our_props.color = std::make_unique<ARAColor>(color);
            }
            
            return RegionSequencePropertiesResponse{our_props, true};
        }
    }
#endif
    return RegionSequencePropertiesResponse{{}, false};
}

ARANullResponse AraDocumentControllerProxyImpl::set_region_sequence_properties(
    const AraDocumentControllerProxy::SetRegionSequencePropertiesRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        ARA::PlugIn::RegionSequenceProperties props;
        copy_region_sequence_properties(request.properties, props);
        
        controller_->setRegionSequenceProperties(
            controller_,
            reinterpret_cast<ARA::PlugIn::RegionSequenceRef>(request.sequence_ref),
            &props);
    }
#endif
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::destroy_region_sequence(
    const AraDocumentControllerProxy::DestroyRegionSequenceRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        controller_->destroyRegionSequence(
            controller_,
            reinterpret_cast<ARA::PlugIn::RegionSequenceRef>(request.sequence_ref));
    }
#endif
    return ARANullResponse{};
}

// Archive operations
ARANullResponse AraDocumentControllerProxyImpl::store_objects_to_archive(
    const AraDocumentControllerProxy::StoreObjectsToArchiveRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        // TODO: Implement archive writer host ref
        // For now, just call with null
        controller_->storeObjectsToArchive(
            controller_,
            nullptr,
            nullptr,
            0,
            nullptr,
            0);
    }
#endif
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::restore_objects_from_archive(
    const AraDocumentControllerProxy::RestoreObjectsFromArchiveRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        // TODO: Implement archive reader host ref
        controller_->restoreObjectsFromArchive(
            controller_,
            nullptr,
            nullptr,
            0,
            nullptr,
            0,
            nullptr,
            0,
            request.include_document_data ? kARATrue : kARAFalse);
    }
#endif
    return ARANullResponse{};
}

// Analysis operations
ARANullResponse AraDocumentControllerProxyImpl::analyze_audio_source(
    const AraDocumentControllerProxy::AnalyzeAudioSourceRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        controller_->analyzeAudioSource(
            controller_,
            reinterpret_cast<ARA::PlugIn::AudioSourceRef>(request.audio_source_ref));
    }
#endif
    return ARANullResponse{};
}

// Editing operations
ARANullResponse AraDocumentControllerProxyImpl::begin_editing(
    const AraDocumentControllerProxy::BeginEditingRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        controller_->beginEditing(controller_);
    }
#endif
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::end_editing(
    const AraDocumentControllerProxy::EndEditingRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        controller_->endEditing(controller_);
    }
#endif
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::notify_model_updates(
    const AraDocumentControllerProxy::NotifyModelUpdatesRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        controller_->notifyModelUpdates(controller_);
    }
#endif
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::update_document_properties(
    const AraDocumentControllerProxy::UpdateDocumentPropertiesRequest& request) {
#ifdef WITH_ARA
    if (controller_) {
        ARA::PlugIn::DocumentProperties props;
        copy_document_properties(request.properties, props);
        controller_->updateDocumentProperties(controller_, &props);
    }
#endif
    return ARANullResponse{};
}