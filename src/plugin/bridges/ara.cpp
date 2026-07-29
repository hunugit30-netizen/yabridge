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

#include <iostream>
#include <filesystem>

#include "ara-impls/plugin-factory-proxy.h"

namespace fs = ghc::filesystem;

AraPluginBridge::AraPluginBridge(const fs::path& plugin_path)
    : PluginBridge<ARASockets<std::jthread>>(
          PluginType::ara,
          plugin_path,
          [&](asio::io_context& io_context, const PluginInfo& info) {
              return ARASockets<std::jthread>(io_context,
                                              info.endpoint_base_dir_,
                                              /*listen=*/true);
          }) {
    log_init_message();
    connect_sockets_guarded();
    warn_on_version_mismatch(sockets_.host_version_.get());
    
    // Initialize the ARA plugin factory proxy by querying the Wine host
    // The Wine host will load the Windows VST3/ARA plugin and query its ARA factory
    AraPluginFactoryProxy::ConstructArgs factory_args =
        sockets_.plugin_host_control_.send_message(
            AraPluginFactoryProxy::Construct{},
            std::pair<ARALogger&, bool>(logger_, true));
    
    plugin_factory_ = std::make_unique<AraPluginFactoryProxyImpl>(*this, std::move(factory_args));
}

AraPluginBridge::~AraPluginBridge() noexcept = default;

const void* AraPluginBridge::get_factory(const char* factory_id) {
    // Delegate to the plugin factory proxy
    if (plugin_factory_) {
        return plugin_factory_->get_factory(factory_id);
    }
    
    return nullptr;
}

AraDocumentControllerProxyImpl* AraPluginBridge::get_or_create_document_controller(uint64_t ref) {
    if (ref == 0) {
        return nullptr;
    }
    
    std::shared_lock read_lock(document_controllers_mutex_);
    auto it = document_controllers_.find(ref);
    if (it != document_controllers_.end()) {
        return it->second.get();
    }
    read_lock.unlock();
    
    // Create new controller
    std::unique_lock write_lock(document_controllers_mutex_);
    // Double-check after acquiring write lock
    it = document_controllers_.find(ref);
    if (it != document_controllers_.end()) {
        return it->second.get();
    }
    
    auto controller = std::make_unique<AraDocumentControllerProxyImpl>(*this, ref);
    AraDocumentControllerProxyImpl* controller_ptr = controller.get();
    document_controllers_.emplace(ref, std::move(controller));
    return controller_ptr;
}

void AraPluginBridge::remove_document_controller(uint64_t ref) {
    if (ref == 0) {
        return;
    }
    
    std::unique_lock lock(document_controllers_mutex_);
    document_controllers_.erase(ref);
}

AraDocumentControllerProxyImpl::AraDocumentControllerProxyImpl(AraPluginBridge& bridge, Ref ref)
    : bridge_(bridge), ref_(ref) {}

MusicalContextRefResponse AraDocumentControllerProxyImpl::create_musical_context(
    const AraDocumentControllerProxy::CreateMusicalContextRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

ARANullResponse AraDocumentControllerProxyImpl::update_musical_context_properties(
    const AraDocumentControllerProxy::UpdateMusicalContextPropertiesRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

ARANullResponse AraDocumentControllerProxyImpl::update_musical_context_content(
    const AraDocumentControllerProxy::UpdateMusicalContextContentRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

ARANullResponse AraDocumentControllerProxyImpl::destroy_musical_context(
    const AraDocumentControllerProxy::DestroyMusicalContextRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

AudioSourceRefResponse AraDocumentControllerProxyImpl::create_audio_source(
    const AraDocumentControllerProxy::CreateAudioSourceRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

ARANullResponse AraDocumentControllerProxyImpl::update_audio_source_properties(
    const AraDocumentControllerProxy::UpdateAudioSourcePropertiesRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

ARANullResponse AraDocumentControllerProxyImpl::update_audio_source_content(
    const AraDocumentControllerProxy::UpdateAudioSourceContentRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

ARANullResponse AraDocumentControllerProxyImpl::enable_audio_source_samples_access(
    const AraDocumentControllerProxy::EnableAudioSourceSamplesAccessRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

ARANullResponse AraDocumentControllerProxyImpl::deactivate_audio_source_for_undo_history(
    const AraDocumentControllerProxy::DeactivateAudioSourceForUndoHistoryRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

ARANullResponse AraDocumentControllerProxyImpl::destroy_audio_source(
    const AraDocumentControllerProxy::DestroyAudioSourceRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

AudioModificationRefResponse AraDocumentControllerProxyImpl::create_audio_modification(
    const AraDocumentControllerProxy::CreateAudioModificationRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

AudioModificationRefResponse AraDocumentControllerProxyImpl::clone_audio_modification(
    const AraDocumentControllerProxy::CloneAudioModificationRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

ARANullResponse AraDocumentControllerProxyImpl::update_audio_modification_properties(
    const AraDocumentControllerProxy::UpdateAudioModificationPropertiesRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

ARANullResponse AraDocumentControllerProxyImpl::deactivate_audio_modification_for_undo_history(
    const AraDocumentControllerProxy::DeactivateAudioModificationForUndoHistoryRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

ARANullResponse AraDocumentControllerProxyImpl::destroy_audio_modification(
    const AraDocumentControllerProxy::DestroyAudioModificationRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

PlaybackRegionRefResponse AraDocumentControllerProxyImpl::create_playback_region(
    const AraDocumentControllerProxy::CreatePlaybackRegionRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

ARANullResponse AraDocumentControllerProxyImpl::update_playback_region_properties(
    const AraDocumentControllerProxy::UpdatePlaybackRegionPropertiesRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

ARANullResponse AraDocumentControllerProxyImpl::destroy_playback_region(
    const AraDocumentControllerProxy::DestroyPlaybackRegionRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

RegionSequenceRefResponse AraDocumentControllerProxyImpl::create_region_sequence(
    const AraDocumentControllerProxy::CreateRegionSequenceRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

CountResponse AraDocumentControllerProxyImpl::get_region_sequence_count(
    const AraDocumentControllerProxy::GetRegionSequenceCountRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

RegionSequencePropertiesResponse AraDocumentControllerProxyImpl::get_region_sequence_properties(
    const AraDocumentControllerProxy::GetRegionSequencePropertiesRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

ARANullResponse AraDocumentControllerProxyImpl::set_region_sequence_properties(
    const AraDocumentControllerProxy::SetRegionSequencePropertiesRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

ARANullResponse AraDocumentControllerProxyImpl::destroy_region_sequence(
    const AraDocumentControllerProxy::DestroyRegionSequenceRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

ARANullResponse AraDocumentControllerProxyImpl::store_objects_to_archive(
    const AraDocumentControllerProxy::StoreObjectsToArchiveRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

ARANullResponse AraDocumentControllerProxyImpl::restore_objects_from_archive(
    const AraDocumentControllerProxy::RestoreObjectsFromArchiveRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

ARANullResponse AraDocumentControllerProxyImpl::analyze_audio_source(
    const AraDocumentControllerProxy::AnalyzeAudioSourceRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

ARANullResponse AraDocumentControllerProxyImpl::begin_editing(
    const AraDocumentControllerProxy::BeginEditingRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

ARANullResponse AraDocumentControllerProxyImpl::end_editing(
    const AraDocumentControllerProxy::EndEditingRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

ARANullResponse AraDocumentControllerProxyImpl::notify_model_updates(
    const AraDocumentControllerProxy::NotifyModelUpdatesRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}

ARANullResponse AraDocumentControllerProxyImpl::update_document_properties(
    const AraDocumentControllerProxy::UpdateDocumentPropertiesRequest& request) {
    auto req = request;
    req.controller_ref = ref_;
    return send_control_message(req);
}