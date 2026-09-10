#pragma once
#include <cstdint>

// Optional, versioned ABI shared by ManhuntGInput and custom frontend plugins.
// Call from the game's input/render thread. Querying never polls or consumes input.
namespace ManhuntGInputUI {
constexpr std::uint32_t Version = 1;
constexpr const char* ExportName = "ManhuntGInput_GetUiState";
enum Flag : std::uint32_t { ControllerActive = 1, InlineGlyphs = 2 };
enum Action : std::uint32_t { PreviousPage = 1, NextPage = 2 };
enum GlyphSet : std::uint32_t { TextOnly = 0, PlayStation = 1, Xbox = 2 };
struct State {
    std::uint32_t size;
    std::uint32_t version;
    std::uint32_t flags;
    std::uint32_t inputFrame;
    std::uint32_t pressed;
    std::uint32_t glyphSet;
    std::uint16_t horizontal;
    std::uint16_t vertical;
    std::uint16_t previousPage;
    std::uint16_t nextPage;
    std::uint16_t back;
    std::uint16_t reserved;
};
static_assert(sizeof(State) == 36, "Unexpected GInput UI ABI packing");
using Query = int (__cdecl*)(std::uint32_t version, State* state, std::uint32_t size);
}
