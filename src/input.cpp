#include "input.h"
#include "compat.h"

uint32_t lastInteractMs = 0;
// Signed, because a screen that opens mid-frame stamps lastInteractMs with a
// millis() later than that frame's `now`.
bool idleFor(uint32_t now, uint32_t ms) { return (int32_t)(now - lastInteractMs) > (int32_t)ms; }
bool swallowA = false, swallowB = false;

void inputTaps(uint32_t now, bool* a, bool* b) {
  if (M5.BtnA.isPressed() || M5.BtnB.isPressed()) lastInteractMs = now;
  *a = M5.BtnA.wasReleased();
  *b = M5.BtnB.wasReleased();
  if (*a && swallowA) { *a = false; swallowA = false; }
  if (*b && swallowB) { *b = false; swallowB = false; }
}

bool heldBoth() {
  if (!M5.BtnA.pressedFor(HOLD_LONG_MS) || !M5.BtnB.pressedFor(HOLD_LONG_MS)) return false;
  swallowA = swallowB = true;
  return true;
}

bool heldSide() {
  if (swallowB || !M5.BtnB.pressedFor(HOLD_LONG_MS) || M5.BtnA.isPressed()) return false;
  swallowB = true;
  return true;
}

bool heldBlue() {
  if (swallowA || !M5.BtnA.pressedFor(HOLD_BLUE_MS) || M5.BtnB.isPressed()) return false;
  swallowA = true;
  return true;
}

// Read again first, as a tune may have played since the top of the frame.
void inputScreenChanged() {
  M5.update();
  if (M5.BtnA.isPressed()) swallowA = true;
  if (M5.BtnB.isPressed()) swallowB = true;
}
