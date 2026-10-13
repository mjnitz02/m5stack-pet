#pragma once
#include <stdint.h>

// The accelerometer, three ways: how hard the stick is being moved, which
// way it is tilted, and whether it is lying on its face.

// One motion sample every 50 ms, shared by everything that cares about
// movement.
extern float imuDelta;      // how far the pull on the stick is from resting 1 g
extern float imuTurn;       // how far the pull has swung round: this one notices gentle movement
extern bool  imuFresh;      // true on the frame a sample was taken
const float SHAKE_DELTA = 0.8f;
void sampleMotion(uint32_t now);

// Tilt steering: how far the stick leans from the way it was held when
// tiltBegin() was called, along the screen's left/right and up/down.
extern float tiltX, tiltY;
void tiltBegin(uint32_t now);
bool tiltUpdate(uint32_t now);   // false while level is still being captured

bool isFaceDown();
void motionRaw(float* v);        // the sensor as it reads now, for the calibration screen
