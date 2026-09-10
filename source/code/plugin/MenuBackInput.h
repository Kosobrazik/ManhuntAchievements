#pragma once

// The native info-bar click is cached until PrintInfo runs again. It can outlive
// the page that produced it; keyboard and info-bar edges must be tracked apart.
class MenuBackInput {
    bool keyHeld=false;
    bool infoHeld=false;
    bool waitForMouseRelease=false;
public:
    void Reset(bool keyDown, bool infoClick, bool mouseDown) {
        keyHeld=keyDown;
        infoHeld=infoClick;
        waitForMouseRelease=mouseDown;
    }
    bool Consume(bool keyDown, bool infoClick, bool mouseDown) {
        const bool keyPressed=keyDown && !keyHeld;
        bool infoPressed=infoClick && !infoHeld;
        keyHeld=keyDown;
        infoHeld=infoClick;
        if (waitForMouseRelease) {
            infoPressed=false;
            if (!mouseDown) waitForMouseRelease=false;
        }
        return keyPressed || infoPressed;
    }
};
