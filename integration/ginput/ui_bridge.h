// Included at the end of dllmain.cpp. Uses the already-polled game-thread state.
extern "C" __declspec(dllexport) int __cdecl ManhuntGInput_GetUiState(
    std::uint32_t version, ManhuntGInputUI::State* state, std::uint32_t size)
{
    namespace UI = ManhuntGInputUI;
    if (version != UI::Version || !state || size < sizeof(UI::State)) return 0;
    *state = {};
    if (!g_uiBridgeReady.load(std::memory_order_acquire)) return 0;
    state->size = sizeof(UI::State);
    state->version = UI::Version;
    state->inputFrame = g_uiInputFrame;
    state->glyphSet = g_configuration.glyphSet == GlyphSet::PlayStation ? UI::PlayStation :
        (g_configuration.glyphSet == GlyphSet::Xbox ? UI::Xbox : UI::TextOnly);
    const bool active = g_configuration.enabled && g_configuration.enableFrontend &&
        g_controllerConnected && g_gamepadUiActive &&
        MainModuleField<int>(kInputEnabledRva) != 0 && IsFrontendActive();
    if (!active) return 1;
    state->flags = UI::ControllerActive;
    if (g_configuration.glyphSet != GlyphSet::None) state->flags |= UI::InlineGlyphs;
    state->horizontal = 0xE000 + kGlyphDpadHorizontal;
    state->vertical = 0xE000 + kGlyphDpadVertical;
    state->previousPage = 0xE000 + kGlyphL1;
    state->nextPage = 0xE000 + kGlyphR1;
    state->back = 0xE000 + kGlyphCircle;
    const WORD pressed = g_currentControllerState.Gamepad.wButtons &
        ~g_previousControllerState.Gamepad.wButtons;
    if (pressed & XINPUT_GAMEPAD_LEFT_SHOULDER) state->pressed |= UI::PreviousPage;
    if (pressed & XINPUT_GAMEPAD_RIGHT_SHOULDER) state->pressed |= UI::NextPage;
    return 1;
}
