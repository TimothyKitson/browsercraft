#include "Utf8.h"

namespace Utf8
{
    char32_t next(const std::string& text, size_t& at)
    {
        const unsigned char lead = static_cast<unsigned char>(text[at++]);
        if (lead < 0x80) return lead;
        int extra = 0;
        char32_t value = 0;
        char32_t lowest = 0;
        if ((lead & 0xE0) == 0xC0) { extra = 1; value = lead & 0x1F; lowest = 0x80; }
        else if ((lead & 0xF0) == 0xE0) { extra = 2; value = lead & 0x0F; lowest = 0x800; }
        else if ((lead & 0xF8) == 0xF0) { extra = 3; value = lead & 0x07; lowest = 0x10000; }
        else return REPLACEMENT;
        for (int i = 0; i < extra; ++i)
        {
            if (at >= text.size()) return REPLACEMENT;
            const unsigned char follow = static_cast<unsigned char>(text[at]);
            if ((follow & 0xC0) != 0x80) return REPLACEMENT;
            value = (value << 6) | (follow & 0x3F);
            ++at;
        }
        if (value < lowest || value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF)) return REPLACEMENT;
        return value;
    }

    std::u32string decode(const std::string& text)
    {
        std::u32string out;
        out.reserve(text.size());
        size_t at = 0;
        while (at < text.size()) out.push_back(next(text, at));
        return out;
    }

    void append(std::string& out, char32_t c)
    {
        if (c > 0x10FFFF || (c >= 0xD800 && c <= 0xDFFF)) c = REPLACEMENT;
        if (c < 0x80)
            out.push_back(static_cast<char>(c));
        else if (c < 0x800)
        {
            out.push_back(static_cast<char>(0xC0 | (c >> 6)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        }
        else if (c < 0x10000)
        {
            out.push_back(static_cast<char>(0xE0 | (c >> 12)));
            out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        }
        else
        {
            out.push_back(static_cast<char>(0xF0 | (c >> 18)));
            out.push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        }
    }

    std::string encode(const std::u32string& text)
    {
        std::string out;
        out.reserve(text.size());
        for (char32_t c : text) append(out, c);
        return out;
    }

    size_t length(const std::string& text)
    {
        size_t count = 0;
        size_t at = 0;
        while (at < text.size())
        {
            next(text, at);
            ++count;
        }
        return count;
    }

    void popBack(std::string& text)
    {
        if (text.empty()) return;
        size_t start = text.size() - 1;
        int back = 0;
        while (start > 0 && back < 3 && (static_cast<unsigned char>(text[start]) & 0xC0) == 0x80)
        {
            --start;
            ++back;
        }
        size_t probe = start;
        next(text, probe);
        if (probe != text.size()) start = text.size() - 1;
        text.erase(start);
    }

    void popFront(std::string& text)
    {
        if (text.empty()) return;
        size_t at = 0;
        next(text, at);
        text.erase(0, at);
    }

    std::string prefix(const std::string& text, size_t codepoints)
    {
        size_t at = 0;
        for (size_t i = 0; i < codepoints && at < text.size(); ++i) next(text, at);
        return text.substr(0, at);
    }

    bool typeable(char32_t c)
    {
        return c >= 32 && c != 127 && !(c >= 0x80 && c < 0xA0) && c != 0xA7 && c != REPLACEMENT && !(c >= 0xE000 && c < 0xE100);
    }

    bool type(std::string& field, const std::string& typed, size_t maxCodepoints)
    {
        bool changed = false;
        size_t count = length(field);
        size_t at = 0;
        while (at < typed.size())
        {
            const char32_t c = next(typed, at);
            if (!typeable(c) || count >= maxCodepoints) continue;
            append(field, c);
            ++count;
            changed = true;
        }
        return changed;
    }
}
