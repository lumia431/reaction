// include/reaction/exception.h
#ifndef REACTION_EXCEPTION_H
#define REACTION_EXCEPTION_H

#include <stdexcept>
#include <string>
#include <format>  // C++20的std::format

namespace reaction {

class Exception : public std::runtime_error {
public:
    // 使用格式化字符串的构造函数
    template <typename... Args>
    explicit Exception(const std::string& format_str, Args&&... args)
        : std::runtime_error(std::format(format_str, std::forward<Args>(args)...)) {}

};

} // namespace reaction

#endif // REACTION_EXCEPTION_H
