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

#include <shared_mutex>
#include <map>
#include <string>
#include <vector>
#include <optional>

#include "../../../common/serialization/ara.h"
#include "../common.h"

#ifdef WITH_ARA
#include "ARAInterface.h"
#endif

/**
 * DocumentController Proxy on the Wine host side.
 * Wraps the ARA::PlugIn::DocumentController and forwards calls to the native plugin side.
 */
class AraDocumentControllerProxy {
   public:
    using Ref = uint64_t;

    struct ConstructArgs {
        ConstructArgs() noexcept = default;
        ConstructArgs(Ref ref, const std::string& id) noexcept : controller_ref(ref), controller_id(id) {}

        Ref controller_ref = 0;
        std::string controller_id;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.text1b(controller_id, 256);
        }
    };

    struct CreateMusicalContextRequest {
        using Response = MusicalContextRefResponse;

        Ref controller_ref = 0;
        std::string host_ref_id;
        ARAMusicalContextProperties properties;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.text1b(host_ref_id, 256);
            s.object(properties);
        }
    };

    struct UpdateMusicalContextPropertiesRequest {
        using Response = ARANullResponse;

        Ref controller_ref = 0;
        Ref context_ref = 0;
        ARAMusicalContextProperties properties;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.value8b(context_ref);
            s.object(properties);
        }
    };

    struct UpdateMusicalContextContentRequest {
        using Response = ARANullResponse;

        Ref controller_ref = 0;
        Ref context_ref = 0;
        std::optional<ARAContentTimeRange> range;
        ARAContentUpdateFlags flags = ARAContentUpdateFlags::kARAContentUpdateNone;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.value8b(context_ref);
            s.ext(range, bitsery::ext::InPlaceOptional{});
            s.value4b(flags);
        }
    };

    struct DestroyMusicalContextRequest {
        using Response = ARANullResponse;

        Ref controller_ref = 0;
        Ref context_ref = 0;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.value8b(context_ref);
        }
    };

    struct CreateAudioSourceRequest {
        using Response = AudioSourceRefResponse;

        Ref controller_ref = 0;
        std::string host_ref_id;
        ARAAudioSourceProperties properties;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.text1b(host_ref_id, 256);
            s.object(properties);
        }
    };

    struct UpdateAudioSourcePropertiesRequest {
        using Response = ARANullResponse;

        Ref controller_ref = 0;
        Ref source_ref = 0;
        ARAAudioSourceProperties properties;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.value8b(source_ref);
            s.object(properties);
        }
    };

    struct UpdateAudioSourceContentRequest {
        using Response = ARANullResponse;

        Ref controller_ref = 0;
        Ref source_ref = 0;
        std::optional<ARAContentTimeRange> range;
        ARAContentUpdateFlags flags = ARAContentUpdateFlags::kARAContentUpdateNone;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.value8b(source_ref);
            s.ext(range, bitsery::ext::InPlaceOptional{});
            s.value4b(flags);
        }
    };

    struct EnableAudioSourceSamplesAccessRequest {
        using Response = ARANullResponse;

        Ref controller_ref = 0;
        Ref source_ref = 0;
        bool enable = false;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.value8b(source_ref);
            s.value1b(enable);
        }
    };

    struct DeactivateAudioSourceForUndoHistoryRequest {
        using Response = ARANullResponse;

        Ref controller_ref = 0;
        Ref source_ref = 0;
        bool deactivate = false;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.value8b(source_ref);
            s.value1b(deactivate);
        }
    };

    struct DestroyAudioSourceRequest {
        using Response = ARANullResponse;

        Ref controller_ref = 0;
        Ref source_ref = 0;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.value8b(source_ref);
        }
    };

    struct CreateAudioModificationRequest {
        using Response = AudioModificationRefResponse;

        Ref controller_ref = 0;
        Ref audio_source_ref = 0;
        std::string host_ref_id;
        ARAAudioModificationProperties properties;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.value8b(audio_source_ref);
            s.text1b(host_ref_id, 256);
            s.object(properties);
        }
    };

    struct CloneAudioModificationRequest {
        using Response = AudioModificationRefResponse;

        Ref controller_ref = 0;
        Ref source_modification_ref = 0;
        std::string host_ref_id;
        ARAAudioModificationProperties properties;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.value8b(source_modification_ref);
            s.text1b(host_ref_id, 256);
            s.object(properties);
        }
    };

    struct UpdateAudioModificationPropertiesRequest {
        using Response = ARANullResponse;

        Ref controller_ref = 0;
        Ref modification_ref = 0;
        ARAAudioModificationProperties properties;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.value8b(modification_ref);
            s.object(properties);
        }
    };

    struct DeactivateAudioModificationForUndoHistoryRequest {
        using Response = ARANullResponse;

        Ref controller_ref = 0;
        Ref modification_ref = 0;
        bool deactivate = false;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.value8b(modification_ref);
            s.value1b(deactivate);
        }
    };

    struct DestroyAudioModificationRequest {
        using Response = ARANullResponse;

        Ref controller_ref = 0;
        Ref modification_ref = 0;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.value8b(modification_ref);
        }
    };

    struct CreatePlaybackRegionRequest {
        using Response = PlaybackRegionRefResponse;

        Ref controller_ref = 0;
        Ref audio_modification_ref = 0;
        std::string host_ref_id;
        ARAPlaybackRegionProperties properties;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.value8b(audio_modification_ref);
            s.text1b(host_ref_id, 256);
            s.object(properties);
        }
    };

    struct UpdatePlaybackRegionPropertiesRequest {
        using Response = ARANullResponse;

        Ref controller_ref = 0;
        Ref region_ref = 0;
        ARAPlaybackRegionProperties properties;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.value8b(region_ref);
            s.object(properties);
        }
    };

    struct DestroyPlaybackRegionRequest {
        using Response = ARANullResponse;

        Ref controller_ref = 0;
        Ref region_ref = 0;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.value8b(region_ref);
        }
    };

    struct CreateRegionSequenceRequest {
        using Response = RegionSequenceRefResponse;

        Ref controller_ref = 0;
        std::string host_ref_id;
        ARARegionSequenceProperties properties;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.text1b(host_ref_id, 256);
            s.object(properties);
        }
    };

    struct GetRegionSequenceCountRequest {
        using Response = CountResponse;

        Ref controller_ref = 0;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
        }
    };

    struct GetRegionSequencePropertiesRequest {
        using Response = RegionSequencePropertiesResponse;

        Ref controller_ref = 0;
        Ref sequence_ref = 0;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.value8b(sequence_ref);
        }
    };

    struct SetRegionSequencePropertiesRequest {
        using Response = ARANullResponse;

        Ref controller_ref = 0;
        Ref sequence_ref = 0;
        ARARegionSequenceProperties properties;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.value8b(sequence_ref);
            s.object(properties);
        }
    };

    struct DestroyRegionSequenceRequest {
        using Response = ARANullResponse;

        Ref controller_ref = 0;
        Ref sequence_ref = 0;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.value8b(sequence_ref);
        }
    };

    struct StoreObjectsToArchiveRequest {
        using Response = ARANullResponse;

        Ref controller_ref = 0;
        std::string archive_writer_host_ref_id;
        std::optional<std::vector<ARAPersistentID>> audio_source_ids;
        std::optional<std::vector<ARAPersistentID>> audio_modification_ids;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.text1b(archive_writer_host_ref_id, 256);
            s.ext(audio_source_ids, bitsery::ext::InPlaceOptional{});
            s.ext(audio_modification_ids, bitsery::ext::InPlaceOptional{});
        }
    };

    struct RestoreObjectsFromArchiveRequest {
        using Response = ARANullResponse;

        Ref controller_ref = 0;
        std::string archive_reader_host_ref_id;
        std::optional<std::vector<ARAPersistentID>> audio_source_archive_ids;
        std::optional<std::vector<ARAPersistentID>> audio_source_current_ids;
        std::optional<std::vector<ARAPersistentID>> audio_modification_archive_ids;
        std::optional<std::vector<ARAPersistentID>> audio_modification_current_ids;
        bool include_document_data = true;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.text1b(archive_reader_host_ref_id, 256);
            s.ext(audio_source_archive_ids, bitsery::ext::InPlaceOptional{});
            s.ext(audio_source_current_ids, bitsery::ext::InPlaceOptional{});
            s.ext(audio_modification_archive_ids, bitsery::ext::InPlaceOptional{});
            s.ext(audio_modification_current_ids, bitsery::ext::InPlaceOptional{});
            s.value1b(include_document_data);
        }
    };

    struct AnalyzeAudioSourceRequest {
        using Response = ARANullResponse;

        Ref controller_ref = 0;
        Ref audio_source_ref = 0;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.value8b(audio_source_ref);
        }
    };

    struct BeginEditingRequest {
        using Response = ARANullResponse;

        Ref controller_ref = 0;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
        }
    };

    struct EndEditingRequest {
        using Response = ARANullResponse;

        Ref controller_ref = 0;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
        }
    };

    struct NotifyModelUpdatesRequest {
        using Response = ARANullResponse;

        Ref controller_ref = 0;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
        }
    };

    struct UpdateDocumentPropertiesRequest {
        using Response = ARANullResponse;

        Ref controller_ref = 0;
        ARADocumentProperties properties;

        template <typename S>
        void serialize(S& s) {
            s.value8b(controller_ref);
            s.object(properties);
        }
    };

    // Response types
    struct MusicalContextRefResponse {
        Ref context_ref = 0;
        bool success = false;

        template <typename S>
        void serialize(S& s) {
            s.value8b(context_ref);
            s.value1b(success);
        }
    };

    struct AudioSourceRefResponse {
        Ref source_ref = 0;
        bool success = false;

        template <typename S>
        void serialize(S& s) {
            s.value8b(source_ref);
            s.value1b(success);
        }
    };

    struct AudioModificationRefResponse {
        Ref modification_ref = 0;
        bool success = false;

        template <typename S>
        void serialize(S& s) {
            s.value8b(modification_ref);
            s.value1b(success);
        }
    };

    struct PlaybackRegionRefResponse {
        Ref region_ref = 0;
        bool success = false;

        template <typename S>
        void serialize(S& s) {
            s.value8b(region_ref);
            s.value1b(success);
        }
    };

    struct RegionSequenceRefResponse {
        Ref sequence_ref = 0;
        bool success = false;

        template <typename S>
        void serialize(S& s) {
            s.value8b(sequence_ref);
            s.value1b(success);
        }
    };

    struct CountResponse {
        uint32_t count = 0;

        template <typename S>
        void serialize(S& s) {
            s.value4b(count);
        }
    };

    struct RegionSequencePropertiesResponse {
        ARARegionSequenceProperties properties;
        bool success = false;

        template <typename S>
        void serialize(S& s) {
            s.object(properties);
            s.value1b(success);
        }
    };

    AraDocumentControllerProxy() = default;
    virtual ~AraDocumentControllerProxy() noexcept = default;

    // Factory method to create a document controller
    virtual ConstructArgs create_document_controller(const ARADocumentProperties& properties) = 0;
    virtual void destroy_document_controller(Ref controller_ref) = 0;

    // Document controller operations
    virtual MusicalContextRefResponse create_musical_context(const CreateMusicalContextRequest& request) = 0;
    virtual ARANullResponse update_musical_context_properties(const UpdateMusicalContextPropertiesRequest& request) = 0;
    virtual ARANullResponse update_musical_context_content(const UpdateMusicalContextContentRequest& request) = 0;
    virtual ARANullResponse destroy_musical_context(const DestroyMusicalContextRequest& request) = 0;

    virtual AudioSourceRefResponse create_audio_source(const CreateAudioSourceRequest& request) = 0;
    virtual ARANullResponse update_audio_source_properties(const UpdateAudioSourcePropertiesRequest& request) = 0;
    virtual ARANullResponse update_audio_source_content(const UpdateAudioSourceContentRequest& request) = 0;
    virtual ARANullResponse enable_audio_source_samples_access(const EnableAudioSourceSamplesAccessRequest& request) = 0;
    virtual ARANullResponse deactivate_audio_source_for_undo_history(const DeactivateAudioSourceForUndoHistoryRequest& request) = 0;
    virtual ARANullResponse destroy_audio_source(const DestroyAudioSourceRequest& request) = 0;

    virtual AudioModificationRefResponse create_audio_modification(const CreateAudioModificationRequest& request) = 0;
    virtual AudioModificationRefResponse clone_audio_modification(const CloneAudioModificationRequest& request) = 0;
    virtual ARANullResponse update_audio_modification_properties(const UpdateAudioModificationPropertiesRequest& request) = 0;
    virtual ARANullResponse deactivate_audio_modification_for_undo_history(const DeactivateAudioModificationForUndoHistoryRequest& request) = 0;
    virtual ARANullResponse destroy_audio_modification(const DestroyAudioModificationRequest& request) = 0;

    virtual PlaybackRegionRefResponse create_playback_region(const CreatePlaybackRegionRequest& request) = 0;
    virtual ARANullResponse update_playback_region_properties(const UpdatePlaybackRegionPropertiesRequest& request) = 0;
    virtual ARANullResponse destroy_playback_region(const DestroyPlaybackRegionRequest& request) = 0;

    virtual RegionSequenceRefResponse create_region_sequence(const CreateRegionSequenceRequest& request) = 0;
    virtual CountResponse get_region_sequence_count(const GetRegionSequenceCountRequest& request) = 0;
    virtual RegionSequencePropertiesResponse get_region_sequence_properties(const GetRegionSequencePropertiesRequest& request) = 0;
    virtual ARANullResponse set_region_sequence_properties(const SetRegionSequencePropertiesRequest& request) = 0;
    virtual ARANullResponse destroy_region_sequence(const DestroyRegionSequenceRequest& request) = 0;

    virtual ARANullResponse store_objects_to_archive(const StoreObjectsToArchiveRequest& request) = 0;
    virtual ARANullResponse restore_objects_from_archive(const RestoreObjectsFromArchiveRequest& request) = 0;

    virtual ARANullResponse analyze_audio_source(const AnalyzeAudioSourceRequest& request) = 0;

    virtual ARANullResponse begin_editing(const BeginEditingRequest& request) = 0;
    virtual ARANullResponse end_editing(const EndEditingRequest& request) = 0;
    virtual ARANullResponse notify_model_updates(const NotifyModelUpdatesRequest& request) = 0;
    virtual ARANullResponse update_document_properties(const UpdateDocumentPropertiesRequest& request) = 0;
};