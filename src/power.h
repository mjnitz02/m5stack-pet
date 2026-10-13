#pragma once
#include <stdint.h>

// --- Battery budget ---------------------------------------------------------
// The backlight and the CPU are the only real loads once the radio is gone.
// Idle → the pet falls asleep on a dimmed screen → the screen goes off and
// the unit naps in deep sleep, where the blue button brings it straight
// back → the unit powers itself off. After that, waking is the side power
// button.
const uint32_t IDLE_BED_MS = 60000;   // no input this long → bedtime
const uint32_t BED_OFF_MS  = 15000;   // asleep this long → screen off, nap
#if defined(PET_LOG)
// Debug build: a nap long enough for its drain to show in the log overnight.
const uint32_t NAP_MS      = 12UL * 60 * 60 * 1000;
#else
const uint32_t NAP_MS      = 60UL * 60 * 1000;   // napping this long → power off
#endif
const uint8_t  BED_BRIGHT  = 12;

// Awake minutes persist every 5 min so a run-to-empty battery test still
// leaves a usable number behind (worst case it under-reports by 5 min).
extern int batMv;
int  batPct();
void powerTick(uint32_t now);
void awakeRestart(uint32_t mark);   // count the awake minutes from here

// Why the chip started: switched on, woken from a nap by the blue button, or
// woken by the nap's own timer, which only wakes it to switch off properly.
enum Wake { WAKE_COLD, WAKE_BUTTON, WAKE_TIMER };
Wake powerWake();
void powerBoardSetup();             // once, after M5.begin()

// The real off switch. Saves nothing, so a nap that has run out can use it
// without loading the pet first.
void powerOffNow();

// Deep sleep keeps this memory and a power-off does not: how far each timer
// had got when the stick napped. A day of short sessions then wears the pet
// down as fast as one long one would. Time spent asleep does not count.
struct NapCarry { uint32_t awakeMs, hungerMs, happyMs; };
extern NapCarry napCarry;

// Screen off but not gone: deep sleep with the blue button armed, and a
// timer that wakes the chip just long enough to power off properly.
// Waking is a reboot either way; the pet comes back from flash.
void napDown();
