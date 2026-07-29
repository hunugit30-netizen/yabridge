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
#include <thread>
#include <map>
#include <memory>
#include <string>

#include "../../common/communication/ara.h"
#include "../../common/logging/ara.h"
#include "../../common/mutual-recursion.h"
#include "../../common/serialization/ara.h"
#include "common.h"

/**
 * This handles the communication between the native host and an ARA plugin
 * hosted in our Wine plugin host. ARA plugins are VST3 plugins that implement
 * additional ARA interfaces. The ARA bridge works similarly to the VST3 bridge,
 * but adds support for the ARA-specific interfaces.
 *
 * The naming scheme of all of these 'bridge' classes is `<type>{,Plugin}Bridge`
 * for greppability reasons. The `Plugin` infix is added on the native plugin
 * side.
 */
class AraDocumentControllerProxyImpl;

class AraPluginBridge : public PluginBridge<ARASockets<std::jthread>> {
   public:
    /**
     * Initializes the ARA module by starting and setting up communicating with
     * the Wine plugin host.
     *
     * @param plugin_path The path to the **native** plugin library `.so` file.
     *   This is used to determine the path to the Windows plugin library we
     *   should load. For directly loaded bridges this should be
     *   `get_this_file_location()`. Chainloaded plugins should use the path of
     *   the chainloader copy instead.
     *
     * @throw std::runtime_error Thrown when the Wine plugin host could not be
     *   found, or if it could not locate and load an ARA module.
     */
    explicit AraPluginBridge(const ghc::filesystem::path& plugin_path);

    /**
     * Terminate the Wine plugin host process and drop all work when the module
     * gets unloaded.
     */
    ~AraPluginBridge() noexcept override;

    /**
     * The implementation for `GetPluginFactory`. When this is first called,
     * we'll query the factory's contents from the Wine plugin hosts if the
     * queried factory type is supported.
     *
     * @see plugin_factory_
     */
    const void* get_factory(const char* factory_id);

    /**
     * Send a control message to the Wine plugin host and return the response.
     * This is a shorthand for `sockets_.plugin_host_control_.send_message()`
     * for use in ARA interface implementations.
     */
    template <typename T>
    typename T::Response send_message(const T& object) {
        return sockets_.plugin_host_control_.send_message(
            object, std::pair<ARALogger&, bool>(logger_, true));
    }

    /**
     * Get or create a document controller proxy.
     */
    AraDocumentControllerProxyImpl* get_or_create_document_controller(uint64_t ref);

    /**
     * Remove a document controller proxy.
     */
    void remove_document_controller(uint64_t ref);

    /**
     * The logging facility used for this instance of yabridge. Wraps around
     * `PluginBridge::generic_logger`.
     */
    ARALogger logger_;

   private:
    /**
     * Handles callbacks from the plugin to the host over the
     * `host_plugin_callback_` sockets.
     */
    std::jthread host_callback_handler_;

    /**
     * Our plugin factory, containing information about all plugins supported by
     * the bridged ARA plugin's factory. This is initialized the first time the
     * host tries to query this in `GetPluginFactory()`.
     *
     * @related get_factory
     */
    std::unique_ptr<AraPluginFactoryProxyImpl> plugin_factory_;

    /**
     * Document controller proxies, keyed by controller reference.
     */
    std::map<uint64_t, std::unique_ptr<AraDocumentControllerProxyImpl>>
        document_controllers_;
    std::shared_mutex document_controllers_mutex_;

    /**
     * Used in `send_mutually_recursive_message()` to be able to execute
     * functions from that same calling thread while we're waiting for a
     * response. See the uses for `send_mutually_recursive_message()` for use
     * cases where this is needed.
     */
    MutualRecursionHelper<std::jthread> mutual_recursion_;
};

/**
 * DocumentController Proxy on the plugin side.
 * Forwards ARA DocumentController calls to the Wine host.
 */
class AraDocumentControllerProxyImpl {
   public:
    using Ref = uint64_t;

    AraDocumentControllerProxyImpl(AraPluginBridge& bridge, Ref ref);
    ~AraDocumentControllerProxyImpl() noexcept = default;

    // Document controller operations
    template <typename T>
    typename T::Response send_control_message(const T& request) {
        return bridge_.send_message(request);
    }

    // Musical context operations
    MusicalContextRefResponse create_musical_context(const AraDocumentControllerProxy::CreateMusicalContextRequest& request);
    ARANullResponse update_musical_context_properties(const AraDocumentControllerProxy::UpdateMusicalContextPropertiesRequest& request);
    ARANullResponse update_musical_context_content(const AraDocumentControllerProxy::UpdateMusicalContextContentRequest& request);
    ARANullResponse destroy_musical_context(const AraDocumentControllerProxy::DestroyMusicalContextRequest& request);

    // Audio source operations
    AudioSourceRefResponse create_audio_source(const AraDocumentControllerProxy::CreateAudioSourceRequest& request);
    ARANullResponse update_audio_source_properties(const AraDocumentControllerProxy::UpdateAudioSourcePropertiesRequest& request);
    ARANullResponse update_audio_source_content(const AraDocumentControllerProxy::UpdateAudioSourceContentRequest& request);
    ARANullResponse enable_audio_source_samples_access(const AraDocumentControllerProxy::EnableAudioSourceSamplesAccessRequest& request);
    ARANullResponse deactivate_audio_source_for_undo_history(const AraDocumentControllerProxy::DeactivateAudioSourceForUndoHistoryRequest& request);
    ARANullResponse destroy_audio_source(const AraDocumentControllerProxy::DestroyAudioSourceRequest& request);

    // Audio modification operations
    AudioModificationRefResponse create_audio_modification(const AraDocumentControllerProxy::CreateAudioModificationRequest& request);
    AudioModificationRefResponse clone_audio_modification(const AraDocumentControllerProxy::CloneAudioModificationRequest& request);
    ARANullResponse update_audio_modification_properties(const AraDocumentControllerProxy::UpdateAudioModificationPropertiesRequest& request);
    ARANullResponse deactivate_audio_modification_for_undo_history(const AraDocumentControllerProxy::DeactivateAudioModificationForUndoHistoryRequest& request);
    ARANullResponse destroy_audio_modification(const AraDocumentControllerProxy::DestroyAudioModificationRequest& request);

    // Playback region operations
    PlaybackRegionRefResponse create_playback_region(const AraDocumentControllerProxy::CreatePlaybackRegionRequest& request);
    ARANullResponse update_playback_region_properties(const AraDocumentControllerProxy::UpdatePlaybackRegionPropertiesRequest& request);
    ARANullResponse destroy_playback_region(const AraDocumentControllerProxy::DestroyPlaybackRegionRequest& request);

    // Region sequence operations
    RegionSequenceRefResponse create_region_sequence(const AraDocumentControllerProxy::CreateRegionSequenceRequest& request);
    CountResponse get_region_sequence_count(const AraDocumentControllerProxy::GetRegionSequenceCountRequest& request);
    RegionSequencePropertiesResponse get_region_sequence_properties(const AraDocumentControllerProxy::GetRegionSequencePropertiesRequest& request);
    ARANullResponse set_region_sequence_properties(const AraDocumentControllerProxy::SetRegionSequencePropertiesRequest& request);
    ARANullResponse destroy_region_sequence(const AraDocumentControllerProxy::DestroyRegionSequenceRequest& request);

    // Archive operations
    ARANullResponse store_objects_to_archive(const AraDocumentControllerProxy::StoreObjectsToArchiveRequest& request);
    ARANullResponse restore_objects_from_archive(const AraDocumentControllerProxy::RestoreObjectsFromArchiveRequest& request);

    // Analysis operations
    ARANullResponse analyze_audio_source(const AraDocumentControllerProxy::AnalyzeAudioSourceRequest& request);

    // Editing operations
    ARANullResponse begin_editing(const AraDocumentControllerProxy::BeginEditingRequest& request);
    ARANullResponse end_editing(const AraDocumentControllerProxy::EndEditingRequest& request);
    ARANullResponse notify_model_updates(const AraDocumentControllerProxy::NotifyModelUpdatesRequest& request);
    ARANullResponse update_document_properties(const AraDocumentControllerProxy::UpdateDocumentPropertiesRequest& request);

   private:
    AraPluginBridge& bridge_;
    Ref ref_;
};

