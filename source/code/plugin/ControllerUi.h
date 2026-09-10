#pragma once
#include "../../integration/ManhuntGInputUI.h"
#include <string>

namespace ControllerUi {
using State = ManhuntGInputUI::State;
State Read();
State ReadFrom(ManhuntGInputUI::Query query);
bool Active(const State& state);
std::wstring Button(const State& state, std::uint16_t glyph, const wchar_t* fallback);
int PageSelection(int selection, int direction, int count, int perPage);
class PageInput {
    bool seen = false;
    std::uint32_t lastFrame = 0;
public:
    void Reset(const State& state);
    int Consume(const State& state);
};
}
