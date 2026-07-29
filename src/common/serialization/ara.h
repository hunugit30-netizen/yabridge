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

#include <variant>
#include <optional>
#include <string>
#include <vector>

#include "../bitsery/ext/in-place-variant.h"
#include "../bitsery/ext/in-place-optional.h"
#include "../utils.h"
#include "common.h"

/**
 * ARA (Audio Random Access) serialization structures.
 * 
 * These define the message types used for IPC between the native
 * plugin and the Wine host for ARA interface proxying.
 */

// Forward declarations for response types
struct Ack;

/**
 * Empty response for operations that don't return data.
 */
struct ARANullResponse {
    template <typename S>
    void serialize(S&) {}
};

/**
 * Basic ARA types that need to be serialized.
 */

// ARA persistent ID (64-bit opaque identifier)
using ARAPersistentID = uint64_t;

// ARA content type
enum class ARAContentType : int32_t {
    kARAContentTypeUnspecified = 0,
    kARAContentTypeAudio = 1,
    kARAContentTypeNote = 2
};

// ARA content update flags
enum class ARAContentUpdateFlags : int32_t {
    kARAContentUpdateNone = 0,
    kARAContentUpdateSignal = 1 << 0,
    kARAContentUpdateAnalysis = 1 << 1,
    kARAContentUpdateAll = kARAContentUpdateSignal | kARAContentUpdateAnalysis
};

// ARA playback transformation flags
enum class ARAPlaybackTransformationFlags : int32_t {
    kARAPlaybackTransformationNone = 0,
    kARAPlaybackTransformationTimeStretch = 1 << 0,
    kARAPlaybackTransformationPitchShift = 1 << 1,
    kARAPlaybackTransformationGain = 1 << 2,
    kARAPlaybackTransformationFade = 1 << 3,
    kARAPlaybackTransformationAll = kARAPlaybackTransformationTimeStretch |
                                     kARAPlaybackTransformationPitchShift |
                                     kARAPlaybackTransformationGain |
                                     kARAPlaybackTransformationFade
};

// ARA plug-in instance role flags
enum class ARAPlugInInstanceRoleFlags : int32_t {
    kARAPlaybackRendererRole = 1 << 0,
    kARAEditorRendererRole = 1 << 1,
    kARAEditorViewRole = 1 << 2
};

// ARA content time range
struct ARAContentTimeRange {
    double start = 0.0;
    double duration = 0.0;

    template <typename S>
    void serialize(S& s) {
        s.value8b(start);
        s.value8b(duration);
    }
};

// ARA document properties
struct ARADocumentProperties {
    ARAPersistentID persistent_id = 0;
    std::string name;
    double sample_rate = 44100.0;

    template <typename S>
    void serialize(S& s) {
        s.value8b(persistent_id);
        s.text1b(name, 256);
        s.value8b(sample_rate);
    }
};

// ARA musical context properties
struct ARAMusicalContextProperties {
    ARAPersistentID persistent_id = 0;
    std::string name;
    double sample_rate = 44100.0;
    double tempo = 120.0;
    int32_t time_signature_numerator = 4;
    int32_t time_signature_denominator = 4;

    template <typename S>
    void serialize(S& s) {
        s.value8b(persistent_id);
        s.text1b(name, 256);
        s.value8b(sample_rate);
        s.value8b(tempo);
        s.value4b(time_signature_numerator);
        s.value4b(time_signature_denominator);
    }
};

// ARA audio source properties
struct ARAAudioSourceProperties {
    ARAPersistentID persistent_id = 0;
    std::string name;
    int32_t channel_count = 2;
    double sample_rate = 44100.0;
    int64_t sample_count = 0;
    bool is_tempo_based = false;

    template <typename S>
    void serialize(S& s) {
        s.value8b(persistent_id);
        s.text1b(name, 256);
        s.value4b(channel_count);
        s.value8b(sample_rate);
        s.value8b(sample_count);
        s.value1b(is_tempo_based);
    }
};

// ARA audio modification properties
struct ARAAudioModificationProperties {
    ARAPersistentID persistent_id = 0;
    std::string name;
    ARAPersistentID audio_source_id = 0;
    double playback_start = 0.0;
    double playback_duration = 0.0;
    double content_start = 0.0;
    double content_duration = 0.0;
    double pitch = 0.0;
    double gain = 1.0;
    double pan = 0.0;

    template <typename S>
    void serialize(S& s) {
        s.value8b(persistent_id);
        s.text1b(name, 256);
        s.value8b(audio_source_id);
        s.value8b(playback_start);
        s.value8b(playback_duration);
        s.value8b(content_start);
        s.value8b(content_duration);
        s.value8b(pitch);
        s.value8b(gain);
        s.value8b(pan);
    }
};

// ARA playback region properties
struct ARAPlaybackRegionProperties {
    ARAPersistentID persistent_id = 0;
    std::string name;
    ARAPersistentID audio_modification_id = 0;
    double playback_start = 0.0;
    double playback_duration = 0.0;
    double content_start = 0.0;
    double content_duration = 0.0;
    double pitch = 0.0;
    double gain = 1.0;
    double pan = 0.0;

    template <typename S>
    void serialize(S& s) {
        s.value8b(persistent_id);
        s.text1b(name, 256);
        s.value8b(audio_modification_id);
        s.value8b(playback_start);
        s.value8b(playback_duration);
        s.value8b(content_start);
        s.value8b(content_duration);
        s.value8b(pitch);
        s.value8b(gain);
        s.value8b(pan);
    }
};

// ARA region sequence properties
struct ARARegionSequenceProperties {
    ARAPersistentID persistent_id = 0;
    std::string name;
    std::vector<ARAPersistentID> playback_region_ids;

    template <typename S>
    void serialize(S& s) {
        s.value8b(persistent_id);
        s.text1b(name, 256);
        s.container1b(playback_region_ids, 256, [](S& s, ARAPersistentID& id) {
            s.value8b(id);
        });
    }
};

// ARA editor renderer properties
struct ARAEditorRendererProperties {
    std::string name;
    int32_t width = 800;
    int32_t height = 600;

    template <typename S>
    void serialize(S& s) {
        s.text1b(name, 256);
        s.value4b(width);
        s.value4b(height);
    }
};

// ARA editor view properties
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

// ARA playback renderer properties
struct ARAPlaybackRendererProperties {
    ARAPlaybackTransformationFlags flags = ARAPlaybackTransformationFlags::kARAPlaybackTransformationNone;
    double sample_rate = 44100.0;
    int32_t max_block_size = 512;
    bool offline = false;

    template <typename S>
    void serialize(S& s) {
        s.value4b(flags);
        s.value8b(sample_rate);
        s.value4b(max_block_size);
        s.value1b(offline);
    }
};

// Archive reader/writer host references (opaque)
using ARAArchiveReaderHostRef = uint64_t;
using ARAArchiveWriterHostRef = uint64_t;

// Archive callbacks
struct ARAArchiveCallback {
    enum class Type : int32_t {
        Progress = 0,
        Complete = 1,
        Error = 2
    };
    Type type = Type::Progress;
    double progress = 0.0;
    std::string message;

    template <typename S>
    void serialize(S& s) {
        s.value4b(type);
        s.value8b(progress);
        s.text1b(message, 256);
    }
};

// Analysis callback
struct ARAAnalysisCallback {
    ARAPersistentID audio_source_id = 0;
    bool success = true;
    std::string error_message;

    template <typename S>
    void serialize(S& s) {
        s.value8b(audio_source_id);
        s.value1b(success);
        s.text1b(error_message, 256);
    }
};

/**
 * Define all request structs for host -> plugin ARA control messages.
 * These must be defined before the variant that uses them.
 */

// Document controller operations
struct CreateDocumentController {
    template <typename S>
    void serialize(S&) {}
};
struct DestroyDocumentController {
    uint64_t controller_ref = 0;
    template <typename S>
    void serialize(S& s) { s.value8b(controller_ref); }
};
struct GetDocumentControllerProperties {
    uint64_t controller_ref = 0;
    template <typename S>
    void serialize(S& s) { s.value8b(controller_ref); }
};
struct SetDocumentControllerProperties {
    uint64_t controller_ref = 0;
    ARADocumentProperties properties;
    template <typename S>
    void serialize(S& s) { s.value8b(controller_ref); s.object(properties); }
};
struct NotifyModelUpdates {
    uint64_t controller_ref = 0;
    template <typename S>
    void serialize(S& s) { s.value8b(controller_ref); }
};
struct BeginEditing {
    uint64_t controller_ref = 0;
    template <typename S>
    void serialize(S& s) { s.value8b(controller_ref); }
};
struct EndEditing {
    uint64_t controller_ref = 0;
    template <typename S>
    void serialize(S& s) { s.value8b(controller_ref); }
};

// Musical context operations
struct CreateMusicalContext {
    uint64_t controller_ref = 0;
    std::string host_ref_id;
    ARAMusicalContextProperties properties;
    template <typename S>
    void serialize(S& s) {
        s.value8b(controller_ref);
        s.text1b(host_ref_id, 256);
        s.object(properties);
    }
};
struct UpdateMusicalContextProperties {
    uint64_t controller_ref = 0;
    uint64_t context_ref = 0;
    ARAMusicalContextProperties properties;
    template <typename S>
    void serialize(S& s) {
        s.value8b(controller_ref);
        s.value8b(context_ref);
        s.object(properties);
    }
};
struct UpdateMusicalContextContent {
    uint64_t controller_ref = 0;
    uint64_t context_ref = 0;
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
struct DestroyMusicalContext {
    uint64_t controller_ref = 0;
    uint64_t context_ref = 0;
    template <typename S>
    void serialize(S& s) {
        s.value8b(controller_ref);
        s.value8b(context_ref);
    }
};

// Audio source operations
struct CreateAudioSource {
    uint64_t controller_ref = 0;
    std::string host_ref_id;
    ARAAudioSourceProperties properties;
    template <typename S>
    void serialize(S& s) {
        s.value8b(controller_ref);
        s.text1b(host_ref_id, 256);
        s.object(properties);
    }
};
struct UpdateAudioSourceProperties {
    uint64_t controller_ref = 0;
    uint64_t source_ref = 0;
    ARAAudioSourceProperties properties;
    template <typename S>
    void serialize(S& s) {
        s.value8b(controller_ref);
        s.value8b(source_ref);
        s.object(properties);
    }
};
struct UpdateAudioSourceContent {
    uint64_t controller_ref = 0;
    uint64_t source_ref = 0;
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
struct EnableAudioSourceSamplesAccess {
    uint64_t controller_ref = 0;
    uint64_t source_ref = 0;
    bool enable = false;
    template <typename S>
    void serialize(S& s) {
        s.value8b(controller_ref);
        s.value8b(source_ref);
        s.value1b(enable);
    }
};
struct DeactivateAudioSourceForUndoHistory {
    uint64_t controller_ref = 0;
    uint64_t source_ref = 0;
    bool deactivate = false;
    template <typename S>
    void serialize(S& s) {
        s.value8b(controller_ref);
        s.value8b(source_ref);
        s.value1b(deactivate);
    }
};
struct DestroyAudioSource {
    uint64_t controller_ref = 0;
    uint64_t source_ref = 0;
    template <typename S>
    void serialize(S& s) {
        s.value8b(controller_ref);
        s.value8b(source_ref);
    }
};

// Audio modification operations
struct CreateAudioModification {
    uint64_t controller_ref = 0;
    uint64_t audio_source_ref = 0;
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
struct CloneAudioModification {
    uint64_t controller_ref = 0;
    uint64_t source_modification_ref = 0;
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
struct UpdateAudioModificationProperties {
    uint64_t controller_ref = 0;
    uint64_t modification_ref = 0;
    ARAAudioModificationProperties properties;
    template <typename S>
    void serialize(S& s) {
        s.value8b(controller_ref);
        s.value8b(modification_ref);
        s.object(properties);
    }
};
struct DeactivateAudioModificationForUndoHistory {
    uint64_t controller_ref = 0;
    uint64_t modification_ref = 0;
    bool deactivate = false;
    template <typename S>
    void serialize(S& s) {
        s.value8b(controller_ref);
        s.value8b(modification_ref);
        s.value1b(deactivate);
    }
};
struct DestroyAudioModification {
    uint64_t controller_ref = 0;
    uint64_t modification_ref = 0;
    template <typename S>
    void serialize(S& s) {
        s.value8b(controller_ref);
        s.value8b(modification_ref);
    }
};

// Playback region operations
struct CreatePlaybackRegion {
    uint64_t controller_ref = 0;
    uint64_t audio_modification_ref = 0;
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
struct UpdatePlaybackRegionProperties {
    uint64_t controller_ref = 0;
    uint64_t region_ref = 0;
    ARAPlaybackRegionProperties properties;
    template <typename S>
    void serialize(S& s) {
        s.value8b(controller_ref);
        s.value8b(region_ref);
        s.object(properties);
    }
};
struct DestroyPlaybackRegion {
    uint64_t controller_ref = 0;
    uint64_t region_ref = 0;
    template <typename S>
    void serialize(S& s) {
        s.value8b(controller_ref);
        s.value8b(region_ref);
    }
};

// Region sequence operations
struct CreateRegionSequence {
    uint64_t controller_ref = 0;
    std::string host_ref_id;
    ARARegionSequenceProperties properties;
    template <typename S>
    void serialize(S& s) {
        s.value8b(controller_ref);
        s.text1b(host_ref_id, 256);
        s.object(properties);
    }
};
struct GetRegionSequenceCount {
    uint64_t controller_ref = 0;
    template <typename S>
    void serialize(S& s) { s.value8b(controller_ref); }
};
struct GetRegionSequenceProperties {
    uint64_t controller_ref = 0;
    uint64_t sequence_ref = 0;
    template <typename S>
    void serialize(S& s) {
        s.value8b(controller_ref);
        s.value8b(sequence_ref);
    }
};
struct SetRegionSequenceProperties {
    uint64_t controller_ref = 0;
    uint64_t sequence_ref = 0;
    ARARegionSequenceProperties properties;
    template <typename S>
    void serialize(S& s) {
        s.value8b(controller_ref);
        s.value8b(sequence_ref);
        s.object(properties);
    }
};
struct DestroyRegionSequence {
    uint64_t controller_ref = 0;
    uint64_t sequence_ref = 0;
    template <typename S>
    void serialize(S& s) {
        s.value8b(controller_ref);
        s.value8b(sequence_ref);
    }
};

// Archive operations
struct StoreObjectsToArchive {
    uint64_t controller_ref = 0;
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
struct RestoreObjectsFromArchive {
    uint64_t controller_ref = 0;
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
struct GetArchiveIDs {
    template <typename S>
    void serialize(S&) {}
};

// Analysis operations
struct AnalyzeAudioSource {
    uint64_t controller_ref = 0;
    uint64_t audio_source_ref = 0;
    template <typename S>
    void serialize(S& s) {
        s.value8b(controller_ref);
        s.value8b(audio_source_ref);
    }
};

// Audio access operations
struct CreateAudioReader {
    template <typename S>
    void serialize(S&) {}
};
struct ReadAudioSamples {
    template <typename S>
    void serialize(S&) {}
};
struct DestroyAudioReader {
    template <typename S>
    void serialize(S&) {}
};

// Plug-in extension operations
struct BindToDocumentController {
    template <typename S>
    void serialize(S&) {}
};
struct BindToDocumentControllerWithRoles {
    template <typename S>
    void serialize(S&) {}
};

// Playback renderer operations
struct CreatePlaybackRenderer {
    template <typename S>
    void serialize(S&) {}
};
struct DestroyPlaybackRenderer {
    template <typename S>
    void serialize(S&) {}
};
struct GetPlaybackRendererProperties {
    template <typename S>
    void serialize(S&) {}
};
struct SetPlaybackRendererProperties {
    template <typename S>
    void serialize(S&) {}
};
struct RenderPlaybackRegion {
    template <typename S>
    void serialize(S&) {}
};

// Editor renderer operations
struct CreateEditorRenderer {
    template <typename S>
    void serialize(S&) {}
};
struct DestroyEditorRenderer {
    template <typename S>
    void serialize(S&) {}
};
struct RenderEditorRegion {
    template <typename S>
    void serialize(S&) {}
};

// Editor view operations
struct CreateEditorView {
    template <typename S>
    void serialize(S&) {}
};
struct DestroyEditorView {
    template <typename S>
    void serialize(S&) {}
};
struct NotifySelection {
    template <typename S>
    void serialize(S&) {}
};
struct NotifyVisibility {
    template <typename S>
    void serialize(S&) {}
};

// Audio access controller
struct GetAudioSourceHostRef {
    template <typename S>
    void serialize(S&) {}
};
struct GetAudioModificationHostRef {
    template <typename S>
    void serialize(S&) {}
};
struct GetPlaybackRegionHostRef {
    template <typename S>
    void serialize(S&) {}
};

// Archiving controller
struct GetArchiveReaderHostRef {
    template <typename S>
    void serialize(S&) {}
};
struct GetArchiveWriterHostRef {
    template <typename S>
    void serialize(S&) {}
};

// Content access controller
struct GetMusicalContextHostRef {
    template <typename S>
    void serialize(S&) {}
};

// Model update controller
struct GetModelUpdateControllerHostRef {
    template <typename S>
    void serialize(S&) {}
};

// Playback controller
struct GetPlaybackControllerHostRef {
    template <typename S>
    void serialize(S&) {}
};

// Content access
struct GetAudioSourceContent {
    template <typename S>
    void serialize(S&) {}
};
struct GetAudioModificationContent {
    template <typename S>
    void serialize(S&) {}
};
struct GetPlaybackRegionContent {
    template <typename S>
    void serialize(S&) {}
};
struct GetMusicalContextContent {
    template <typename S>
    void serialize(S&) {}
};

// Audio modification content reader
struct CreateContentReader {
    template <typename S>
    void serialize(S&) {}
};
struct ReadContent {
    template <typename S>
    void serialize(S&) {}
};
struct DestroyContentReader {
    template <typename S>
    void serialize(S&) {}
};

/**
 * Callback types for plugin -> host ARA messages.
 */
struct OnDocumentControllerChanged {
    template <typename S>
    void serialize(S&) {}
};
struct OnPlaybackRendererChanged {
    template <typename S>
    void serialize(S&) {}
};
struct OnAudioSourceChanged {
    template <typename S>
    void serialize(S&) {}
};
struct OnRegionSequenceChanged {
    template <typename S>
    void serialize(S&) {}
};
struct OnMusicalContextChanged {
    template <typename S>
    void serialize(S&) {}
};
struct OnAudioModificationChanged {
    template <typename S>
    void serialize(S&) {}
};
struct OnPlaybackRegionChanged {
    template <typename S>
    void serialize(S&) {}
};

struct OnArchiveProgress {
    template <typename S>
    void serialize(S&) {}
};
struct OnArchiveComplete {
    template <typename S>
    void serialize(S&) {}
};
struct OnArchiveError {
    template <typename S>
    void serialize(S&) {}
};

struct OnAnalysisComplete {
    template <typename S>
    void serialize(S&) {}
};

struct OnAudioAccessStarted {
    template <typename S>
    void serialize(S&) {}
};
struct OnAudioAccessEnded {
    template <typename S>
    void serialize(S&) {}
};

struct OnAudioSourceContentChanged {
    template <typename S>
    void serialize(S&) {}
};
struct OnAudioModificationContentChanged {
    template <typename S>
    void serialize(S&) {}
};
struct OnPlaybackRegionContentChanged {
    template <typename S>
    void serialize(S&) {}
};
struct OnMusicalContextContentChanged {
    template <typename S>
    void serialize(S&) {}
};

struct OnPlaybackRegionHeadAndTailTimeChanged {
    template <typename S>
    void serialize(S&) {}
};

struct OnModelUpdatesAvailable {
    template <typename S>
    void serialize(S&) {}
};

/**
 * Request types for host -> plugin ARA control messages (Document Controller).
 */
struct ARAControlRequest {
    ARAControlRequest() {}
    template <typename T>
    ARAControlRequest(T request) : payload(std::move(request)) {}

    using Payload = std::variant<
        // Document controller operations
        struct CreateDocumentController,
        struct DestroyDocumentController,
        struct GetDocumentControllerProperties,
        struct SetDocumentControllerProperties,
        struct NotifyModelUpdates,
        struct BeginEditing,
        struct EndEditing,
        
        // Musical context operations
        struct CreateMusicalContext,
        struct UpdateMusicalContextProperties,
        struct UpdateMusicalContextContent,
        struct DestroyMusicalContext,
        
        // Audio source operations
        struct CreateAudioSource,
        struct UpdateAudioSourceProperties,
        struct UpdateAudioSourceContent,
        struct EnableAudioSourceSamplesAccess,
        struct DeactivateAudioSourceForUndoHistory,
        struct DestroyAudioSource,
        
        // Audio modification operations
        struct CreateAudioModification,
        struct CloneAudioModification,
        struct UpdateAudioModificationProperties,
        struct DeactivateAudioModificationForUndoHistory,
        struct DestroyAudioModification,
        
        // Playback region operations
        struct CreatePlaybackRegion,
        struct UpdatePlaybackRegionProperties,
        struct DestroyPlaybackRegion,
        
        // Region sequence operations
        struct CreateRegionSequence,
        struct GetRegionSequenceCount,
        struct GetRegionSequenceProperties,
        struct SetRegionSequenceProperties,
        struct DestroyRegionSequence,
        
        // Archive operations
        struct StoreObjectsToArchive,
        struct RestoreObjectsFromArchive,
        struct GetArchiveIDs,
        
        // Analysis operations
        struct AnalyzeAudioSource,
        
        // Audio access operations
        struct CreateAudioReader,
        struct ReadAudioSamples,
        struct DestroyAudioReader,
        
        // Plug-in extension operations
        struct BindToDocumentController,
        struct BindToDocumentControllerWithRoles,
        
        // Playback renderer operations
        struct CreatePlaybackRenderer,
        struct DestroyPlaybackRenderer,
        struct GetPlaybackRendererProperties,
        struct SetPlaybackRendererProperties,
        struct RenderPlaybackRegion,
        
        // Editor renderer operations
        struct CreateEditorRenderer,
        struct DestroyEditorRenderer,
        struct RenderEditorRegion,
        
        // Editor view operations
        struct CreateEditorView,
        struct DestroyEditorView,
        struct NotifySelection,
        struct NotifyVisibility,
        
        // Audio access controller
        struct GetAudioSourceHostRef,
        struct GetAudioModificationHostRef,
        struct GetPlaybackRegionHostRef,
        
        // Archiving controller
        struct GetArchiveReaderHostRef,
        struct GetArchiveWriterHostRef,
        
        // Content access controller
        struct GetMusicalContextHostRef,
        
        // Model update controller
        struct GetModelUpdateControllerHostRef,
        
        // Playback controller
        struct GetPlaybackControllerHostRef,
        
        // Content access
        struct GetAudioSourceContent,
        struct GetAudioModificationContent,
        struct GetPlaybackRegionContent,
        struct GetMusicalContextContent,
        
        // Audio modification content reader
        struct CreateContentReader,
        struct ReadContent,
        struct DestroyContentReader
    >;

    Payload payload;

    template <typename S>
    void serialize(S& s) {
        s.ext(payload, bitsery::ext::InPlaceVariant{});
    }
};

/**
 * Callback types for plugin -> host ARA messages.
 */
struct ARACallbackRequest {
    ARACallbackRequest() {}
    template <typename T>
    ARACallbackRequest(T request) : payload(std::move(request)) {}

    using Payload = std::variant<
        // Document controller callbacks
        struct OnDocumentControllerChanged,
        struct OnPlaybackRendererChanged,
        struct OnAudioSourceChanged,
        struct OnRegionSequenceChanged,
        struct OnMusicalContextChanged,
        struct OnAudioModificationChanged,
        struct OnPlaybackRegionChanged,
        
        // Archive callbacks
        struct OnArchiveProgress,
        struct OnArchiveComplete,
        struct OnArchiveError,
        
        // Analysis callbacks
        struct OnAnalysisComplete,
        
        // Audio access callbacks
        struct OnAudioAccessStarted,
        struct OnAudioAccessEnded,
        
        // Content callbacks
        struct OnAudioSourceContentChanged,
        struct OnAudioModificationContentChanged,
        struct OnPlaybackRegionContentChanged,
        struct OnMusicalContextContentChanged,
        
        // Playback callbacks
        struct OnPlaybackRegionHeadAndTailTimeChanged,
        
        // Model update callbacks
        struct OnModelUpdatesAvailable
    >;

    Payload payload;

    template <typename S>
    void serialize(S& s) {
        s.ext(payload, bitsery::ext::InPlaceVariant{});
    }
};

/**
 * Response for getting ARA factory.
 */
struct ARAFactoryResponse {
    uint64_t factory_id = 0;
    uint64_t factory_vtable = 0;

    template <typename S>
    void serialize(S& s) {
        s.value8b(factory_id);
        s.value8b(factory_vtable);
    }
};

/**
 * Request to get the ARA factory from the plugin.
 */
struct GetARAFactoryRequest {
    using Response = ARAFactoryResponse;

    template <typename S>
    void serialize(S& s) {}
};

/**
 * Document controller response types.
 */
struct DocumentControllerRefResponse {
    uint64_t controller_ref = 0;

    template <typename S>
    void serialize(S& s) {
        s.value8b(controller_ref);
    }
};

struct DocumentControllerPropertiesResponse {
    ARADocumentProperties properties;
    bool success = false;

    template <typename S>
    void serialize(S& s) {
        s.object(properties);
        s.value1b(success);
    }
};

struct MusicalContextRefResponse {
    uint64_t context_ref = 0;

    template <typename S>
    void serialize(S& s) {
        s.value8b(context_ref);
    }
};

struct AudioSourceRefResponse {
    uint64_t source_ref = 0;

    template <typename S>
    void serialize(S& s) {
        s.value8b(source_ref);
    }
};

struct AudioModificationRefResponse {
    uint64_t modification_ref = 0;

    template <typename S>
    void serialize(S& s) {
        s.value8b(modification_ref);
    }
};

struct PlaybackRegionRefResponse {
    uint64_t region_ref = 0;

    template <typename S>
    void serialize(S& s) {
        s.value8b(region_ref);
    }
};

struct RegionSequenceRefResponse {
    uint64_t sequence_ref = 0;

    template <typename S>
    void serialize(S& s) {
        s.value8b(sequence_ref);
    }
};

struct PlaybackRendererRefResponse {
    uint64_t renderer_ref = 0;

    template <typename S>
    void serialize(S& s) {
        s.value8b(renderer_ref);
    }
};

struct EditorRendererRefResponse {
    uint64_t renderer_ref = 0;

    template <typename S>
    void serialize(S& s) {
        s.value8b(renderer_ref);
    }
};

struct EditorViewRefResponse {
    uint64_t view_ref = 0;

    template <typename S>
    void serialize(S& s) {
        s.value8b(view_ref);
    }
};

struct AudioReaderRefResponse {
    uint64_t reader_ref = 0;

    template <typename S>
    void serialize(S& s) {
        s.value8b(reader_ref);
    }
};

struct ContentReaderRefResponse {
    uint64_t reader_ref = 0;

    template <typename S>
    void serialize(S& s) {
        s.value8b(reader_ref);
    }
};

struct ArchiveIDsResponse {
    std::vector<std::string> archive_ids;

    template <typename S>
    void serialize(S& s) {
        s.container1b(archive_ids, 256, [](S& s, std::string& id) {
            s.text1b(id, 256);
        });
    }
};

struct AudioSamplesResponse {
    std::vector<float> samples;
    int32_t channel_count = 0;
    int64_t sample_count = 0;

    template <typename S>
    void serialize(S& s) {
        s.container1b(samples, 1000000, [](S& s, float& sample) {
            s.value4b(sample);
        });
        s.value4b(channel_count);
        s.value8b(sample_count);
    }
};

struct ContentReadResponse {
    std::vector<uint8_t> data;

    template <typename S>
    void serialize(S& s) {
        s.container1b(data, 1000000, [](S& s, uint8_t& byte) {
            s.value1b(byte);
        });
    }
};

struct PlaybackRegionRenderResponse {
    bool success = false;
    double head_time = 0.0;
    double tail_time = 0.0;

    template <typename S>
    void serialize(S& s) {
        s.value1b(success);
        s.value8b(head_time);
        s.value8b(tail_time);
    }
};

/**
 * ARA Plugin Factory Proxy - wraps around ARA::PlugIn::Factory for serialization
 */
class AraPluginFactoryProxy {
   public:
    struct ConstructArgs {
        ConstructArgs() noexcept;
        
        // Read from an existing ARA factory implementation
        ConstructArgs(void* ara_factory);

        bool supports_ara_factory = false;
        std::string ara_factory_id;
        uint64_t ara_factory_vtable = 0;
        
        struct FactoryInfo {
            uint64_t factory_id = 0;
            uint64_t version = 0;
            uint64_t vendor_id = 0;
            std::string vendor_name;
            std::string product_name;
            std::string version_string;
        };
        std::optional<FactoryInfo> factory_info;

        template <typename S>
        void serialize(S& s) {
            s.value1b(supports_ara_factory);
            s.text1b(ara_factory_id, 256);
            s.value8b(ara_factory_vtable);
            s.ext(factory_info, bitsery::ext::InPlaceOptional{});
        }
    };

    struct Construct {
        using Response = ConstructArgs;

        template <typename S>
        void serialize(S&) {}
    };

    AraPluginFactoryProxy(ConstructArgs&& args) noexcept;
    virtual ~AraPluginFactoryProxy() noexcept = default;

    virtual const void* get_factory(const char* factory_id) = 0;

   protected:
    ConstructArgs arguments_;
};