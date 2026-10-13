#pragma once
// Host stand-in for M5Unified and M5GFX: just what src/ uses. Nothing is
// drawn. Every call is reported to the simulator, which keeps a checksum of
// the drawing and a readable list of everything else.
#include <stdio.h>

#include "Arduino.h"

enum { TL_DATUM = 0, TC_DATUM = 1 };

namespace lgfx {

class LGFXBase {
 public:
  explicit LGFXBase(const char* tag) : _tag(tag) {}

  void fillScreen(int c) { sim::hw::draw(_tag, "fill %04X", c); }
  void fillSprite(int c) { sim::hw::draw(_tag, "fill %04X", c); }
  void fillRect(int x, int y, int w, int h, int c) { sim::hw::draw(_tag, "fillRect %d %d %d %d %04X", x, y, w, h, c); }
  void drawRect(int x, int y, int w, int h, int c) { sim::hw::draw(_tag, "rect %d %d %d %d %04X", x, y, w, h, c); }
  void fillCircle(int x, int y, int r, int c) { sim::hw::draw(_tag, "fillCircle %d %d %d %04X", x, y, r, c); }
  void drawCircle(int x, int y, int r, int c) { sim::hw::draw(_tag, "circle %d %d %d %04X", x, y, r, c); }
  void fillEllipse(int x, int y, int rx, int ry, int c) { sim::hw::draw(_tag, "fillEllipse %d %d %d %d %04X", x, y, rx, ry, c); }
  void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, int c) {
    sim::hw::draw(_tag, "fillTri %d %d %d %d %d %d %04X", x0, y0, x1, y1, x2, y2, c);
  }
  void drawLine(int x0, int y0, int x1, int y1, int c) { sim::hw::draw(_tag, "line %d %d %d %d %04X", x0, y0, x1, y1, c); }
  void drawFastHLine(int x, int y, int w, int c) { sim::hw::draw(_tag, "hline %d %d %d %04X", x, y, w, c); }
  void drawPixel(int x, int y, int c) { sim::hw::draw(_tag, "pixel %d %d %04X", x, y, c); }
  void drawRoundRect(int x, int y, int w, int h, int r, int c) {
    sim::hw::draw(_tag, "roundRect %d %d %d %d %d %04X", x, y, w, h, r, c);
  }
  void fillRoundRect(int x, int y, int w, int h, int r, int c) {
    sim::hw::draw(_tag, "fillRoundRect %d %d %d %d %d %04X", x, y, w, h, r, c);
  }

  // Text settings are not recorded as calls. Each piece of text is recorded
  // with the settings it was drawn under, so setting the same size twice, or
  // in another order, is not a difference.
  void setTextSize(int s) { _size = s; }
  void setTextColor(int fg, int bg) { _fg = fg; _bg = bg; }
  void setTextDatum(int d) { _datum = d; }
  void setCursor(int x, int y) { _cx = x; _cy = y; }
  void drawString(const char* s, int x, int y) {
    sim::hw::draw(_tag, "string %d %d %d s%d %04X/%04X \"%s\"", _datum, x, y, _size, _fg, _bg, s);
  }
  void print(const char* s) {
    sim::hw::draw(_tag, "text %d %d s%d %04X/%04X \"%s\"", _cx, _cy, _size, _fg, _bg, s);
    _cx += (int)strlen(s) * 6 * _size;
  }
  void print(char ch) {
    char s[2] = { ch, 0 };
    print(s);
  }
  __attribute__((format(printf, 2, 3))) void printf(const char* fmt, ...) {
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    print(buf);
  }

  uint16_t color565(uint8_t r, uint8_t g, uint8_t b) { return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3); }

 protected:
  const char* _tag;
  int _size = 1, _fg = 0xFFFF, _bg = 0, _datum = TL_DATUM, _cx = 0, _cy = 0;
};

}  // namespace lgfx

class M5GFX : public lgfx::LGFXBase {
 public:
  M5GFX() : LGFXBase("lcd") {}
  void    setRotation(int) {}
  void    setBrightness(uint8_t v) { sim::hw::setBrightness(v); }
  uint8_t getBrightness() const { return (uint8_t)sim::hw::brightness(); }
  void    sleep() {
    sim::hw::setBrightness(0);
    sim::hw::event("display sleep");
  }
};

class M5Canvas : public lgfx::LGFXBase {
 public:
  // The firmware builds its sprite on M5.Lcd before main() runs, so M5 has to
  // have been built by then. Only some compilers notice when it has not.
  explicit M5Canvas(lgfx::LGFXBase* parent) : LGFXBase("spr") {
    if (!parent) {
      fprintf(stderr, "M5Canvas made before M5: M5 must be defined ahead of the firmware\n");
      abort();
    }
  }
  void* createSprite(int, int) { return this; }
  void  pushSprite(int, int) { sim::hw::pushed(); }
  // No pixels are kept here. What stands in for them is a checksum of the
  // drawing calls since the buffer was last asked for, which the firmware
  // does once a frame. Two frames running that draw the same things then
  // read as the same picture, which they are: drawing over what is already
  // there changes nothing.
  void*  getBuffer() {
    _pixels = sim::hw::drawnSinceAsked();
    return &_pixels;
  }
  size_t bufferLength() const { return sizeof(_pixels); }

 private:
  uint64_t _pixels = 0;
};

namespace m5 {

struct Button_Class {
  bool isPressed() const { return cur; }
  bool wasPressed() const { return cur && !prev; }
  bool wasReleased() const { return !cur && prev; }
  bool pressedFor(uint32_t ms) const { return cur && sim::hw::millis() - since >= ms; }
  void latch(bool raw) {
    prev = cur;
    cur = raw;
    if (cur != prev) since = sim::hw::millis();
  }
  bool     cur = false, prev = false;
  uint32_t since = 0;
};

struct IMU_Class {
  bool getAccel(float* x, float* y, float* z) {
    sim::hw::accel(x, y, z);
    return true;
  }
};

struct Speaker_Class {
  void tone(float freq, uint32_t dur) { sim::hw::event("beep %u %u", (unsigned)freq, (unsigned)dur); }
  void setVolume(uint8_t) {}
  void end() { sim::hw::event("speaker off"); }
};

struct M5PM1_Class {
  enum gpio_t { gpio0, gpio1, gpio2, gpio3 };
  void    setGPIOOutput(gpio_t pin, bool on) { sim::hw::event("pm1 gpio%d %d", (int)pin, (int)on); }
  void    setLedEnLevel(bool on) { sim::hw::event("pm1 led %d", (int)on); }
  uint8_t readRegister8(uint8_t) { return 0; }
};

struct Power_Class {
  int16_t getBatteryVoltage() { return (int16_t)sim::hw::batteryMv(); }
  int16_t getVBUSVoltage() { return (int16_t)sim::hw::vbusMv(); }
  void    setExtOutput(bool on) { sim::hw::event("ext output %d", (int)on); }
  void    powerOff() {
    sim::hw::event("power off");
    throw sim::PowerOff();
  }
  M5PM1_Class M5pm1;
};

struct I2C_Class {
  bool writeRegister8(uint8_t addr, uint8_t reg, uint8_t val, uint32_t) {
    sim::hw::event("i2c %02X %02X=%02X", addr, reg, val);
    return true;
  }
};

struct M5Unified {
  M5GFX         Display;
  M5GFX&        Lcd = Display;
  Button_Class  BtnA, BtnB;
  IMU_Class     Imu;
  Speaker_Class Speaker;
  Power_Class   Power;
  I2C_Class     In_I2C;

  void begin() { sim::hw::event("m5 begin"); }
  void update() {
    BtnA.latch(sim::hw::button(0));
    BtnB.latch(sim::hw::button(1));
  }
};

}  // namespace m5

extern m5::M5Unified M5;
