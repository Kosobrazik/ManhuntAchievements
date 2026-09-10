#pragma once
#include <cstdint>
// Verified against Steam manhunt.exe; see docs/DEVELOPMENT.md.
struct HookSite { uintptr_t address; unsigned size; unsigned char expected[7]; const char* name; };
inline constexpr HookSite kHookSites[] = {
    { 0x63BC93, 4, { 0x66, 0x8B, 0x44, 0x24 }, "executable signature" },
    { 0x4D50D4, 5, { 0xE8, 0xD7, 0x10, 0x00, 0x00 }, "actual stream file size" },
    { 0x600C20, 7, { 0x53, 0x8B, 0x0D, 0x20, 0x87, 0x7C, 0x00 }, "main menu draw" },
    { 0x600B34, 5, { 0xE8, 0xD7, 0x7E, 0xFD, 0xFF }, "main menu input" },
    { 0x5D70F9, 5, { 0xA1, 0xF8, 0x86, 0x7C, 0x00 }, "menu background" },
    { 0x5D75D7, 5, { 0xA1, 0xF8, 0x86, 0x7C, 0x00 }, "menu dispatch" },
    { 0x5FFB89, 1, { 0x03 }, "pause up wrap" },
    { 0x5FFBAC, 1, { 0x03 }, "pause down wrap" },
    { 0x7D61D0, 4, { 0x50, 0xFB, 0x5F, 0x00 }, "pause process pointer" },
    { 0x7D61D4, 4, { 0x20, 0xFD, 0x5F, 0x00 }, "pause draw pointer" },
    { 0x7113FC, 4, { 0xA0, 0xB8, 0x45, 0x00 }, "player death pointer" },
    { 0x4D7F70, 5, { 0xE8, 0xDB, 0x1D, 0xFB, 0xFF }, "shutdown" },
    { 0x474A02, 5, { 0xE8, 0xE9, 0x00, 0x16, 0x00 }, "scene start" },
    { 0x473F53, 5, { 0xE8, 0x98, 0x0B, 0x16, 0x00 }, "scene reset" },
    { 0x5F189F, 5, { 0xE8, 0xCC, 0x57, 0xFE, 0xFF }, "render" },
    { 0x4811F6, 5, { 0xE8, 0xE5, 0x50, 0x13, 0x00 }, "execution" },
    { 0x4ECDE3, 5, { 0xE8, 0xB8, 0x95, 0x0C, 0x00 }, "kill" },
    { 0x45E688, 5, { 0xE8, 0xD3, 0x14, 0x18, 0x00 }, "painkiller" },
    { 0x47414B, 5, { 0xE8, 0xD0, 0xE1, 0x15, 0x00 }, "scene finalization" },
    { 0x4FC7E1, 5, { 0xE8, 0x5A, 0x3E, 0x02, 0x00 }, "lure" },
    { 0x519357, 5, { 0xE8, 0x14, 0x1E, 0xF1, 0xFF }, "sighting A" },
    { 0x5199D8, 5, { 0xE8, 0x93, 0x17, 0xF1, 0xFF }, "sighting B" },
    { 0x466742, 5, { 0xE8, 0x69, 0xD2, 0x03, 0x00 }, "body put down" },
};
// The sites below are optional: each is taken only when it still holds the
// bytes named here, and losing one costs the feature that rides on it rather
// than the whole plugin.

// Where a heavy object crushes a character: it names itself as the damage
// source and kills outright. That name is wiped again before the death handler
// runs, so it has to be taken here or not at all.
inline constexpr HookSite kCrushSite =
    { 0x4B99E0, 5, { 0x53, 0x55, 0x8B, 0x6C, 0x24 }, "crushed by object" };

// Where a ped records who hurt it. An explosion has no source entity to record,
// so the shot itself is the only thing that names the exploding object; without
// this the barrel achievement cannot be recognised at all.
inline constexpr HookSite kExplosiveDamageSite =
    { 0x4ECE70, 5, { 0x53, 0x56, 0x55, 0x8B, 0x44 }, "ped damage source" };

// Entry of the ped death handler, ahead of the five checks the game's own kill
// counter sits behind: object kills never reach that counter. Its absence costs
// only the object kill achievements.
inline constexpr HookSite kPedDeathSite =
    { 0x4ECD10, 5, { 0x53, 0x55, 0x89, 0xCB, 0xBD }, "ped death entry" };

// Our trampoline replays a call from the game's own prologue. Without it the
// row cannot be moved and is appended at the bottom of the foreign menu instead.
inline constexpr HookSite kMenuTextSite =
    { 0x5D5B30, 5, { 0x53, 0x31, 0xDB, 0xE8, 0x78 }, "menu option text" };

// Needed only to append a row to another plugin's main menu, so a shape we
// cannot chain safely costs us that row and never the whole plugin.
inline constexpr HookSite kMenuOptionSite =
    { 0x5D55C0, 7, { 0x53, 0x56, 0x57, 0x55, 0x83, 0xEC, 0x0C }, "menu option registration" };

// Immediately after the frontend render call, which is the only call site the
// game has for that function. Used when another plugin already owns the call at
// 0x5F189F: its hook is entered by a call whose callee we must not assume
// anything about, so the popup rides one instruction further instead. The
// trampoline replays the compare whose flags the following jump reads.
inline constexpr HookSite kRenderTailSite =
    { 0x5F18A4, 7, { 0x83, 0x3D, 0x78, 0x35, 0x7D, 0x00, 0x00 }, "render tail" };

