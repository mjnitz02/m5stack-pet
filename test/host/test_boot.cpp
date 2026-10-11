// Getting from a blank stick to a pet, and what each kind of wake-up does.
#include "play.h"

using namespace sim;

TEST(new_stick_opens_on_the_lesson) {
  Device d;
  d.boot(Wake::Cold, [](Stick& s) {
    s.run(500);
    CHECK_EQ(probe::screen(), "learn");
    CHECK(!probe::hatched());
    CHECK(!probe::learned());
  });
  CHECK_EQ(d.nvs.get("pet/roster"), 1u);     // stamped on the first boot
  CHECK_EQ(d.nvs.get("pet/hatched"), 0u);
}

TEST(lesson_leads_to_the_egg_and_is_remembered) {
  Device d;
  d.boot(Wake::Cold, [](Stick& s) {
    finishLesson(s);
    CHECK_EQ(probe::screen(), "egg");
    CHECK(probe::learned());
  });
  CHECK_EQ(d.nvs.get("pet/learned"), 1u);
  d.boot(Wake::Cold, [](Stick& s) {
    s.run(500);
    CHECK_EQ(probe::screen(), "egg");          // not the lesson again
  });
}

TEST(lesson_left_alone_naps_and_starts_over) {
  Device d;
  Boot b = d.boot(Wake::Cold, [](Stick& s) { s.run(120000); });
  CHECK_EQ(b.end, End::Nap);
  CHECK_EQ(d.nvs.get("pet/learned"), 0u);
  d.boot(Wake::Button, [](Stick& s) {
    s.run(500);
    CHECK_EQ(probe::screen(), "learn");
  });
}

TEST(seven_taps_hatch_the_egg) {
  Device d = unhatchedEgg();
  d.boot(Wake::Cold, [](Stick& s) {
    s.run(500);
    CHECK_EQ(probe::screen(), "egg");
    for (int i = 0; i < 6; i++) s.tap(A);
    CHECK_EQ(probe::screen(), "egg");
    s.tap(B);                                  // either button counts
    CHECK_EQ(probe::screen(), "home");
    CHECK(probe::hatched());
    CHECK_EQ(probe::hunger(), 2);
    CHECK_EQ(probe::happy(), 2);
  });
  CHECK_EQ(d.nvs.get("pet/hatched"), 1u);
  CHECK(d.nvs.get("pet/species", 99) < 7u);
}

TEST(shaking_hatches_the_egg_too) {
  Device d = unhatchedEgg();
  d.boot(Wake::Cold, [](Stick& s) {
    s.run(1500);                               // let the motion baseline settle
    s.shake();
    CHECK(s.runUntil([] { return probe::screen() == "home"; }, 5000));
  });
}

TEST(a_new_egg_is_never_the_animal_it_was) {
  for (uint32_t seed = 1; seed <= 40; seed++) {
    Device d = unhatchedEgg();
    d.seed = seed;
    d.nvs.set("pet/species", 3);               // the dog was here before the egg
    d.boot(Wake::Cold, [](Stick& s) {
      for (int i = 0; i < 7; i++) s.tap(A);
      CHECK(probe::hatched());
    });
    CHECK(d.nvs.get("pet/species") != 3u);
    CHECK(d.nvs.get("pet/species") < 7u);
  }
}

TEST(an_egg_hatches_the_animal_a_parent_picked) {
  for (uint32_t seed = 1; seed <= 10; seed++) {
    Device d = unhatchedEgg();
    d.seed = seed;
    d.nvs.set("pet/pick", 2);                  // species + 1: the cat
    d.boot(Wake::Cold, [](Stick& s) {
      for (int i = 0; i < 7; i++) s.tap(A);
      CHECK_EQ(probe::species(), "cat");
    });
  }
}

TEST(cold_boot_wakes_a_little_hungry_and_bored) {
  Device d = hatchedPet(5, 3, 3);
  d.boot(Wake::Cold, [](Stick& s) {
    s.run(200);
    CHECK_EQ(probe::screen(), "home");
    CHECK_EQ(probe::species(), "octopus");
    CHECK_EQ(probe::hunger(), 2);
    CHECK_EQ(probe::happy(), 2);
  });
}

TEST(cold_boot_never_takes_needs_below_zero) {
  Device d = hatchedPet(5, 0, 0);
  d.boot(Wake::Cold, [](Stick& s) {
    s.run(200);
    CHECK_EQ(probe::hunger(), 0);
    CHECK_EQ(probe::happy(), 0);
  });
}

TEST(nap_wake_costs_nothing) {
  Device d = hatchedPet(5, 3, 3);
  d.boot(Wake::Button, [](Stick& s) {
    s.run(200);
    CHECK_EQ(probe::screen(), "home");
    CHECK_EQ(probe::hunger(), 3);
    CHECK_EQ(probe::happy(), 3);
  });
}

TEST(the_press_that_wakes_a_nap_does_not_select) {
  Device d = hatchedPet();
  d.blueHeldAtBoot = true;
  d.boot(Wake::Button, [](Stick& s) {
    s.run(100);
    s.release(A);
    s.run(200);
    CHECK_EQ(probe::screen(), "home");         // not the food tray the frame is on
    s.tap(A);                                  // the next press is an ordinary one
    CHECK_EQ(probe::screen(), "food");
  });
}

TEST(holding_the_waking_press_does_not_open_the_toy_box) {
  Device d = hatchedPet();
  d.blueHeldAtBoot = true;
  d.boot(Wake::Button, [](Stick& s) {
    s.run(1500);
    s.release(A);
    s.run(100);
    CHECK_EQ(probe::screen(), "home");
  });
}

TEST(nap_timer_wake_powers_straight_off) {
  Device d = hatchedPet(5, 3, 3);
  Boot b = d.boot(Wake::Timer, [](Stick&) { CHECK(false); });   // never gets as far as the script
  CHECK_EQ(b.end, End::Off);
  CHECK(b.buttonArmed);                        // still wakeable if it was on USB
  CHECK_EQ(d.nvs.get("pet/hunger"), 3u);       // nothing loaded, nothing saved
  CHECK(b.trace.find("brightness 0") != std::string::npos);
}
