#pragma once
#include "compat.h"

// What every file shares: the sprite everything is drawn into, the list of
// screens and which one is up, and the pet's need timers. main.cpp and the
// screen headers it includes own all of it.

extern TFT_eSprite spr;

const int W = 135, H = 240;
const int CX = W / 2;

enum PetState { P_SLEEP, P_IDLE, P_PLAY, P_ATTENTION, P_CELEBRATE, P_DIZZY, P_HEART };
// One row each in main.cpp's SCREENS table, in this order.
enum Mode     { M_EGG, M_HOME, M_FOOD, M_FEED, M_PLAY, M_BED, M_PARENT, M_MENU, M_REWARD, M_CAL, M_LEARN,
                M_WALK, M_BATH, M_ROCK, M_FREEZE, M_DRESS, M_STICKERS,
                M_DRAW, M_MARBLE, M_COPY, M_HIDE, M_DANCE, M_N };
extern Mode mode;
const char* screenName();        // the screen it is on, for the log and the tests

// Whether now is still short of a time set earlier as millis() plus a wait.
// Signed, so it stays right when millis() wraps round.
static inline bool before(uint32_t now, uint32_t until) { return (int32_t)(now - until) < 0; }

// When each need last dropped a pip (see home.h).
extern uint32_t lastHungerMs, lastHappyMs;
