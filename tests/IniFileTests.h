#pragma once
#include "../source/code/core/IniFile.h"
#include "../source/code/core/AchievementLanguageSetting.h"

#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void WriteSettings(const std::filesystem::path& path, bool byteOrderMark)
{
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (byteOrderMark)
        file << "\xEF\xBB\xBF";
    file << "[Achievements]\r\nLog=2\r\nSound=1\r\n";
}

int ReadLogSetting(const std::filesystem::path& path)
{
    return static_cast<int>(GetPrivateProfileIntW(L"Achievements", L"Log", 0,
        path.wstring().c_str()));
}
}

void TestAchievementLanguageSettings(const std::filesystem::path& root)
{
    const auto path = root / "language.ini";
    struct Case { const char* value; int expected; };
    const Case cases[] = {
        {"auto", 0}, {"automatic", 0}, {"AUTO", 0},
        {"en", 1}, {"English", 1}, {"ru", 2}, {"RUSSIAN", 2},
        {"de", 3}, {"German", 3}, {"fr", 4}, {"French", 4},
        {"es", 5}, {"Spanish", 5}, {"it", 6}, {"Italian", 6},
        {"0", 0}, {"1", 1}, {"2", 2}, {"3", 3}, {"4", 4}, {"5", 5}, {"6", 6},
        {"  rU  ", 2}, {"\tFrEnCh\t", 4},
        {"", 0}, {"7", 0}, {"-1", 0}, {"1junk", 0}, {"unknown", 0},
        {"russianxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 0},
    };
    for (const auto& item : cases)
    {
        {
            std::ofstream file(path, std::ios::binary | std::ios::trunc);
            file << "[Achievements]\r\nLanguage=" << item.value << "\r\nEnabled=1\r\n";
        }
        if (AchievementLanguageSetting::Read(path) != item.expected)
            throw std::runtime_error(std::string("Language setting failed: ") + item.value);
    }
    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        file << "[Achievements]\r\nEnabled=1\r\n";
    }
    if (AchievementLanguageSetting::Read(path) != 0)
        throw std::runtime_error("Missing Language must select auto");
    std::filesystem::remove(path);
    if (AchievementLanguageSetting::Read(path) != 0)
        throw std::runtime_error("Missing INI must select auto");
    std::cout << "Named/legacy language INI settings and invalid-value fallback: PASS\n";
}

// A UTF-8 byte order mark ahead of the first section makes Windows' own INI
// reader miss that section entirely, so every setting falls back to its default
// and the plugin runs as if the file were empty. It cost a whole diagnostic
// session once: logging was switched on in the file and off in the game.
void TestIniByteOrderMark(const std::filesystem::path& root)
{
    const auto path = root / "settings.ini";
    const auto check = [](bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    };

    WriteSettings(path, true);
    check(ReadLogSetting(path) == 0, "a byte order mark was expected to hide the section");
    RemoveIniByteOrderMark(path);
    check(ReadLogSetting(path) == 2, "settings must be readable once the mark is gone");

    // A clean file has to come through untouched.
    WriteSettings(path, false);
    RemoveIniByteOrderMark(path);
    check(ReadLogSetting(path) == 2, "a file without a mark must be left alone");

    // No file: no crash, and nothing created.
    std::filesystem::remove(path);
    RemoveIniByteOrderMark(path);
    check(!std::filesystem::exists(path), "a missing INI must not be created");

    std::cout << "INI byte order mark: PASS\n";
}
