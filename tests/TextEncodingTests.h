#pragma once
#include "../source/code/plugin/AchievementTextEncoding.h"

#include <initializer_list>
#include <iostream>
#include <stdexcept>

// The gallery is Russian only when the game's menu is. Accented Latin letters
// sit where the Russian repack keeps its widened CP1251 bytes, so a Portuguese
// menu with "CONFIGURAÇÖES" and a button name such as "TRIÂNGULO" beside it
// once turned the gallery into Russian written in a Latin font.
inline void TestTextEncoding()
{
    namespace E = AchievementTextEncoding;
    const auto classify = [](std::initializer_list<const wchar_t*> strings) {
        E::LetterCounts counts;
        for (const wchar_t* text : strings) E::CountLetters(text, counts);
        return E::Classify(counts);
    };
    const auto require = [](bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    };

    // English and the other shipped banks.
    require(classify({L"PLAY", L"LOAD GAME", L"SETTINGS", L"QUIT", L"MAIN MENU"}) ==
        E::Kind::Latin, "English must stay English");
    require(classify({L"JOUER", L"PARAMÈTRES", L"MENU PRINCIPAL"}) == E::Kind::Latin,
        "French accents are not Russian");
    // A Portuguese fan translation, and with a translated button name added.
    require(classify({L"JOGAR", L"CARREGAR JOGO", L"CONFIGURAÇÖES", L"SAIR",
        L"MENU PRINCIPAL", L"TRIÂNGULO Voltar"}) == E::Kind::Latin,
        "Portuguese with an accented button name is not Russian");
    // The Russian repack's widened CP1251 bytes: "ИГРАТЬ", "НАСТРОЙКИ".
    require(classify({L"ÈÃÐÀÒÜ",
        L"ÍÀÑÒÐÎÉÊÈ"}) ==
        E::Kind::WidenedCp1251Russian, "the Russian repack must be Russian");
    // Real Cyrillic, however little of it.
    require(classify({L"PLAY", L"ИГРАТЬ"}) ==
        E::Kind::UnicodeRussian, "Unicode Cyrillic must be Russian");
    // Nothing returned yet: English.
    require(classify({nullptr, nullptr}) == E::Kind::Latin, "no text must mean English");
    std::cout << "Gallery language from the menu text: PASS\n";
}
