#pragma once
#include <cstdint>
#include <Windows.h>
struct HookSite;
// Cooperation with any other ASI that already hooks the sites we share, without
// knowing which plugin it is. Nothing here is tied to a particular build.
namespace PluginCompatibility {
bool Discover();
// A foreign module owns the main menu draw/input sites, so its rows are drawn by
// it and we only append our own. False means we own the vanilla main menu.
bool ForeignMainMenu();
// The site already carries an E9/E8 detour into a module that is neither the
// game nor us. Whether such a target may be called depends on the site: one
// that replaces a function entry is reached by the game with a call and must
// return, while one that replaces a call in the middle of a function need not
// return at all and may only be entered with a jump.
bool ForeignSite(uintptr_t site);
bool AcceptPatch(const HookSite& site);
bool DetourableSite(const HookSite& site);
uintptr_t CallTarget(uintptr_t site);
// Target of an existing E9 detour that belongs to another plugin, else 0.
uintptr_t JumpTarget(uintptr_t site);
bool BelongsToForeignModule(uintptr_t target);
}
