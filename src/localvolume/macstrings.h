#pragma once

#include <string>
#include <string_view>
#include "filesystem.h"

namespace Executor
{

//using mac_string_view = std::basic_string_view<unsigned char>;

struct mac_string_view : std::basic_string_view<unsigned char>
{
    mac_string_view() = default;
    mac_string_view(const unsigned char* pascalString)
    {
        if(pascalString)
            *this = mac_string_view(pascalString + 1, pascalString[0]);
    }

    mac_string_view(const unsigned char* p, size_t n)
        : basic_string_view(p,n)
    {
    }
};

inline mac_string_view PascalStringView(const unsigned char *s)
{
    if(s)
        return mac_string_view(s+1, (size_t)s[0]);
    else
        return mac_string_view();
}

using mac_string = std::basic_string<unsigned char>;

std::u32string toUnicode(mac_string_view sv);
mac_string toMacRoman(const std::u32string& s);

fs::path toUnicodeFilename(mac_string_view sv);
mac_string toMacRomanFilename(const fs::path& s);

bool matchesMacRomanFilename(const fs::path& s, mac_string_view sv);

bool equalCaseInsensitive(mac_string_view s1, mac_string_view s2);
}
