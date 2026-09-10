#pragma once
#include <filesystem>

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
};
