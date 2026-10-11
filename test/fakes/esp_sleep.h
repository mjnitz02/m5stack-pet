#pragma once
// Host stand-in for the ESP-IDF sleep calls the StickS3 nap uses.
#include "sim_hw.h"

typedef enum {
  ESP_SLEEP_WAKEUP_UNDEFINED = 0,
  ESP_SLEEP_WAKEUP_EXT0 = 2,
  ESP_SLEEP_WAKEUP_TIMER = 4,
} esp_sleep_wakeup_cause_t;

inline esp_sleep_wakeup_cause_t esp_sleep_get_wakeup_cause() { return (esp_sleep_wakeup_cause_t)sim::hw::wakeCause(); }
inline int esp_sleep_enable_ext0_wakeup(int /*gpio*/, int /*level*/) {
  sim::hw::armButtonWake();
  return 0;
}
inline int esp_sleep_enable_timer_wakeup(uint64_t us) {
  sim::hw::armTimerWake(us);
  return 0;
}
[[noreturn]] inline void esp_deep_sleep_start() {
  sim::hw::event("deep sleep");
  throw sim::DeepSleep();
}
