#include "petlog.h"

#if defined(PET_LOG)
#include <LittleFS.h>
#include "app.h"
#include "input.h"
#include "motion.h"

static const char*    LOG_PATH = "/log.csv";
static const size_t   LOG_MAX  = 1024 * 1024;   // about 90 hours of rows, then it starts over
static const uint32_t LOG_TICK_MS = 10000;

static bool     _logOk = false;
static float    _logMotion = 0, _logTurn = 0;
static uint16_t _logBtnA = 0, _logBtnB = 0, _logFrames = 0;
static uint32_t _logMarkMs = 0;

void logRow(const char* event) {
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
           now / 1000.0f, event, screenName(),
           (int)M5.Power.getBatteryVoltage(), (int)M5.Power.getVBUSVoltage(),
           (unsigned)M5.Display.getBrightness(), _logMotion, _logTurn,
           (unsigned)_logBtnA, (unsigned)_logBtnB,
           span ? _logFrames * 1000.0f / span : 0.0f);
  f.close();
  _logMotion = _logTurn = 0;
  _logBtnA = _logBtnB = _logFrames = 0;
  _logMarkMs = now;
}

void logBegin(const char* bootEvent) {
  // A partition that has never held the log has to be formatted first: say
  // so rather than sit on a black screen.
  _logOk = LittleFS.begin(false);
  if (!_logOk) {
    M5.Display.setBrightness(128);
    M5.Display.setTextColor(0xFFFF, 0x0000);
    M5.Display.drawString("log: formatting", 8, 110);
    _logOk = LittleFS.begin(true);
    M5.Display.fillScreen(0x0000);
  }
  _logMarkMs = millis();
  logRow(bootEvent);
}

// `L` on the serial port dumps the log between two marker lines, `C` clears
// it. tools/pull_log.py is the other end. `0` and `1` set the PM1's LED_EN
// level on the StickS3.
static void logSerial() {
  while (Serial.available() > 0) {
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
#if defined(BOARD_STICKS3)
    } else if (c == '0' || c == '1') {
      // Bench test: flip the PM1's LED_EN level and report its power config
      // register (0x06), to see by eye which light that bit drives.
      M5.Power.M5pm1.setLedEnLevel(c == '1');
      Serial.printf("#PM1 PWR_CFG %02X\n", (unsigned)M5.Power.M5pm1.readRegister8(0x06));
      lastInteractMs = millis();
#endif
    }
  }
}

void logTick(uint32_t now) {
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

#endif
