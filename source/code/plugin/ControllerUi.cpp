#include "ControllerUi.h"
#include <Windows.h>
#include <algorithm>

namespace ControllerUi {
State ReadFrom(ManhuntGInputUI::Query query)
{
    State state{};
    if (!query || !query(ManhuntGInputUI::Version, &state, sizeof(state)) ||
        state.version != ManhuntGInputUI::Version || state.size != sizeof(state))
        return {};
    return state;
}

State Read()
{
    // Discovery only: never load GInput as a dependency. Retry when it is absent
    // or still initializing so ASI load order does not affect optional support.
    HMODULE module = GetModuleHandleW(L"ManhuntGInput.asi");
    const auto query = module ? reinterpret_cast<ManhuntGInputUI::Query>(
        GetProcAddress(module, ManhuntGInputUI::ExportName)) : nullptr;
    return ReadFrom(query);
}

bool Active(const State& state)
{
    return (state.flags & ManhuntGInputUI::ControllerActive) != 0;
}

std::wstring ButtonFrom(const State& state, std::uint16_t glyph, const wchar_t* fallback,
    ManhuntGInputUI::ButtonNameQuery names)
{
    if (Active(state) && (state.flags & ManhuntGInputUI::InlineGlyphs) &&
        glyph >= 0xE000 && glyph <= 0xE013)
        return std::wstring(1, static_cast<wchar_t>(glyph));
    // Icons off: GInput spells the button the way its own menus do -- the
    // console's naming the player chose and the player's translation -- so the
    // gallery says "TRIANGLE" when Triangle is what goes back, not our guess.
    if (Active(state) && names)
    {
        if (const wchar_t* name = names(glyph))
            return name;
    }
    return fallback;
}

std::wstring Button(const State& state, std::uint16_t glyph, const wchar_t* fallback)
{
    // Discovered like the state itself; a GInput without the lookup keeps our
    // own spelling.
    HMODULE module = GetModuleHandleW(L"ManhuntGInput.asi");
    const auto names = module ? reinterpret_cast<ManhuntGInputUI::ButtonNameQuery>(
        GetProcAddress(module, ManhuntGInputUI::ButtonNameExport)) : nullptr;
    return ButtonFrom(state, glyph, fallback, names);
}

void PageInput::Reset(const State& state)
{
    seen = true;
    lastFrame = state.inputFrame;
}

int PageInput::Consume(const State& state)
{
    if (!Active(state)) { Reset(state); return 0; }
    if (seen && lastFrame == state.inputFrame) return 0;
    Reset(state);
    const bool previous = (state.pressed & ManhuntGInputUI::PreviousPage) != 0;
    const bool next = (state.pressed & ManhuntGInputUI::NextPage) != 0;
    return static_cast<int>(next) - static_cast<int>(previous);
}

int PageSelection(int selection, int direction, int count, int perPage)
{
    if (count <= 0 || perPage <= 0) return 0;
    selection = (std::max)(0, (std::min)(selection, count - 1));
    const int page = selection / perPage;
    const int next = (std::max)(0, (std::min)(page + direction, (count - 1) / perPage));
    return (std::min)(next * perPage + selection % perPage, count - 1);
}
}
