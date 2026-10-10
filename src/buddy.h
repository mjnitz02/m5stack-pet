#pragma once
#include <stdint.h>

// Multi-species ASCII buddy renderer. Each species lives in its own
// src/buddies/<name>.cpp file and exposes 7 state functions matching
// the PetState enum order: sleep, idle, busy, attention, celebrate,
// dizzy, heart.
void buddyInit();
void buddyTick(uint8_t personaState);
void buddyInvalidate();
#include "compat.h"
void buddyRenderTo(TFT_eSPI* tgt, uint8_t personaState);
void buddySetSpeciesIdx(uint8_t idx);
void buddySetPeek(bool peek);
uint8_t buddySpeciesIdx();
uint8_t buddySpeciesCount();
uint16_t buddyBodyColor();
// Dress-up. tint replaces the species body colour (0 = species default);
// hat is HAT_NONE..HAT_FLOWER and rides on the head through every animation.
enum { HAT_NONE, HAT_BOW, HAT_CROWN, HAT_PARTY, HAT_FLOWER, HAT_N };
void buddySetLook(uint16_t tint, uint8_t hat);
uint16_t buddyInkColor();
// Draw a hat centred on x with its bottom edge at y. k scales it (1 = icon).
void buddyDrawHat(TFT_eSPI* tgt, uint8_t hat, int x, int y, int k);
const char* buddySpeciesName();

// Per-species state function: takes the global tickCount and renders
// the buddy + any overlays for the current state into the shared sprite.
typedef void (*StateFn)(uint32_t t);

struct Species {
  const char* name;
  uint16_t bodyColor;
  StateFn states[7];   // index by PetState (0=sleep .. 6=heart)
};
