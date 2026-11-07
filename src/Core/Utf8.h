#pragma once
#include <cstddef>
#include <string>

namespace Utf8
{
    constexpr char32_t REPLACEMENT = 0xFFFD;

    char32_t next(const std::string& text, size_t& at);
    std::u32string decode(const std::string& text);
    void append(std::string& out, char32_t codepoint);
    std::string encode(const std::u32string& text);
    size_t length(const std::string& text);
    void popBack(std::string& text);
    void popFront(std::string& text);
    std::string prefix(const std::string& text, size_t codepoints);
    bool typeable(char32_t codepoint);
    bool type(std::string& field, const std::string& typed, size_t maxCodepoints);
}
