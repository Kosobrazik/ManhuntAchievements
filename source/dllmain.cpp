#include "code/core/AchievementSettings.h"
#include "code/plugin/AchievementHooks.h"
#include "code/plugin/AchievementMenu.h"
#include "code/plugin/eAchievements.h"
#include "code/plugin/eLog.h"
#include "code/plugin/PluginCompatibility.h"
#include "code/manhunt/core.h"
#include <Windows.h>
#include <cstring>

static_assert(sizeof(void*) == 4, "ManhuntAchievements requires Win32/x86.");

static void InstallAchievements()
{
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;

    // Validate every patched site before making the first change to game memory.
    eLog::Message(__FUNCTION__, "ASI loading complete; checking game hook sites and other plugins");
    if (!PluginCompatibility::Discover() || !AchievementHooks::Validate())
    {
        MessageBoxA(nullptr,
            "Unsupported executable or conflicting plugin hooks.\n"
            "ManhuntAchievements was not activated.\n"
            "Do not load the old PluginMH_Achievements at the same time.",
            "ManhuntAchievements", MB_OK | MB_ICONWARNING);
        return;
    }

    eLog::Message(__FUNCTION__, "Hook validation passed; preparing achievement state");
    eAchievements::Initialize();
    eLog::Message(__FUNCTION__, "Installing lifecycle and gameplay hooks");
    AchievementHooks::Install();
    eLog::Message(__FUNCTION__, "Installing achievement event hooks");
    eAchievements::InitHooks();
    eLog::Message(__FUNCTION__, "Installing achievement menu hooks");
    AchievementMenu::InitHooks();
    FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
    eLog::Message(__FUNCTION__, "Standalone achievements initialized");
}

static bool InitializeGameWithAchievements()
{
    // Hooks go in first, before the frontend reads its resources. The profile
    // waits for the game to start up and be able to report its user directory.
    InstallAchievements();
    const bool started = CallAndReturn<bool, 0x4D7830>();
    eAchievements::LoadProfile();
    return started;
}

extern "C" __declspec(dllexport) void InitializeASI()
{
    static bool scheduled=false;
    if (scheduled) return;
    scheduled=true;
    AchievementSettings::Init();
    eLog::Initialise();
    eLog::Message(__FUNCTION__, "Settings read; preparing startup hook");
    if (!AchievementSettings::bEnableAchievements) return;
    constexpr unsigned char original[]={0xE8,0x3E,0x9C,0x01,0x00};
    MEMORY_BASIC_INFORMATION memory{};
    const auto* address=reinterpret_cast<const unsigned char*>(0x4BDBED);
    if (reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))!=0x400000 ||
        !VirtualQuery(address,&memory,sizeof(memory)) || memory.State!=MEM_COMMIT ||
        (memory.Protect & (PAGE_NOACCESS|PAGE_GUARD)) || memcmp(address,original,sizeof(original))!=0) {
        eLog::Message(__FUNCTION__,"Unsupported startup hook; achievements not activated");
        return;
    }
    // Run after ASI loading, before the frontend TXD is read, in either load order.
    InjectHook(0x4BDBED,InitializeGameWithAchievements,PATCH_CALL);
    FlushInstructionCache(GetCurrentProcess(),nullptr,0);
    eLog::Message(__FUNCTION__,"Waiting for native game initialization");
}
