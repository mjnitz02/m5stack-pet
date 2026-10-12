// The firmware itself, compiled for the computer against test/fakes/, and the
// probes that let tests read its state. See probe.h.
#include "../../src/main.cpp"

#include "probe.h"

namespace probe {

std::string screen() {
  static const char* const NAME[] = {
    "egg", "home", "food", "feed", "play", "bed", "parent", "menu", "reward", "cal", "learn",
    "walk", "bath", "rock", "freeze", "dress", "stickers",
    "draw", "marble", "copy", "hide", "dance" };
  static_assert(sizeof(NAME) / sizeof(NAME[0]) == M_DANCE + 1, "NAME is out of step with Mode");
  return NAME[mode];
}

bool hatched() { return pet().hatched; }
bool learned() { return pet().learned; }
bool sound() { return pet().sound; }
int  hunger() { return pet().hunger; }
int  happy() { return pet().happy; }
int  stickers() { return __builtin_popcount(pet().stickers); }
std::string species() { return buddySpeciesName(); }
int  speciesCount() { return buddySpeciesCount(); }
int  menuTile() { return menuSel; }

void marbleOffset(int* dx, int* dy) {
  *dx = mbAX - (int)mbX;
  *dy = mbAY - (int)mbY;
}
int copyRound() { return ccRound; }
std::string copySequence() {
  std::string s;
  for (int i = 0; i <= ccRound && i < COPY_ROUNDS; i++) s += ccSeq[i] ? 'B' : 'A';
  return s;
}
int hideSpot() { return hdSpot; }
int hideFrame() { return hdSel; }

void napMemory(void** at, size_t* len) {
  *at = &napCarry;
  *len = sizeof(napCarry);
}

void animate(int species, int state, int steps, bool small) {
  buddySetSpeciesIdx((uint8_t)species);
  buddySetPeek(small);
  for (int i = 0; i < steps; i++) {
    delay(200);
    buddyTick((uint8_t)state);
    spr.pushSprite(0, 0);
  }
}

}  // namespace probe
