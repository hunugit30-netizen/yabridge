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

#include <future>
#include <variant>

#include "../logging/ara.h"
#include "../serialization/ara.h"
#include "common.h"

/**
 * Manages all the sockets used for communicating between the plugin and the
 * Wine host when hosting an ARA plugin.
 *
 * ARA uses a similar communication model to VST3/CLAP - main thread control
 * messages, callbacks, and potentially dedicated audio thread sockets.
 *
 * @tparam Thread The thread implementation to use. On the Linux side this
 *   should be `std::jthread` and on the Wine side this should be `Win32Thread`.
 */
template <typename Thread>
class ARASockets final : public Sockets {
   public:
    /**
     * Sets up the sockets using the specified base directory. The sockets won't
     * be active until `connect()` gets called.
     *
     * @param io_context The IO context the sockets should be bound to. Relevant
     *   when doing asynchronous operations.
     * @param endpoint_base_dir The base directory that will be used for the
     *   Unix domain sockets.
     * @param listen If `true`, start listening on the sockets. Incoming
     *   connections will be accepted when `connect()` gets called. This should
     *   be set to `true` on the plugin side, and `false` on the Wine host side.
     *
     * @see ARASockets::connect
     */
    ARASockets(asio::io_context& io_context,
               const ghc::filesystem::path& endpoint_base_dir,
               bool listen)
        : Sockets(endpoint_base_dir),
          plugin_host_control_(
              io_context,
              (base_dir_ / "plugin_host_control.sock").string(),
              listen),
          host_plugin_callback_(
              io_context,
              (base_dir_ / "host_plugin_callback.sock").string(),
              listen) {}

    // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.VirtualCall)
    ~ARASockets() noexcept override { close(); }

    void connect() override {
        plugin_host_control_.connect();
        host_plugin_callback_.connect();
    }

    void close() override {
        // Manually close all sockets so we break out of any blocking operations
        plugin_host_control_.close();
        host_plugin_callback_.close();
    }

    /**
     * For sending control messages from the plugin to the host.
     * This will be listened on by the Wine plugin host when it calls
     * `receive_multi()`.
     */
    TypedMessageHandler<Thread, ARALogger, ARAControlRequest>
        plugin_host_control_;

    /**
     * For sending callbacks from the host back to the plugin.
     */
    TypedMessageHandler<Thread, ARALogger, ARACallbackRequest>
        host_plugin_callback_;
};