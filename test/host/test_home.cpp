// The pet screen: its three icons, feeding, and needs running down.
#include "play.h"

using namespace sim;

TEST(home_icons_are_feed_dance_bed) {
  Device d = hatchedPet();
  d.boot(Wake::Button, [](Stick& s) {
    s.tap(A);
    CHECK_EQ(probe::screen(), "food");
  });
  d.boot(Wake::Button, [](Stick& s) {
    s.tap(B);
    s.tap(A);
    CHECK_EQ(probe::screen(), "dance");
  });
  d.boot(Wake::Button, [](Stick& s) {
    s.tap(B);
    s.tap(B);
    s.tap(A);
    CHECK_EQ(probe::screen(), "bed");
  });
  d.boot(Wake::Button, [](Stick& s) {
    for (int i = 0; i < 3; i++) s.tap(B);      // round to the first again
    s.tap(A);
    CHECK_EQ(probe::screen(), "food");
  });
}

TEST(feeding_fills_one_apple_and_saves) {
  Device d = hatchedPet(5, 2, 3);
  d.boot(Wake::Button, [](Stick& s) {
    s.tap(A);
    s.tap(A);
    CHECK_EQ(probe::screen(), "feed");
    CHECK_EQ(probe::hunger(), 2);              // not until the food lands
    CHECK(s.runUntil([] { return probe::screen() == "home"; }, 6000));
    CHECK_EQ(probe::hunger(), 3);
    CHECK_EQ(probe::happy(), 3);
  });
  CHECK_EQ(d.nvs.get("pet/hunger"), 3u);
}

TEST(feeding_a_full_pet_stays_full) {
  Device d = hatchedPet(5, 4, 3);
  d.boot(Wake::Button, [](Stick& s) {
    s.tap(A);
    s.tap(A);
    CHECK(s.runUntil([] { return probe::screen() == "home"; }, 6000));
    CHECK_EQ(probe::hunger(), 4);
  });
}

TEST(food_tray_remembers_the_last_food) {
  Device d = hatchedPet();
  d.boot(Wake::Button, [](Stick& s) {
    s.tap(A);
    s.tap(B);
    s.tap(B);                                  // apple, carrot, strawberry
    s.tap(A);
    CHECK(s.runUntil([] { return probe::screen() == "home"; }, 6000));
  });
  CHECK_EQ(d.nvs.get("pet/food"), 2u);
  d.boot(Wake::Button, [](Stick& s) {
    s.tap(A);
    s.tap(A);                                  // straight in: it opened on the strawberry
    CHECK(s.runUntil([] { return probe::screen() == "home"; }, 6000));
  });
  CHECK_EQ(d.nvs.get("pet/food"), 2u);
}

TEST(buttons_do_nothing_while_the_pet_eats) {
  Device d = hatchedPet(5, 2, 3);
  d.boot(Wake::Button, [](Stick& s) {
    s.tap(A);
    s.tap(A);
    s.tap(A);
    s.tap(B);
    CHECK_EQ(probe::screen(), "feed");
    CHECK(s.runUntil([] { return probe::screen() == "home"; }, 6000));
    CHECK_EQ(probe::hunger(), 3);
  });
}

TEST(food_tray_left_alone_goes_back_to_the_pet) {
  Device d = hatchedPet();
  d.boot(Wake::Button, [](Stick& s) {
    s.tap(A);
    s.run(14000);
    CHECK_EQ(probe::screen(), "food");
    s.run(2000);
    CHECK_EQ(probe::screen(), "home");
  });
}

TEST(needs_run_down_slowly_while_awake) {
  Device d = hatchedPet(5, 3, 3);
  d.nvs.set("pet/stayon", 1);                  // or it would be asleep long before
  d.boot(Wake::Button, [](Stick& s) {
    s.run(7 * 60000 + 50000);
    CHECK_EQ(probe::hunger(), 3);
    s.run(20000);                              // 8 minutes: an apple
    CHECK_EQ(probe::hunger(), 2);
    CHECK_EQ(probe::happy(), 3);
    s.run(4 * 60000);                          // 12 minutes: a heart
    CHECK_EQ(probe::happy(), 2);
    s.run(4 * 60000);                          // 16 minutes: the second apple
    CHECK_EQ(probe::hunger(), 1);
  });
}

TEST(needs_stop_at_zero) {
  Device d = hatchedPet(5, 1, 1);
  d.nvs.set("pet/stayon", 1);
  d.boot(Wake::Button, [](Stick& s) {
    s.run(40 * 60000);
    CHECK_EQ(probe::hunger(), 0);
    CHECK_EQ(probe::happy(), 0);
    CHECK_EQ(probe::screen(), "home");         // and nothing else happens
  });
}

TEST(low_battery_changes_nothing_but_the_picture) {
  Device d = hatchedPet();
  d.batteryMv = 3250;
  d.boot(Wake::Button, [](Stick& s) {
    s.run(30000);
    CHECK_EQ(probe::screen(), "home");
    s.tap(A);
    CHECK_EQ(probe::screen(), "food");
  });
}
