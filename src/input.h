#pragma once
#include <stdint.h>

// The two buttons. A tap counts when the button is let go, so the
// both-buttons hold can be told apart from two ordinary presses. A button
// that has already done something while down is "swallowed": letting it go
// does nothing more.

const uint32_t HOLD_BLUE_MS = 700;    // blue held this long → toy box, or back to the pet
const uint32_t HOLD_LONG_MS = 1500;   // side alone → sound on/off; both → parent screen

// When anything last counted as someone playing: a button, or movement on
// the screens where moving is the game.
extern uint32_t lastInteractMs;
bool idleFor(uint32_t now, uint32_t ms);

extern bool swallowA, swallowB;

// Once a frame, after M5.update(): which buttons were let go.
void inputTaps(uint32_t now, bool* a, bool* b);

// The holds. Each is true once per hold and swallows what it used.
bool heldBoth();
bool heldSide();   // the side button alone
bool heldBlue();   // the blue button alone

// A button that is down when the screen changes was pressed for the old
// screen: its hold and its release do nothing on the new one.
void inputScreenChanged();
