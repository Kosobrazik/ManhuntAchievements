#pragma once
#include <Windows.h>
#include <filesystem>
#include <string>
#include <cwchar>

namespace AchievementLanguageSetting
{
    // Internal values stay unchanged so the gallery and old INIs keep working.
    inline int Parse(std::wstring value)
    {
        const auto first = value.find_first_not_of(L" \t\r\n");
        if (first == std::wstring::npos)
            return 0;
        const auto last = value.find_last_not_of(L" \t\r\n");
        value = value.substr(first, last - first + 1);
        if (value.size() == 1 && value[0] >= L'0' && value[0] <= L'6')
            return value[0] - L'0';

        struct Name { const wchar_t* shortName; const wchar_t* fullName; int language; };
        const Name names[] = {
            {L"auto", L"automatic", 0},
            {L"en", L"english", 1},
            {L"ru", L"russian", 2},
            {L"de", L"german", 3},
            {L"fr", L"french", 4},
            {L"es", L"spanish", 5},
            {L"it", L"italian", 6},
        };
        for (const auto& name : names)
            if (_wcsicmp(value.c_str(), name.shortName) == 0 ||
                _wcsicmp(value.c_str(), name.fullName) == 0)
                return name.language;
        return 0;
    }

    inline int Read(const std::filesystem::path& path)
    {
        wchar_t value[64]{};
        GetPrivateProfileStringW(L"Achievements", L"Language", L"auto", value,
            static_cast<DWORD>(sizeof(value) / sizeof(value[0])), path.c_str());
        return Parse(value);
    }
}
