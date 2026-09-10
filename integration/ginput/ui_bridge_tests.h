namespace {
void TestExternalUiBridge() {
    namespace UI = ManhuntGInputUI;
    Reset();
    UI::State state{};
    g_uiBridgeReady.store(false);
    assert(!ManhuntGInput_GetUiState(UI::Version, &state, sizeof(state)));
    g_uiBridgeReady.store(true);
    MainModuleField<int>(kCurrentFrontendMenuRva) = 24;
    assert(!ManhuntGInput_GetUiState(999, &state, sizeof(state)));
    assert(!ManhuntGInput_GetUiState(UI::Version, nullptr, sizeof(state)));
    assert(!ManhuntGInput_GetUiState(UI::Version, &state, sizeof(state) - 1));
    g_configuration.glyphSet = GlyphSet::Xbox;
    g_uiInputFrame = 123;
    g_currentControllerState.Gamepad.wButtons = XINPUT_GAMEPAD_RIGHT_SHOULDER;
    assert(ManhuntGInput_GetUiState(UI::Version, &state, sizeof(state)));
    assert(state.version == UI::Version && state.size == sizeof(state));
    assert(state.flags == (UI::ControllerActive | UI::InlineGlyphs));
    assert(state.glyphSet == UI::Xbox && state.back == 0xE001);
    assert(state.nextPage == 0xE00B && state.previousPage == 0xE007);
    assert(state.inputFrame == 123 && state.pressed == UI::NextPage);
    UI::State again{};
    assert(ManhuntGInput_GetUiState(UI::Version, &again, sizeof(again)));
    assert(again.pressed == state.pressed); // Query does not consume another plugin's input.
    g_previousControllerState = g_currentControllerState;
    assert(ManhuntGInput_GetUiState(UI::Version, &state, sizeof(state)) && !state.pressed);
    g_configuration.glyphSet = GlyphSet::PlayStation;
    assert(ManhuntGInput_GetUiState(UI::Version, &state, sizeof(state)) && state.glyphSet == UI::PlayStation);
    g_configuration.glyphSet = GlyphSet::None;
    assert(ManhuntGInput_GetUiState(UI::Version, &state, sizeof(state)) && state.flags == UI::ControllerActive);
    g_gamepadUiActive = false;
    assert(ManhuntGInput_GetUiState(UI::Version, &state, sizeof(state)) && !state.flags && !state.pressed);
    g_gamepadUiActive = true;
    g_controllerConnected = false;
    assert(ManhuntGInput_GetUiState(UI::Version, &state, sizeof(state)) && !state.flags);
    g_controllerConnected = true;
    g_configuration.enableFrontend = false;
    assert(ManhuntGInput_GetUiState(UI::Version, &state, sizeof(state)) && !state.flags);
    g_configuration.enableFrontend = true;
    MainModuleField<int>(kCurrentFrontendMenuRva) = -1;
    assert(ManhuntGInput_GetUiState(UI::Version, &state, sizeof(state)) && !state.flags);
    std::puts("PASS: versioned UI bridge, Xbox/PS/text glyphs, input ownership, disconnect and page edges");
}
}
