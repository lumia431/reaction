// include/reaction/log.h
#ifndef REACTION_LOG_H
#define REACTION_LOG_H

#include <iostream>
#include <string>
#include <format>

namespace reaction
{
    class Log
    {
    public:
        template <typename... Args>
        static void info(const std::string &format_str, Args &&...args)
        {
            log("INFO", format_str, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void warn(const std::string &format_str, Args &&...args)
        {
            log("WARN", format_str, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void error(const std::string &format_str, Args &&...args)
        {
            log("ERROR", format_str, std::forward<Args>(args)...);
        }

    private:
        template <typename... Args>
        static void log(const std::string &level, const std::string &format_str, Args &&...args)
        {
            auto fmt_args{std::make_format_args(args...)};
            std::string message = std::vformat(format_str, fmt_args);
            std::cout << "[" << level << "] " << message << std::endl;
        }
    };

} // namespace reaction

#endif // REACTION_LOG_H