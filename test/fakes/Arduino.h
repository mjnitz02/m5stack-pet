#pragma once
// Host stand-in for the Arduino core: just what src/ uses.
#include <cmath>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sim_hw.h"

inline uint32_t millis() { return sim::hw::millis(); }
inline void     delay(uint32_t ms) { sim::hw::delay(ms); }
inline uint32_t esp_random() { return sim::hw::random(); }
inline void     setCpuFrequencyMhz(uint32_t) {}

// On the stick this puts a variable in the memory deep sleep keeps. Here the
// simulator carries it from one boot to the next: see probe::napMemory().
#define RTC_DATA_ATTR

#define constrain(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

// The same answer on every host: the maths library's float sine and cosine
// can differ in the last bit between macOS and Linux, which is enough to move
// a pixel and break a stored trace.
static inline float sim_cosf(float a) { return (float)cos((double)a); }
static inline float sim_sinf(float a) { return (float)sin((double)a); }
#define cosf sim_cosf
#define sinf sim_sinf

// Like the stick's USB port, nothing written before begin() goes anywhere.
// After it, each line is an event in the trace.
struct SerialFake {
  void   begin(unsigned long baud) {
    _open = true;
    sim::hw::event("serial open %lu", baud);
  }
  void   setTxTimeoutMs(uint32_t) {}
  __attribute__((format(printf, 2, 3))) int printf(const char* fmt, ...) {
    char buf[200];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    print(buf);
    return n;
  }
  void   print(const char* s) {
    if (!_open) return;
    char buf[200];
    snprintf(buf, sizeof(buf), "%s", s);
    size_t n = strlen(buf);
    while (n && buf[n - 1] == '\n') buf[--n] = 0;
    sim::hw::event("serial %s", buf);
  }
  void   println(const char* s) { print(s); }
  int    available() { return 0; }
  int    read() { return -1; }
  size_t write(const uint8_t*, size_t n) { return n; }
  void   flush() {}
  bool   _open = false;
};
extern SerialFake Serial;
