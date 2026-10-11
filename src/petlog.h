#pragma once

// Flight recorder for debug builds (`make flash-debug`). An ordinary build
// compiles all of this away.
//
// Rows go to /log.csv on the LittleFS partition, so the stick can be played
// with off the cable and read back later with `make log`. One row every ten
// seconds, plus one whenever the screen changes or the unit boots, naps or
// powers off:
//
//   t_s         seconds since this boot
//   event       boot:cold / boot:button / boot:timer, mode, tick, nap, off
//   mode        the screen it is on
//   bat_mv      battery voltage (the PMIC has no current sensor, so drain
//               has to be read off how fast this falls)
//   vbus_mv     USB voltage; over ~4000 the battery is charging and bat_mv
//               says nothing about drain
//   backlight   0-255
//   motion      biggest imuDelta since the last row (shake / walk strength)
//   turn        biggest imuTurn since the last row (tilt / rock strength)
//   btn_a,btn_b presses since the last row
//   fps         frames drawn per second since the last row
//
// Included from main.cpp after the globals it reads.

#if defined(PET_LOG)
#include <LittleFS.h>

static const char*    LOG_PATH = "/log.csv";
static const size_t   LOG_MAX  = 1024 * 1024;   // about 90 hours of rows, then it starts over
static const uint32_t LOG_TICK_MS = 10000;

static bool     _logOk = false;
static float    _logMotion = 0, _logTurn = 0;
static uint16_t _logBtnA = 0, _logBtnB = 0, _logFrames = 0;
static uint32_t _logMarkMs = 0;

static void logRow(const char* event) {
  if (!_logOk) return;
  File f = LittleFS.open(LOG_PATH, FILE_APPEND);
  if (f && f.size() > LOG_MAX) {
    f.close();
    LittleFS.remove(LOG_PATH);
    f = LittleFS.open(LOG_PATH, FILE_APPEND);
  }
  if (!f) return;
  if (f.size() == 0) f.println("t_s,event,mode,bat_mv,vbus_mv,backlight,motion,turn,btn_a,btn_b,fps");
  uint32_t now = millis();
  uint32_t span = now - _logMarkMs;
  f.printf("%.1f,%s,%s,%d,%d,%u,%.2f,%.2f,%u,%u,%.1f\n",
           now / 1000.0f, event, MODE_NAME[mode],
           (int)M5.Power.getBatteryVoltage(), (int)M5.Power.getVBUSVoltage(),
           (unsigned)M5.Display.getBrightness(), _logMotion, _logTurn,
           (unsigned)_logBtnA, (unsigned)_logBtnB,
           span ? _logFrames * 1000.0f / span : 0.0f);
  f.close();
  _logMotion = _logTurn = 0;
  _logBtnA = _logBtnB = _logFrames = 0;
  _logMarkMs = now;
}

static void logBegin(const char* bootEvent) {
  _logOk = LittleFS.begin(true);
  _logMarkMs = millis();
  logRow(bootEvent);
}

// `L` on the serial port dumps the log between two marker lines, `C` clears
// it. tools/pull_log.py is the other end.
static void logSerial() {
  while (Serial.available()) {
    int c = Serial.read();
    if (c == 'L') {
#if defined(BOARD_STICKS3)
      Serial.setTxTimeoutMs(200);   // setup() made writes non-blocking, which would drop most of a dump
#endif
      File f = _logOk ? LittleFS.open(LOG_PATH, FILE_READ) : File();
      Serial.printf("#LOG BEGIN %u\n", f ? (unsigned)f.size() : 0u);
      uint8_t buf[256];
      while (f && f.available()) Serial.write(buf, f.read(buf, sizeof(buf)));
      if (f) f.close();
      Serial.println("#LOG END");
      Serial.flush();
#if defined(BOARD_STICKS3)
      Serial.setTxTimeoutMs(0);
#endif
      lastInteractMs = millis();    // don't nod off right after a long dump
    } else if (c == 'C') {
      if (_logOk) LittleFS.remove(LOG_PATH);
      Serial.println("#LOG CLEARED");
    }
  }
}

// Once per frame, after the buttons and the IMU have been read.
static void logTick(uint32_t now) {
  static int8_t seenMode = -1;
  _logFrames++;
  if (M5.BtnA.wasPressed()) _logBtnA++;
  if (M5.BtnB.wasPressed()) _logBtnB++;
  if (imuFresh) {
    if (imuDelta > _logMotion) _logMotion = imuDelta;
    if (imuTurn  > _logTurn)   _logTurn   = imuTurn;
  }
  logSerial();
  if (seenMode != (int8_t)mode) {
    seenMode = (int8_t)mode;
    logRow("mode");
  } else if (now - _logMarkMs >= LOG_TICK_MS) {
    logRow("tick");
  }
}

#else

static inline void logBegin(const char*) {}
static inline void logRow(const char*) {}
static inline void logTick(uint32_t) {}

#endif
