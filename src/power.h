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
// Napping this long → power off. Long enough to last the night, so the blue
// button still wakes it in the morning; with the green light off a nap costs
// a few mV an hour (two sticks, overnight, 2026-10-13).
const uint32_t NAP_MS      = 12UL * 60 * 60 * 1000;
const uint8_t  BED_BRIGHT  = 12;

// Awake minutes persist every 5 min so a run-to-empty battery test still
// leaves a usable number behind (worst case it under-reports by 5 min).
extern int batMv;
int  batPct();

// Charge as three coarse states, like a handheld console: full, some left,
// recharge me. Finer steps would just read as "draining" during play. Lines
// come from one awake run to empty at low backlight (2026-10-15, 145 min):
// the voltage fell steadily, about 7 mV a minute, to 3.65 V, then slower to
// 3.5 V, then dropped off a cliff in the last 20 minutes. That splits it into
// about an hour full, 40 minutes some left, and 40 minutes of recharge me.
// Thresholds are read with the screen on, which sags the reading a bit.
enum BatLevel : uint8_t { BAT_LOW, BAT_PART, BAT_FULL, BAT_UNKNOWN };
const int BAT_PART_MV = 3750;   // below this: some left
const int BAT_LOW_MV  = 3550;   // below this: recharge me
const int BAT_RISE_MV = 50;     // a level only comes back this far above its line
BatLevel batLevel();
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
