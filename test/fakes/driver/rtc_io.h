#pragma once
// Host stand-in for the RTC pin calls made before a nap.

enum gpio_num_t { GPIO_NUM_11 = 11 };

inline int rtc_gpio_pullup_en(gpio_num_t) { return 0; }
inline int rtc_gpio_pulldown_dis(gpio_num_t) { return 0; }
