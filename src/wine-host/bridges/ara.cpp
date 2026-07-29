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
                return AraPluginFactoryProxy::ConstructArgs(
                    get_ara_factory_raw());
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

