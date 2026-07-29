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

#include "common.h"

/**
 * ARA-specific logger wrappers. These are thin wrappers around the base
 * Logger class that provide ARA-specific logging functionality with
 * appropriate verbosity levels.
 */
class ARALogger {
   public:
    ARALogger() = default;
    explicit ARALogger(Logger logger) : logger_(std::move(logger)) {}

    // Log at trace level (verbosity 2)
    template <typename F>
    void log_trace(F&& fn) {
        logger_.log_trace(std::forward<F>(fn));
    }

    // Log at debug level (verbosity 1)
    template <typename F>
    void log_debug(F&& fn) {
        logger_.log_debug(std::forward<F>(fn));
    }

    // Log at info level (verbosity 0)
    template <typename F>
    void log_info(F&& fn) {
        logger_.log_info(std::forward<F>(fn));
    }

    // Log at warning level (always shown)
    template <typename F>
    void log_warning(F&& fn) {
        logger_.log_warning(std::forward<F>(fn));
    }

    // Log at error level (always shown)
    template <typename F>
    void log_error(F&& fn) {
        logger_.log_error(std::forward<F>(fn));
    }

    // Log a raw string
    void log(const std::string& message) {
        logger_.log(message);
    }

   private:
    Logger logger_ = Logger::create_from_environment("[ARA] ");
};