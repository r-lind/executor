#pragma once

#include <string>
#include <string_view>
#include "filesystem.h"

namespace Executor
{

using mac_string_view = std::basic_string_view<unsigned char>;
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

}
