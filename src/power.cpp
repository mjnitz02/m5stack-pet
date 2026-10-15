#include "power.h"
#include "app.h"
#include "pet.h"
#include "petlog.h"
#if defined(BOARD_STICKS3)
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#endif

// Awake minutes persist every 5 min so a run-to-empty battery test still
// leaves a usable number behind (worst case it under-reports by 5 min).
static uint32_t awakeMarkMs = 0;
static uint8_t  awakeUnsaved = 0;
int      batMv = 0;
static uint32_t lastBatMs = 0;

void awakeRestart(uint32_t mark) { awakeMarkMs = mark; }

// Steps down at once, but only back up once the reading is clearly over the
// line, so a beep sagging the voltage can't make it flicker.
static BatLevel batShown = BAT_UNKNOWN;
static BatLevel levelFor(int mv) {
  return mv >= BAT_PART_MV ? BAT_FULL : mv >= BAT_LOW_MV ? BAT_PART : BAT_LOW;
}
BatLevel batLevel() { return batShown; }

int batPct() {
  int pct = (batMv - 3200) / 10;
  return pct < 0 ? 0 : pct > 100 ? 100 : pct;
}

void powerTick(uint32_t now) {
  if (now - awakeMarkMs >= 60000) {
    awakeMarkMs += 60000;
    pet().awakeMin++;
    if (++awakeUnsaved >= 5) { petSave(); awakeUnsaved = 0; }
  }
  if (lastBatMs == 0 || now - lastBatMs >= 10000) {
    lastBatMs = now;
    batMv = M5.Power.getBatteryVoltage();
    if (batMv > 0) {
      BatLevel now = levelFor(batMv), clear = levelFor(batMv - BAT_RISE_MV);
      if (batShown == BAT_UNKNOWN || now < batShown) batShown = now;
      else if (clear > batShown) batShown = clear;
    }
    Serial.printf("bat=%dmV awake=%lum\n", batMv, (unsigned long)pet().awakeMin);
  }
}

#if defined(BOARD_STICKS3)
// Deep sleep only stops the ESP32: every other chip on the board stays in
// whatever state it was left in, so each one is switched off by hand first.
// M5.begin() brings all of it back on the next boot, bar the light: see powerBoardSetup().
static void railsDown() {
  M5.Speaker.end();                                      // amp enable (PM1 GPIO3) low
  M5.In_I2C.writeRegister8(0x18, 0x12, 0x02, 100000);    // ES8311 codec: DAC off
  M5.In_I2C.writeRegister8(0x18, 0x13, 0x00, 100000);    //   output drive off
  M5.In_I2C.writeRegister8(0x18, 0x0D, 0xFC, 100000);    //   analog off
  M5.In_I2C.writeRegister8(0x18, 0x00, 0x00, 100000);    //   clocks off
  M5.In_I2C.writeRegister8(0x69, 0x7D, 0x00, 400000);    // BMI270: accel + gyro off
  M5.In_I2C.writeRegister8(0x69, 0x7C, 0x01, 400000);    //   suspend
  M5.Power.setExtOutput(false);                          // 5 V boost (Grove port)
  M5.Power.M5pm1.setGPIOOutput(m5::M5PM1_Class::gpio2, false);   // LCD rail
  M5.Power.M5pm1.setLedEnLevel(false);                   // green light by the power button
}
#endif

void powerOffNow() {
  M5.Display.sleep();
#if defined(BOARD_STICKS3)
  railsDown();   // for the USB case below, where the PMIC stays up
  // On USB the PMIC can't cut the rail, so powerOff() lands in deep sleep
  // instead. Arm the front button (GPIO11) so that case is still wakeable.
  rtc_gpio_pullup_en(GPIO_NUM_11);
  rtc_gpio_pulldown_dis(GPIO_NUM_11);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_11, 0);
#endif
  M5.Power.powerOff();
}

RTC_DATA_ATTR NapCarry napCarry;

void napDown() {
  petSave();
#if defined(BOARD_STICKS3)
  uint32_t now = millis();
  napCarry = { now - awakeMarkMs, now - lastHungerMs, now - lastHappyMs };
  logRow("nap");
  M5.Display.sleep();
  railsDown();
  rtc_gpio_pullup_en(GPIO_NUM_11);
  rtc_gpio_pulldown_dis(GPIO_NUM_11);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_11, 0);
  esp_sleep_enable_timer_wakeup(NAP_MS * 1000ULL);
  esp_deep_sleep_start();
#else
  logRow("off");
  powerOffNow();
#endif
}

Wake powerWake() {
#if defined(BOARD_STICKS3)
  esp_sleep_wakeup_cause_t woke = esp_sleep_get_wakeup_cause();
  if (woke == ESP_SLEEP_WAKEUP_TIMER) return WAKE_TIMER;
  if (woke == ESP_SLEEP_WAKEUP_EXT0)  return WAKE_BUTTON;
#endif
  return WAKE_COLD;
}

void powerBoardSetup() {
#if defined(BOARD_STICKS3)
  // The PM1 keeps its settings through a nap and M5.begin() leaves this one
  // alone, so the light railsDown() put out is lit again here.
  M5.Power.M5pm1.setLedEnLevel(true);
  // Only the accelerometer is ever read; M5.begin() starts the gyro as well.
  M5.In_I2C.writeRegister8(0x69, 0x7D, 0x04, 400000);    // BMI270: accel only
#endif
  // No radio and 5 fps animation: 80 MHz is plenty and roughly halves the
  // CPU's share of the battery. (Lowest speed that keeps USB alive.)
  setCpuFrequencyMhz(80);
}
