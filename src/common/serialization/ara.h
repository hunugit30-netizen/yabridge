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

#include "../bitsery/ext/in-place-variant.h"
#include "../utils.h"
#include "common.h"

/**
 * ARA (Audio Random Access) serialization structures.
 * 
 * These are placeholder/stub definitions for the ARA API. The ARA SDK provides
 * the actual interface definitions (ARADocumentController, ARAPlaybackRenderer,
 * etc.). This file defines the message types used for IPC between the native
 * plugin and the Wine host.
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
 * Request types for host -> plugin ARA control messages.
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
        
        // Playback renderer operations
        struct CreatePlaybackRenderer,
        struct DestroyPlaybackRenderer,
        struct GetPlaybackRendererProperties,
        struct SetPlaybackRendererProperties,
        
        // Audio source operations
        struct GetAudioSourceCount,
        struct GetAudioSourceProperties,
        struct GetAudioSourceSamples,
        
        // Region sequence operations
        struct GetRegionSequenceCount,
        struct GetRegionSequenceProperties,
        struct SetRegionSequenceProperties,
        
        // Archive operations
        struct ArchiveDocument,
        struct RestoreDocument
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
        
        // Archive callbacks
        struct OnArchiveProgress,
        struct OnArchiveComplete,
        
        // Analysis callbacks
        struct OnAnalysisComplete
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
    // The factory is identified by its CLSID/IID
    uint64_t factory_id = 0;
    uint64_t factory_vtable = 0;  // Serialized vtable pointer (opaque)

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
    void serialize(S& s) {
        // No payload needed
    }
};