#pragma once
#include "eAchievements.h"
#include <string>
// PluginMH numbers its own pages 24 to 29 and puts its achievements on 29 as
// well. Our menu dispatch detour sits ahead of its own and takes the frame for
// this page, so the shared number costs nothing.
constexpr int MENU_ACHIEVEMENTS = 29;
class AchievementMenu {
public:
    static void InitHooks();
    static void MainMenu();
    static void AppendAchievementRowToMainMenu();
    static void HookMenuOptionRegistered();
    static void HookMenuOptionText();
    static bool ProcessMainMenu();
    static void HookSelectMenuBackground();
    static void HookExecuteMenuProcess();
    static void AchievementsMenu();
    static void ProcessAchievementsMenu();
    static int HookPauseMenuProcess();
    static void HookPauseMenuDraw();
    static int GetAchievementSmallTexture(eAchievementID id);
    static std::wstring GetLocalizedAchievementName(eAchievementID id);
    static std::wstring GetLocalizedAchievementDescription(eAchievementID id);
    static std::wstring GetLocalizedAchievementUnlockedText();
    static void Open(int returnMenu);
    static int m_nCurrentAchievementPos, m_nCurrentAchievementPage;
    static int m_allAchievementPages, m_achievementReturnMenu;
    static wchar_t m_szStatsBuffer[128];
};
