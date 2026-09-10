#include "AchievementSettings.h"
#include "IniFile.h"
#include <Windows.h>

bool AchievementSettings::bEnableAchievements = true;
bool AchievementSettings::bEnableAchievementSound = true;
// The PS4 trophy script never looks at cheats, so neither do we by default.
bool AchievementSettings::bAllowAchievementsWithCheats = true;
bool AchievementSettings::bEnableLog = false; // Opt-in via Log= in the INI.
int AchievementSettings::iLogLevel = 0;  // 1 = kills and scenes, 2 = everything.
int AchievementSettings::iAchievementLanguage = 0;

std::filesystem::path AchievementSettings::ModuleDirectory()
{
    HMODULE module = nullptr;
    wchar_t path[MAX_PATH] = {};
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&ModuleDirectory), &module) &&
        GetModuleFileNameW(module, path, MAX_PATH))
        return std::filesystem::path(path).parent_path();
    return {};
}

void AchievementSettings::Init()
{
    const auto path = ModuleDirectory() / L"ManhuntAchievements.ini";
    RemoveIniByteOrderMark(path);
    const auto value = [&](const wchar_t* key, int defaultValue) {
        return static_cast<int>(GetPrivateProfileIntW(L"Achievements", key,
            defaultValue, path.c_str()));
    };
    bEnableAchievements = value(L"Enabled", 1) != 0;
    bEnableAchievementSound = value(L"Sound", 1) != 0;
    bAllowAchievementsWithCheats = value(L"AllowWithCheats", 1) != 0;
    iLogLevel = value(L"Log", 0);
    if (iLogLevel < 0)
        iLogLevel = 0;
    if (iLogLevel > 2)
        iLogLevel = 2;
    bEnableLog = iLogLevel != 0;
    iAchievementLanguage = value(L"Language", 0);
    if (iAchievementLanguage < 0 || iAchievementLanguage > 2)
        iAchievementLanguage = 0;
}
