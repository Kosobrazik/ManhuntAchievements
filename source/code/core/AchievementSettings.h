#pragma once
#include <filesystem>
#include <string>

class AchievementSettings {
public:
    static void Init();
    static std::filesystem::path ModuleDirectory();
    static bool bEnableAchievements;
    static bool bEnableAchievementSound;
    static bool bAllowAchievementsWithCheats;
    static bool bEnableLog;
    static int iLogLevel;
    static int iAchievementLanguage;
    // Where the gallery's translations are looked for, relative to the ASI and
    // then to the game.
    static std::wstring sTranslationFiles;
};
