#pragma once
#include "../source/code/plugin/ControllerUi.h"
#include <stdexcept>

namespace {
ManhuntGInputUI::State fakeUi{};
int __cdecl FakeUiQuery(std::uint32_t, ManhuntGInputUI::State* state, std::uint32_t)
{
    *state = fakeUi;
    return 1;
}
void TestControllerUi()
{
    namespace UI = ManhuntGInputUI;
    const auto check = [](bool condition) {
        if (!condition) throw std::runtime_error("Optional controller UI regression");
    };
    check(!ControllerUi::Active(ControllerUi::ReadFrom(nullptr)));
    fakeUi = {};
    check(!ControllerUi::Active(ControllerUi::ReadFrom(FakeUiQuery)));
    fakeUi.size = sizeof(fakeUi);
    fakeUi.version = 999;
    fakeUi.flags = UI::ControllerActive;
    check(!ControllerUi::Active(ControllerUi::ReadFrom(FakeUiQuery)));
    fakeUi.version = UI::Version;
    fakeUi.flags |= UI::InlineGlyphs;
    fakeUi.back = 0xE001;
    auto state = ControllerUi::ReadFrom(FakeUiQuery);
    check(ControllerUi::Active(state));
    check(ControllerUi::Button(state, state.back, L"B") == L"\uE001");
    check(ControllerUi::Button(state, 0xFFFF, L"B") == L"B");
    state.flags = UI::ControllerActive;
    check(ControllerUi::Button(state, state.back, L"B") == L"B");
    state.flags = 0;
    check(ControllerUi::Button(state, state.back, L"Back") == L"Back");

    ControllerUi::PageInput pages;
    state.flags = UI::ControllerActive;
    state.inputFrame = 10;
    state.pressed = UI::NextPage;
    pages.Reset(state); // Do not carry a press into a freshly opened gallery.
    check(pages.Consume(state) == 0);
    ++state.inputFrame;
    check(pages.Consume(state) == 1);
    check(pages.Consume(state) == 0); // Several renders of one input frame.
    ++state.inputFrame;
    state.pressed = UI::NextPage | UI::PreviousPage;
    check(pages.Consume(state) == 0);
    ++state.inputFrame;
    state.flags = 0; // Mouse ownership or a disconnected controller.
    state.pressed = UI::PreviousPage;
    check(pages.Consume(state) == 0);
    state.flags = UI::ControllerActive;
    check(pages.Consume(state) == 0);
    ++state.inputFrame;
    check(pages.Consume(state) == -1);

    check(ControllerUi::PageSelection(0, -1, 33, 6) == 0);
    check(ControllerUi::PageSelection(2, 1, 33, 6) == 8);
    check(ControllerUi::PageSelection(29, 1, 33, 6) == 32);
    check(ControllerUi::PageSelection(32, 1, 33, 6) == 32);
    check(ControllerUi::PageSelection(30, -1, 33, 6) == 24);
    std::cout << "Optional controller UI/ABI/page navigation: PASS\n";
}
}
