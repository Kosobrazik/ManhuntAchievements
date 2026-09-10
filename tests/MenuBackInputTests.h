#pragma once
#include "../source/code/plugin/MenuBackInput.h"
#include <stdexcept>

inline void TestMenuBackInput()
{
    const auto check=[](bool ok) {
        if (!ok) throw std::runtime_error("Menu back transition regression");
    };
    MenuBackInput back;
    // Reproduce the reported sequence: mouse exit leaves a cached click behind,
    // then the user reopens the gallery. It must stay open, without a close sound.
    back.Reset(false,false,false);
    check(back.Consume(false,true,true));
    check(!back.Consume(false,true,true));
    back.Reset(false,true,true);
    check(!back.Consume(false,true,true));
    check(!back.Consume(false,true,false));
    check(!back.Consume(false,true,false));
    check(!back.Consume(false,false,false));
    check(back.Consume(false,true,true));

    // A new Escape / GInput B remains usable even if the pointer hit stayed set.
    back.Reset(false,true,false);
    check(back.Consume(true,true,false));
    check(!back.Consume(true,true,false));
    check(!back.Consume(false,true,false));
    check(back.Consume(true,true,false));

    // Opening while a key/click is held must not dismiss the new page.
    back.Reset(true,false,true);
    check(!back.Consume(true,true,true));
    check(!back.Consume(false,true,false));
    check(!back.Consume(false,false,false));
    check(back.Consume(true,true,false));
    check(!back.Consume(true,true,false));
    std::cout << "Menu reopen / cached Back / held input: PASS\n";
}
