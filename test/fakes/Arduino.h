#pragma once
// Host stand-in for the Arduino core: just what src/ uses.
#include <cmath>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sim_hw.h"

inline uint32_t millis() { return sim::hw::millis(); }
inline void     delay(uint32_t ms) { sim::hw::delay(ms); }
inline uint32_t esp_random() { return sim::hw::random(); }
inline void     setCpuFrequencyMhz(uint32_t) {}

#define constrain(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

// The same answer on every host: the maths library's float sine and cosine
// can differ in the last bit between macOS and Linux, which is enough to move
// a pixel and break a stored trace.
static inline float sim_cosf(float a) { return (float)cos((double)a); }
static inline float sim_sinf(float a) { return (float)sin((double)a); }
#define cosf sim_cosf
#define sinf sim_sinf

struct SerialFake {
  void   begin(unsigned long) {}
  void   setTxTimeoutMs(uint32_t) {}
  __attribute__((format(printf, 2, 3))) int printf(const char*, ...) { return 0; }
  void   print(const char*) {}
  void   println(const char*) {}
  int    available() { return 0; }
  int    read() { return -1; }
  size_t write(const uint8_t*, size_t n) { return n; }
  void   flush() {}
};
extern SerialFake Serial;
