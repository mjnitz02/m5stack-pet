#pragma once
#include <stddef.h>
#include <stdint.h>

// What the fake hardware headers in this directory call into. The other
// side is test/host/sim.cpp, which holds the simulated stick: its clock, its
// buttons, its accelerometer, its saved settings and the record of what the
// firmware did.

namespace sim {

// Thrown where the real call never returns.
struct DeepSleep {};
struct PowerOff {};

namespace hw {

uint32_t millis();
void     delay(uint32_t ms);
uint32_t random();

bool button(int which);                      // 0 = A (blue), 1 = B (side)
void accel(float* x, float* y, float* z);
int  batteryMv();
int  vbusMv();
int  wakeCause();                            // 0 cold, 2 button (ext0), 4 timer

void setBrightness(int v);
int  brightness();

// Something worth reading in a trace: a beep, a save, a power event.
void event(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
// One drawing call. These are folded into a checksum per simulated second.
void draw(const char* target, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
void pushed();                               // a finished frame went to the screen

void armButtonWake();
void armTimerWake(uint64_t us);

size_t nvsLen(const char* ns, const char* key);                        // 0 = not there
bool   nvsRead(const char* ns, const char* key, void* out, size_t len);
void   nvsWrite(const char* ns, const char* key, const void* in, size_t len);
void   nvsClear(const char* ns);

}  // namespace hw
}  // namespace sim
