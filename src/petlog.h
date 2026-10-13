#pragma once
#include <stdint.h>

// Flight recorder for debug builds (`make flash-debug`). An ordinary build
// compiles all of this away.
//
// Rows go to /log.csv on the LittleFS partition, so the stick can be played
// with off the cable and read back later with `make log`. One row every ten
// seconds, plus one whenever the screen changes or the unit boots, naps or
// powers off:
//
//   t_s         seconds since this boot
//   event       boot:cold / boot:button / boot:timer, mode, tick, nap, off,
//               or slow:<step>:<ms> when one step of the loop took over a
//               second (the steps are named in loop(), in main.cpp)
//   mode        the screen it is on
//   bat_mv      battery voltage (the PMIC has no current sensor, so drain
//               has to be read off how fast this falls)
//   vbus_mv     USB voltage; over ~4000 the battery is charging and bat_mv
//               says nothing about drain
//   backlight   0-255
//   motion      biggest imuDelta since the last row (shake / walk strength)
//   turn        biggest imuTurn since the last row (tilt / rock strength)
//   btn_a,btn_b presses since the last row
//   fps         passes of the loop per second since the last row (only a
//               picture that changed is sent to the screen)

#if defined(PET_LOG)

void logBegin(const char* bootEvent);
void logRow(const char* event);
void logTick(uint32_t now);   // once per frame, after the buttons and the IMU have been read
void logStep(const char* name);   // the loop is starting this step; the one before it has ended

#else

static inline void logBegin(const char*) {}
static inline void logRow(const char*) {}
static inline void logTick(uint32_t) {}
static inline void logStep(const char*) {}

#endif
