#pragma once
#include "../source/code/plugin/AchievementTranslation.h"

#include <iostream>
#include <stdexcept>
#include <string>

// A gallery translation file is a trophy list much as a website prints it, with
// a Match line and the gallery's own words added in front. This is the
// Brazilian Portuguese list a tester brought, as it came.
inline void TestTranslationFile()
{
    namespace T = AchievementTranslation;
    const auto require = [](bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    };
    const char* file = u8R"(Match = TIPO DE CONTROLE
Gallery.Title = CONQUISTAS
Gallery.TitleCount = CONQUISTAS %d/%d
Gallery.Unlocked = CONQUISTA DESBLOQUEADA
Gallery.Back = voltar

Manhunt — Troféus

[PLATINA]

1. Psicopata
Descrição: Desbloqueie todos as conquistas.

[BRONZE]

2. Ele Nem Viu Chegando
Descrição: Faça uma execução horrível.

3. Assassino 5 Estrelas
Descrição: Consiga uma pontuação de 5 estrelas em qualquer fase.

4. O Melhor é Ficar Sem Drogas
Descrição: Conclua qualquer cena na dificuldade Extremo sem usar analgésicos.

5. Bang, Bang, Bum
Descrição: Mate um caçador atirando em um tanque explosivo.

6. Sem Guindaste, Sem Ganho
Descrição: Esmague um caçador com uma geladeira.

7. Tem Medo do Escuro?
Descrição: Esconda o corpo de uma vítima nas sombras.

8. Névoa Rosa
Descrição: Acerte um caçador na cabeça com um rifle de precisão.

9. Poder Mental
Descrição: Use uma cabeça decepada para atrair um caçador.

10. Eficiência Mortal
Descrição: Mate dois caçadores com um único cartucho de escopeta.

11. Uh, Uh, Ah, Ah!
Descrição: Jogue qualquer cena usando o traje de Macaco.

12. Siga o Coelho Branco
Descrição: Conclua qualquer cena usando o traje de Coelho.


[PRATA]

13. Morte(o) pelas Costas
Descrição: Conclua qualquer cena usando apenas execuções.

14. Fique nas Sombras
Descrição: Conclua qualquer cena sem ser detectado nenhuma vez.

15. Alinhe-os e Derrube-os
Descrição: Mate 30 caçadores em Duro como Prego.

16. Diversão na Pancadaria
Descrição: Sobreviva até que 30 caçadores tenham sido mortos no Clube da Luta.

17. Fuga do Macaco
Descrição: Escape vivo do Zoológico em Macaco vê, Macaco mata!

18. Você Não Vai a Lugar Nenhum!
Descrição: Mate todos os Encapuzados em H0ra d3 Morr3r em sete minutos ou menos.

19. Temporada de Caça
Descrição: Mate 45 caçadores em uma única cena.

20. Cortem as Cabeças!
Descrição: Execute 20 caçadores em uma única cena.

21. Duro como Prego
Descrição: Consiga pelo menos três estrelas nas cenas 1 a 5.

22. Clube da Luta
Descrição: Consiga pelo menos três estrelas nas cenas 6 a 10.

23. Macaco vê, Macaco mata!
Descrição: Consiga pelo menos três estrelas nas cenas 11 a 15.

24. H0ra d3 Morr3r
Descrição: Consiga pelo menos três estrelas nas cenas 16 a 20.

[OURO]

25. Sanguinário
Descrição: Conclua todas as cenas na dificuldade Extremo.

26. Perigoso
Descrição: Conclua todas as cenas na dificuldade Fetiche.

27. Aberração 4 Estrelas
Descrição: Consiga uma pontuação de 4 estrelas em todas as cenas na dificuldade Fetiche.

28. Monstro 5 Estrelas
Descrição: Consiga uma pontuação de 5 estrelas em todas as cenas na dificuldade Extremo.

[BRONZE]

29. Sujando as Mãos
Descrição: Mate 10 caçadores em uma única cena usando apenas os punhos.

30. Mandando Ver com o Taco
Descrição: Mate 10 caçadores em uma única cena usando um taco de beisebol.

31. O Ceifador
Descrição: Mate 10 caçadores em uma única cena usando uma foice.

32. Abate Sutil
Descrição: Faça uma execução usando a motosserra.

33. Um Presente Especial
Descrição: Leve a cabeça do caçador que está patrulhando até a Sala de Segurança durante "À Beira da Loucura".
)";
    // Windows line endings and a byte order mark, as Notepad saves it.
    std::string crlf = "\xEF\xBB\xBF";
    for (const char* p = file; *p; ++p)
    {
        if (*p == '\n') crlf += '\r';
        crlf += *p;
    }
    const T::Translation translation = T::Parse(crlf);
    require(translation.match == L"TIPO DE CONTROLE", "the Match line");
    require(translation.phrases[T::Title] == L"CONQUISTAS", "the gallery title");
    require(translation.phrases[T::TitleCount] == L"CONQUISTAS %d/%d", "the counter");
    require(translation.phrases[T::Back] == L"voltar", "the back hint");
    require(translation.phrases[T::Select].empty(), "a phrase left out stays empty");
    for (int index = 0; index < T::kAchievementCount; ++index)
    {
        require(!translation.names[index].empty() && !translation.descriptions[index].empty(),
            "every achievement named and described");
    }
    require(translation.names[0] == L"Psicopata", "the first name");
    require(translation.descriptions[0] == L"Desbloqueie todos as conquistas.",
        "the label in front of a description is dropped");
    require(translation.names[12] == L"Morte(o) pelas Costas", "a name with brackets");
    require(translation.names[32] == L"Um Presente Especial", "the last name");
    require(translation.descriptions[32].find(L"À Beira da Loucura") != std::wstring::npos,
        "accents and quotes survive");
    require(translation.count == 4 + 2 * T::kAchievementCount, "nothing counted twice");

    // A description may hold a colon of its own; only a one-word label goes.
    const T::Translation colon = T::Parse("7. Name\nKill a hunter: quietly.\n");
    require(colon.descriptions[6] == L"Kill a hunter: quietly.", "a colon in the text stays");
    // Numbers out of range and stray lines are ignored.
    const T::Translation stray = T::Parse("0. Nothing\n34. Too far\nJust text\n");
    require(stray.count == 0, "nothing outside 1..33");
    std::cout << "Gallery translation file: PASS\n";
}
