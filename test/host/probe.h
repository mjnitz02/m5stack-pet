#pragma once
#include <cstddef>
#include <string>

// The tests' only window into the firmware's own variables. Tests go through
// these and never name anything in src/ directly, so when the firmware is
// reorganised, firmware.cpp is the one file here that has to follow.
// Only valid inside a boot script.

namespace probe {

std::string screen();        // "home", "egg", "bed", "menu", "walk", ...
bool        hatched();
bool        learned();
bool        sound();
int         hunger();
int         happy();
int         stickers();      // how many have been earned
std::string species();       // the animal on screen
int         speciesCount();
int         menuTile();      // the tile the toy box frame is on
int         batLevel();      // 0 recharge me, 1 some left, 2 full, 3 not read yet

// What a player would read off the screen in the games that need looking at.
void        marbleOffset(int* dx, int* dy);   // from the ball to the apple, in pixels
int         copyRound();                      // 0-based
std::string copySequence();                   // this round's pattern, 'A' blue and 'B' side
int         hideSpot();                       // the bush the pet is behind, 0..2
int         hideFrame();                      // the bush the frame is on

// The firmware's variables that a nap keeps and a power-off loses. The
// simulator copies them out at the end of a boot and back in before the next.
void napMemory(void** at, size_t* len);

// Run one animal's animation for a state (0 sleep .. 6 heart) for so many
// animation steps, away from the game. small is the size used beside a game.
void animate(int species, int state, int steps, bool small = false);

}  // namespace probe
