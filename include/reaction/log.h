/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include <iostream>
#include <sstream>
#include <string>

#if __has_include(<format>) && (__cplusplus >= 202002L)
#include <format>
#define REACTION_HAS_FORMAT 1
#else
#define REACTION_HAS_FORMAT 0
#endif

namespace reaction {

/**
 * @brief Lightweight logging utility.
 *
 * Supports three levels: Info, Warn, and Error.
 * Uses std::format when available (C++20), otherwise falls back to custom "{}"-based formatting.
 */
class Log {
public:
    /// Log levels
    enum class Level {
        Info = 0,
        Warn = 1,
        Error = 2
    };

    /// Global log threshold; only messages with level >= threshold will be printed.
    static inline Level level_threshold = Level::Error;

    /// Print an info-level message
    template <typename... Args>
    static void info(const std::string &format_str, Args &&...args) {
        log(Level::Info, "INFO", format_str, std::forward<Args>(args)...);
    }

    /// Print a warning-level message
    template <typename... Args>
    static void warn(const std::string &format_str, Args &&...args) {
        log(Level::Warn, "WARN", format_str, std::forward<Args>(args)...);
    }

    /// Print an error-level message
    template <typename... Args>
    static void error(const std::string &format_str, Args &&...args) {
        log(Level::Error, "ERROR", format_str, std::forward<Args>(args)...);
    }

private:
    /**
     * @brief Internal formatter and dispatcher
     *
     * Formats and prints log messages if the level passes threshold.
     */
    template <typename... Args>
    static void log(Level level, const std::string &level_str, const std::string &format_str, Args &&...args) {
        if (level < level_threshold) return;

#if REACTION_HAS_FORMAT
        std::string message = std::vformat(format_str, std::make_format_args(args...));
#else
        std::ostringstream oss;
        replace_placeholders(oss, format_str, std::forward<Args>(args)...);
        std::string message = oss.str();
#endif

        std::cout << "[" << level_str << "] " << message << std::endl;
    }

#if !REACTION_HAS_FORMAT
    /**
     * @brief Fallback formatting: base case (no args)
     */
    static void replace_placeholders(std::ostringstream &oss, const std::string &str) {
        oss << str;
    }

    /**
     * @brief Fallback formatting: replaces first "{}" with the next argument
     */
    template <typename T, typename... Args>
    static void replace_placeholders(std::ostringstream &oss, const std::string &str, T &&first, Args &&...rest) {
        size_t pos = str.find("{}");
        if (pos != std::string::npos) {
            oss << str.substr(0, pos);
            oss << std::forward<T>(first);
            replace_placeholders(oss, str.substr(pos + 2), std::forward<Args>(rest)...);
        } else {
            // No placeholder left; append as is
            oss << str;
        }
    }
#endif
};

} // namespace reaction
