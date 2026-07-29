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

#include <windows.h>
#include <string>
#include <iostream>
#include <vector>

#include "vst3-impls/component-handler-proxy.h"
#include "vst3-impls/connection-point-proxy.h"
#include "vst3-impls/context-menu-proxy.h"
#include "vst3-impls/host-context-proxy.h"
#include "vst3-impls/plug-frame-proxy.h"
#include "../../common/serialization/ara.h"

// Generated inside of the build directory
#include <version.h>

// NOLINTNEXTLINE(bugprone-suspicious-include)
#include <public.sdk/source/vst/hosting/module_win32.cpp>

/**
 * This is a workaround for Bluecat Audio plugins that don't expose their
 * `IPluginBase` interface through the query interface. Even though every plugin
 * object _must_ support `IPlugBase`, these plugins only expose those functions
 * through `IComponent` (which derives from `IPluginBase`). So if we do
 * encounter one of those plugins, then we'll just have to coerce an
 * `IComponent` pointer into an `IPluginBase` smart pointer. This way we can
 * keep the rest of yabridge's design in tact.
 */
Steinberg::FUnknownPtr<Steinberg::IPluginBase> hack_init_plugin_base(
    Steinberg::IPtr<Steinberg::FUnknown> object,
    Steinberg::IPtr<Steinberg::Vst::IComponent> component);

ARABridge::ARABridge(MainContext& main_context,
                     std::string plugin_dll_path,
                     std::string endpoint_base_dir,
                     pid_t parent_pid)
    : HostBridge(main_context, std::move(plugin_dll_path), parent_pid),
      logger_(generic_logger_),
      sockets_(main_context_.context_, endpoint_base_dir, false),
      current_instance_id_(0),
      next_controller_ref_(1),
      plugin_factory_(nullptr, [](Steinberg::IPluginFactory* f) { if (f) f->release(); }) {
    std::string error;
    // Load the Windows VST3 plugin using Wine
    module_ = VST3::Hosting::Win32Module::create(plugin_dll_path_, error);
    if (!module_) {
        throw std::runtime_error("Could not load the VST3/ARA module for '" +
                                 plugin_dll_path_ + "': " + error);
    }

    // Connect to the native plugin
    sockets_.connect();

    // Fetch this instance's configuration from the plugin to finish the setup
    // process
    config_ = sockets_.plugin_host_callback_.send_message(
        WantsConfiguration{.host_version = yabridge_git_version}, std::nullopt);

    // Allow this plugin to configure the main context's tick rate
    main_context_.update_timer_interval(config_.event_loop_interval());

    // Try to get the ARA factory from the VST3 plugin
    initialize_ara_factory();

    // Create the ARA factory proxy for serialization
    ara_factory_proxy_ = std::make_unique<AraFactoryProxyImpl>();
#ifdef WITH_ARA
    if (ara_factory_) {
        *ara_factory_proxy_ = AraFactoryProxyImpl(ara_factory_);
    }
#else
    if (ara_factory_raw_) {
        *ara_factory_proxy_ = AraFactoryProxyImpl();
    }
#endif
}

ARABridge::~ARABridge() noexcept {
    try {
        // Drop all work make sure all sockets are closed
        plugin_host_->terminate();
        main_context_.stop();
    } catch (const std::system_error&) {
        // It could be that the sockets have already been closed or that the
        // process has already exited (at which point we probably won't be
        // executing this, but maybe if all the stars align)
    }
}

bool ARABridge::inhibits_event_loop() noexcept {
    std::shared_lock lock(object_instances_mutex_);

    for (const auto& [instance_id, instance] : object_instances_) {
        if (!instance.is_initialized) {
            return true;
        }
    }

    return false;
}

void ARABridge::run() {
    set_realtime_priority(true);

    sockets_.host_plugin_control_.receive_messages(
        std::nullopt,
        overload{
            [&](const AraPluginFactoryProxy::Construct&)
                -> AraPluginFactoryProxy::Construct::Response {
                // Return serialized ARA factory info
                return ara_factory_proxy_->serialize_factory_info();
            },
            [&](const Vst3PluginFactoryProxy::Construct&)
                -> Vst3PluginFactoryProxy::Construct::Response {
                return Vst3PluginFactoryProxy::ConstructArgs(
                    module_->getFactory().get());
            },
            [&](const Vst3PlugViewProxy::Destruct& request)
                -> Vst3PlugViewProxy::Destruct::Response {
                main_context_
                    .run_in_context([&]() -> void {
                        // When the pointer gets dropped by the host, we want to
                        // drop it here as well, along with the `IPlugFrame`
                        // proxy object it may have received in
                        // `IPlugView::setFrame()`.
                        const auto& [instance, _] =
                            get_instance(request.owner_instance_id);

                        instance.plug_view_instance.reset();
                        instance.plug_frame_proxy.reset();
                    })
                    .wait();

                return Ack{};
            },
            [&](const Vst3PluginProxy::Construct& request)
                -> Vst3PluginProxy::Construct::Response {
                Steinberg::TUID cid;

                ArrayUID wine_cid = request.cid.get_wine_uid();
                std::copy(wine_cid.begin(), wine_cid.end(), cid);

                // Even though we're requesting a specific interface (to mimic
                // what the host is doing), we're immediately upcasting it to an
                // `FUnknown` so we can create a perfect proxy object.
                // We create the object from the GUI thread in case it
                // immediatly starts timers or something (even though it
                // shouldn't)
                Steinberg::IPtr<Steinberg::FUnknown> object =
                    main_context_
                        .run_in_context(
                            [&]() -> Steinberg::IPtr<Steinberg::FUnknown> {
                                Steinberg::IPtr<Steinberg::FUnknown> result;

                                // The plugin may spawn audio worker threads
                                // when constructing an object. Since Wine
                                // doesn't implement Window's realtime process
                                // priority yet we'll just have to make sure the
                                // any spawned threads are running with
                                // `SCHED_FIFO` ourselves.
                                set_realtime_priority(true);
                                switch (request.requested_interface) {
                                    case Vst3PluginProxy::Construct::Interface::
                                        IComponent:
                                        result =
                                            module_->getFactory()
                                                .createInstance<
                                                    Steinberg::Vst::IComponent>(
                                                    cid);
                                        break;
                                    case Vst3PluginProxy::Construct::Interface::
                                        IEditController:
                                        result =
                                            module_->getFactory()
                                                .createInstance<
                                                    Steinberg::Vst::
                                                        IEditController>(cid);
                                        break;
                                    default:
                                        // Unreachable
                                        result = nullptr;
                                }

                                return result;
                            })
                        .wait();

                if (!object) {
                    return Vst3PluginProxy::ConstructArgs{};
                }

                // Create the instance with the object
                PluginInstance instance;
                instance.object = object;
                instance.component = object;
                instance.audio_processor = object;
                const size_t instance_id = current_instance_id_.fetch_add(1);
                instance.object->addRef();  // We'll manage the reference count

                // Check if this component supports ARA
                bool supports_ara = false;
                if (instance.component) {
                    // Query for IPlugInEntryPoint to check for ARA support
                    Steinberg::FUnknownPtr<Steinberg::Vst::IPlugInEntryPoint>
                        entry_point(instance.component);
                    if (entry_point) {
                        supports_ara = true;
                    }
                }
                instance.supports_ara = supports_ara;
                instance.is_initialized = true;

                // Store the instance
                {
                    std::unique_lock lock(object_instances_mutex_);
                    object_instances_.emplace(
                        instance_id,
                        std::move(instance));
                }

                // Add audio processor sockets if needed
                if (instance.component || instance.audio_processor) {
                    sockets_.add_audio_processor_and_connect(instance_id);
                }

                return Vst3PluginProxy::ConstructArgs{
                    .instance_id = instance_id,
                    .supports_ara = supports_ara};
            },
            // ARA DocumentController messages
            [&](const AraDocumentControllerProxy::CreateMusicalContextRequest& request)
                -> MusicalContextRefResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (!controller) {
                    return MusicalContextRefResponse{0};
                }
                return controller->create_musical_context(request);
            },
            [&](const AraDocumentControllerProxy::UpdateMusicalContextPropertiesRequest& request)
                -> ARANullResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (controller) {
                    controller->update_musical_context_properties(request);
                }
                return ARANullResponse{};
            },
            [&](const AraDocumentControllerProxy::UpdateMusicalContextContentRequest& request)
                -> ARANullResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (controller) {
                    controller->update_musical_context_content(request);
                }
                return ARANullResponse{};
            },
            [&](const AraDocumentControllerProxy::DestroyMusicalContextRequest& request)
                -> ARANullResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (controller) {
                    controller->destroy_musical_context(request);
                }
                return ARANullResponse{};
            },
            [&](const AraDocumentControllerProxy::CreateAudioSourceRequest& request)
                -> AudioSourceRefResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (!controller) {
                    return AudioSourceRefResponse{0};
                }
                return controller->create_audio_source(request);
            },
            [&](const AraDocumentControllerProxy::UpdateAudioSourcePropertiesRequest& request)
                -> ARANullResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (controller) {
                    controller->update_audio_source_properties(request);
                }
                return ARANullResponse{};
            },
            [&](const AraDocumentControllerProxy::UpdateAudioSourceContentRequest& request)
                -> ARANullResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (controller) {
                    controller->update_audio_source_content(request);
                }
                return ARANullResponse{};
            },
            [&](const AraDocumentControllerProxy::EnableAudioSourceSamplesAccessRequest& request)
                -> ARANullResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (controller) {
                    controller->enable_audio_source_samples_access(request);
                }
                return ARANullResponse{};
            },
            [&](const AraDocumentControllerProxy::DeactivateAudioSourceForUndoHistoryRequest& request)
                -> ARANullResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (controller) {
                    controller->deactivate_audio_source_for_undo_history(request);
                }
                return ARANullResponse{};
            },
            [&](const AraDocumentControllerProxy::DestroyAudioSourceRequest& request)
                -> ARANullResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (controller) {
                    controller->destroy_audio_source(request);
                }
                return ARANullResponse{};
            },
            [&](const AraDocumentControllerProxy::CreateAudioModificationRequest& request)
                -> AudioModificationRefResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (!controller) {
                    return AudioModificationRefResponse{0};
                }
                return controller->create_audio_modification(request);
            },
            [&](const AraDocumentControllerProxy::CloneAudioModificationRequest& request)
                -> AudioModificationRefResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (!controller) {
                    return AudioModificationRefResponse{0};
                }
                return controller->clone_audio_modification(request);
            },
            [&](const AraDocumentControllerProxy::UpdateAudioModificationPropertiesRequest& request)
                -> ARANullResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (controller) {
                    controller->update_audio_modification_properties(request);
                }
                return ARANullResponse{};
            },
            [&](const AraDocumentControllerProxy::DeactivateAudioModificationForUndoHistoryRequest& request)
                -> ARANullResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (controller) {
                    controller->deactivate_audio_modification_for_undo_history(request);
                }
                return ARANullResponse{};
            },
            [&](const AraDocumentControllerProxy::DestroyAudioModificationRequest& request)
                -> ARANullResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (controller) {
                    controller->destroy_audio_modification(request);
                }
                return ARANullResponse{};
            },
            [&](const AraDocumentControllerProxy::CreatePlaybackRegionRequest& request)
                -> PlaybackRegionRefResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (!controller) {
                    return PlaybackRegionRefResponse{0};
                }
                return controller->create_playback_region(request);
            },
            [&](const AraDocumentControllerProxy::UpdatePlaybackRegionPropertiesRequest& request)
                -> ARANullResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (controller) {
                    controller->update_playback_region_properties(request);
                }
                return ARANullResponse{};
            },
            [&](const AraDocumentControllerProxy::DestroyPlaybackRegionRequest& request)
                -> ARANullResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (controller) {
                    controller->destroy_playback_region(request);
                }
                return ARANullResponse{};
            },
            [&](const AraDocumentControllerProxy::CreateRegionSequenceRequest& request)
                -> RegionSequenceRefResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (!controller) {
                    return RegionSequenceRefResponse{0};
                }
                return controller->create_region_sequence(request);
            },
            [&](const AraDocumentControllerProxy::GetRegionSequenceCountRequest& request)
                -> CountResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (!controller) {
                    return CountResponse{0};
                }
                return controller->get_region_sequence_count(request);
            },
            [&](const AraDocumentControllerProxy::GetRegionSequencePropertiesRequest& request)
                -> RegionSequencePropertiesResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (!controller) {
                    return RegionSequencePropertiesResponse{};
                }
                return controller->get_region_sequence_properties(request);
            },
            [&](const AraDocumentControllerProxy::SetRegionSequencePropertiesRequest& request)
                -> ARANullResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (controller) {
                    controller->set_region_sequence_properties(request);
                }
                return ARANullResponse{};
            },
            [&](const AraDocumentControllerProxy::DestroyRegionSequenceRequest& request)
                -> ARANullResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (controller) {
                    controller->destroy_region_sequence(request);
                }
                return ARANullResponse{};
            },
            [&](const AraDocumentControllerProxy::StoreObjectsToArchiveRequest& request)
                -> ARANullResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (controller) {
                    controller->store_objects_to_archive(request);
                }
                return ARANullResponse{};
            },
            [&](const AraDocumentControllerProxy::RestoreObjectsFromArchiveRequest& request)
                -> ARANullResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (controller) {
                    controller->restore_objects_from_archive(request);
                }
                return ARANullResponse{};
            },
            [&](const AraDocumentControllerProxy::AnalyzeAudioSourceRequest& request)
                -> ARANullResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (controller) {
                    controller->analyze_audio_source(request);
                }
                return ARANullResponse{};
            },
            [&](const AraDocumentControllerProxy::BeginEditingRequest& request)
                -> ARANullResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (controller) {
                    controller->begin_editing(request);
                }
                return ARANullResponse{};
            },
            [&](const AraDocumentControllerProxy::EndEditingRequest& request)
                -> ARANullResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (controller) {
                    controller->end_editing(request);
                }
                return ARANullResponse{};
            },
            [&](const AraDocumentControllerProxy::NotifyModelUpdatesRequest& request)
                -> ARANullResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (controller) {
                    controller->notify_model_updates(request);
                }
                return ARANullResponse{};
            },
            [&](const AraDocumentControllerProxy::UpdateDocumentPropertiesRequest& request)
                -> ARANullResponse {
                auto* controller = get_or_create_document_controller(request.controller_ref);
                if (controller) {
                    controller->update_document_properties(request);
                }
                return ARANullResponse{};
            },
        });
}

void ARABridge::close_sockets() {
    sockets_.close();
}

void ARABridge::initialize_ara_factory() {
    // Try to get the ARA factory from the VST3 plugin
    // First, we need to create a component instance and query for
    // IPlugInEntryPoint
    try {
        // Get the first class info from the factory
        Steinberg::PClassInfo class_info;
        if (module_->getFactory()->countClasses() > 0) {
            tresult result = module_->getFactory()->getClassInfo(0, &class_info);
            if (result == Steinberg::kResultOk) {
                // Create a component instance
                Steinberg::IPtr<Steinberg::Vst::IComponent> component =
                    module_->getFactory().createInstance<Steinberg::Vst::IComponent>(
                        class_info.cid);

                if (component) {
                    // Query for IPlugInEntryPoint which provides access to ARA factory
                    Steinberg::FUnknownPtr<Steinberg::Vst::IPlugInEntryPoint>
                        entry_point(component);
                    
                    if (entry_point) {
#ifdef WITH_ARA
                        ara_factory_ = entry_point->getARAFactory();
#else
                        // Store the raw pointer for the response
                        ara_factory_raw_ = entry_point->getARAFactory();
#endif
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        logger_.log_error([&](auto& log) {
            log << "Failed to initialize ARA factory: " << e.what();
        });
    }
}

#ifdef WITH_ARA
ARA::PlugIn::Factory* ARABridge::get_ara_factory() {
    return ara_factory_;
}
#else
void* ARABridge::get_ara_factory_raw() {
    return ara_factory_raw_;
}
#endif

std::pair<ARABridge::PluginInstance&, std::shared_lock<std::shared_mutex>>
ARABridge::get_instance(size_t instance_id) noexcept {
    std::shared_lock lock(object_instances_mutex_);
    return std::pair<PluginInstance&, std::shared_lock<std::shared_mutex>>(
        object_instances_.at(instance_id), std::move(lock));
}

AraDocumentControllerProxyImpl* ARABridge::get_or_create_document_controller(uint64_t ref) {
    if (ref == 0) {
        return nullptr;
    }
    
    std::shared_lock lock(document_controllers_mutex_);
    auto it = document_controllers_.find(ref);
    if (it != document_controllers_.end()) {
        return it->second.get();
    }
    return nullptr;
}

void ARABridge::remove_document_controller(uint64_t ref) {
    if (ref == 0) {
        return;
    }
    
    std::unique_lock lock(document_controllers_mutex_);
    document_controllers_.erase(ref);
}

AraDocumentControllerProxyImpl::AraDocumentControllerProxyImpl(ARABridge& bridge, Ref ref)
    : bridge_(bridge), ref_(ref) {
#ifdef WITH_ARA
    // The actual ARA DocumentController would be created here
    // For now, we just store the reference
#endif
}

AraDocumentControllerProxyImpl::~AraDocumentControllerProxyImpl() noexcept = default;

// DocumentController proxy methods - these would call into the actual ARA SDK
// For now they are stubs that log and return default responses

MusicalContextRefResponse AraDocumentControllerProxyImpl::create_musical_context(
    const AraDocumentControllerProxy::CreateMusicalContextRequest& request) {
    bridge_.logger_.log_info([&](auto& log) {
        log << "ARA: CreateMusicalContext request for controller " << request.controller_ref;
    });
    return MusicalContextRefResponse{0, false};
}

ARANullResponse AraDocumentControllerProxyImpl::update_musical_context_properties(
    const AraDocumentControllerProxy::UpdateMusicalContextPropertiesRequest& request) {
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::update_musical_context_content(
    const AraDocumentControllerProxy::UpdateMusicalContextContentRequest& request) {
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::destroy_musical_context(
    const AraDocumentControllerProxy::DestroyMusicalContextRequest& request) {
    return ARANullResponse{};
}

AudioSourceRefResponse AraDocumentControllerProxyImpl::create_audio_source(
    const AraDocumentControllerProxy::CreateAudioSourceRequest& request) {
    return AudioSourceRefResponse{0, false};
}

ARANullResponse AraDocumentControllerProxyImpl::update_audio_source_properties(
    const AraDocumentControllerProxy::UpdateAudioSourcePropertiesRequest& request) {
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::update_audio_source_content(
    const AraDocumentControllerProxy::UpdateAudioSourceContentRequest& request) {
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::enable_audio_source_samples_access(
    const AraDocumentControllerProxy::EnableAudioSourceSamplesAccessRequest& request) {
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::deactivate_audio_source_for_undo_history(
    const AraDocumentControllerProxy::DeactivateAudioSourceForUndoHistoryRequest& request) {
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::destroy_audio_source(
    const AraDocumentControllerProxy::DestroyAudioSourceRequest& request) {
    return ARANullResponse{};
}

AudioModificationRefResponse AraDocumentControllerProxyImpl::create_audio_modification(
    const AraDocumentControllerProxy::CreateAudioModificationRequest& request) {
    return AudioModificationRefResponse{0, false};
}

AudioModificationRefResponse AraDocumentControllerProxyImpl::clone_audio_modification(
    const AraDocumentControllerProxy::CloneAudioModificationRequest& request) {
    return AudioModificationRefResponse{0, false};
}

ARANullResponse AraDocumentControllerProxyImpl::update_audio_modification_properties(
    const AraDocumentControllerProxy::UpdateAudioModificationPropertiesRequest& request) {
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::deactivate_audio_modification_for_undo_history(
    const AraDocumentControllerProxy::DeactivateAudioModificationForUndoHistoryRequest& request) {
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::destroy_audio_modification(
    const AraDocumentControllerProxy::DestroyAudioModificationRequest& request) {
    return ARANullResponse{};
}

PlaybackRegionRefResponse AraDocumentControllerProxyImpl::create_playback_region(
    const AraDocumentControllerProxy::CreatePlaybackRegionRequest& request) {
    return PlaybackRegionRefResponse{0, false};
}

ARANullResponse AraDocumentControllerProxyImpl::update_playback_region_properties(
    const AraDocumentControllerProxy::UpdatePlaybackRegionPropertiesRequest& request) {
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::destroy_playback_region(
    const AraDocumentControllerProxy::DestroyPlaybackRegionRequest& request) {
    return ARANullResponse{};
}

RegionSequenceRefResponse AraDocumentControllerProxyImpl::create_region_sequence(
    const AraDocumentControllerProxy::CreateRegionSequenceRequest& request) {
    return RegionSequenceRefResponse{0, false};
}

CountResponse AraDocumentControllerProxyImpl::get_region_sequence_count(
    const AraDocumentControllerProxy::GetRegionSequenceCountRequest& request) {
    return CountResponse{0};
}

RegionSequencePropertiesResponse AraDocumentControllerProxyImpl::get_region_sequence_properties(
    const AraDocumentControllerProxy::GetRegionSequencePropertiesRequest& request) {
    return RegionSequencePropertiesResponse{{}, false};
}

ARANullResponse AraDocumentControllerProxyImpl::set_region_sequence_properties(
    const AraDocumentControllerProxy::SetRegionSequencePropertiesRequest& request) {
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::destroy_region_sequence(
    const AraDocumentControllerProxy::DestroyRegionSequenceRequest& request) {
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::store_objects_to_archive(
    const AraDocumentControllerProxy::StoreObjectsToArchiveRequest& request) {
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::restore_objects_from_archive(
    const AraDocumentControllerProxy::RestoreObjectsFromArchiveRequest& request) {
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::analyze_audio_source(
    const AraDocumentControllerProxy::AnalyzeAudioSourceRequest& request) {
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::begin_editing(
    const AraDocumentControllerProxy::BeginEditingRequest& request) {
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::end_editing(
    const AraDocumentControllerProxy::EndEditingRequest& request) {
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::notify_model_updates(
    const AraDocumentControllerProxy::NotifyModelUpdatesRequest& request) {
    return ARANullResponse{};
}

ARANullResponse AraDocumentControllerProxyImpl::update_document_properties(
    const AraDocumentControllerProxy::UpdateDocumentPropertiesRequest& request) {
    return ARANullResponse{};
}

AraDocumentControllerProxyImpl* ARABridge::get_or_create_document_controller(uint64_t ref) {
    if (ref == 0) {
        return nullptr;
    }
    
    std::shared_lock read_lock(document_controllers_mutex_);
    auto it = document_controllers_.find(ref);
    if (it != document_controllers_.end()) {
        return it->second.get();
    }
    read_lock.unlock();
    
    // Create new controller if we have the ARA factory
#ifdef WITH_ARA
    if (ara_factory_) {
        std::unique_lock write_lock(document_controllers_mutex_);
        // Double-check after acquiring write lock
        it = document_controllers_.find(ref);
        if (it != document_controllers_.end()) {
            return it->second.get();
        }
        
        // For now, we create a new controller with a default document
        // In a real implementation, we'd need to track which controller corresponds to which ref
        // This is a simplified implementation
        auto controller = std::make_unique<AraDocumentControllerProxyImpl>(*this, ref);
        AraDocumentControllerProxyImpl* controller_ptr = controller.get();
        document_controllers_.emplace(ref, std::move(controller));
        return controller_ptr;
    }
#endif
    
    return nullptr;
}

void ARABridge::remove_document_controller(uint64_t ref) {
    if (ref == 0) {
        return;
    }
    
    std::unique_lock lock(document_controllers_mutex_);
    document_controllers_.erase(ref);
}

AraDocumentControllerProxyImpl::AraDocumentControllerProxyImpl(ARABridge& bridge, Ref ref)
    : bridge_(bridge), ref_(ref) {
#ifdef WITH_ARA
    // In a full implementation, we would create the actual document controller here
    // using the ARA factory. For now, we just store the reference.
    if (bridge_.ara_factory_) {
        // We would need the host instance and document properties to create the controller
        // This is a placeholder - the actual implementation would be more complex
    }
#endif
}

AraDocumentControllerProxyImpl::~AraDocumentControllerProxyImpl() noexcept {
    // Clean up the document controller if needed
#ifdef WITH_ARA
    if (controller_) {
        // The controller will be destroyed by the ARA factory
    }
#endif
}