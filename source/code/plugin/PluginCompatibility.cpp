#include "PluginCompatibility.h"
#include "HookSites.h"
#include "eLog.h"
#include <cstring>

namespace {
HMODULE self;
HMODULE game;
bool foreignMainMenu;
// Executable code inside a loaded module, whichever module that is.
HMODULE OwnerOf(uintptr_t target)
{
    MEMORY_BASIC_INFORMATION memory{};
    if (!VirtualQuery(reinterpret_cast<void*>(target),&memory,sizeof(memory)) ||
        memory.State!=MEM_COMMIT || (memory.Protect & (PAGE_GUARD|PAGE_NOACCESS)) ||
        !(memory.Protect & (PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))
        return nullptr;
    return static_cast<HMODULE>(memory.AllocationBase);
}
// Leftovers of an instruction the five byte jump split in half: either still
// the game's own bytes, or NOPs, which is how most detours pad the remainder.
bool TailIsOriginalOrPadding(const HookSite& site)
{
    const auto* code=reinterpret_cast<const unsigned char*>(site.address);
    for (unsigned i=5;i<site.size;++i)
        if (code[i]!=site.expected[i] && code[i]!=0x90)
            return false;
    return true;
}
// An E9/E8 detour already installed by some other plugin at a site we also use.
bool DetourToForeignModule(uintptr_t site, unsigned char opcode)
{
    return *reinterpret_cast<const unsigned char*>(site)==opcode &&
        PluginCompatibility::BelongsToForeignModule(PluginCompatibility::CallTarget(site));
}
}

bool PluginCompatibility::BelongsToForeignModule(uintptr_t target)
{
    const HMODULE owner=OwnerOf(target);
    return owner && owner!=self && owner!=game;
}

bool PluginCompatibility::Discover()
{
    self=nullptr; game=GetModuleHandleW(nullptr); foreignMainMenu=false;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&PluginCompatibility::Discover),&self);
    if (GetModuleHandleW(L"PluginMH_Achievements.asi")) {
        eLog::Message(__FUNCTION__,"Legacy PluginMH_Achievements detected; duplicate achievement engines are unsupported");
        return false;
    }
    // Both main menu sites must be owned by the same foreign plugin: we append a
    // row to a list it draws, so a half-hooked menu would desynchronize indices.
    const bool draw=DetourToForeignModule(0x600C20,0xE9);
    const bool input=DetourToForeignModule(0x600B34,0xE9);
    foreignMainMenu=draw && input &&
        OwnerOf(CallTarget(0x600C20))==OwnerOf(CallTarget(0x600B34));
    eLog::Message(__FUNCTION__,
        "Foreign detours: main menu draw=%d, main menu input=%d, frontend render=%d; "
        "appending our row=%d",
        draw ? 1 : 0,input ? 1 : 0,ForeignSite(0x5F189F) ? 1 : 0,foreignMainMenu ? 1 : 0);
    return true;
}

bool PluginCompatibility::ForeignMainMenu() { return foreignMainMenu; }

bool PluginCompatibility::ForeignSite(uintptr_t site)
{
    return DetourToForeignModule(site,0xE9) || DetourToForeignModule(site,0xE8);
}

uintptr_t PluginCompatibility::CallTarget(uintptr_t site)
{
    int32_t offset=0; memcpy(&offset,reinterpret_cast<void*>(site+1),4);
    return site+5+offset;
}

uintptr_t PluginCompatibility::JumpTarget(uintptr_t site)
{
    return DetourToForeignModule(site,0xE9) ? CallTarget(site) : 0;
}

// Safe to take over: untouched game code, or a detour we know how to chain to.
bool PluginCompatibility::DetourableSite(const HookSite& site)
{
    const auto* code=reinterpret_cast<const unsigned char*>(site.address);
    return memcmp(code,site.expected,site.size)==0 || AcceptPatch(site);
}

bool PluginCompatibility::AcceptPatch(const HookSite& site)
{
    const auto* code=reinterpret_cast<const unsigned char*>(site.address);
    // Function pointers rather than code. Another plugin's entry there is a
    // reason to stand aside, never a reason to refuse to load.
    if (site.address==0x7113FC || site.address==0x7D61D0 || site.address==0x7D61D4)
        return BelongsToForeignModule(*reinterpret_cast<const uintptr_t*>(code));
    switch (site.address) {
    // Detoured with a jump: the replaced instruction is longer than five bytes,
    // so whatever follows the jump must still be the game's original code.
    case 0x600C20: case 0x600B34: case 0x5D70F9: case 0x5D75D7: case 0x5D55C0:
        return DetourToForeignModule(site.address,0xE9) && TailIsOriginalOrPadding(site);
    case 0x4D7F70: case 0x474A02: case 0x473F53: case 0x5F189F:
    case 0x4811F6: case 0x4ECDE3: case 0x45E688:
        return DetourToForeignModule(site.address,0xE8);
    default: return false;
    }
}

// Historical local workaround, excluded from releases at the user's request.
// Achievements must not change PluginMH / WidescreenFix frame behavior.
// void PluginCompatibility::PreserveWidescreenFrame()
// {
//     Restored the native LCD frame branch at 0x5D74B0 when PluginMH's block skip
//     and a live WidescreenFix draw hook were both present. Removed together with
//     the build-specific PluginMH detection; see git history for the full body.
// }
