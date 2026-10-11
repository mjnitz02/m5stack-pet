// Falling asleep, napping and powering off.
#include "play.h"

using namespace sim;

static bool has(const Boot& b, const char* text) { return b.trace.find(text) != std::string::npos; }

TEST(home_left_alone_goes_to_bed_then_naps) {
  Device d = hatchedPet();
  Boot b = d.boot(Wake::Button, [](Stick& s) {
    CHECK_EQ(s.brightness(), 102);
    s.run(59000);
    CHECK_EQ(probe::screen(), "home");
    s.run(2000);
    CHECK_EQ(probe::screen(), "bed");
    CHECK_EQ(s.brightness(), 12);
    CHECK_EQ(s.beeps(), 0);                    // no lullaby when it nods off by itself
    s.run(14000);
    CHECK_EQ(probe::screen(), "bed");
    s.run(60000);
    CHECK(false);                              // it napped before getting here
  });
  CHECK_EQ(b.end, End::Nap);
  CHECK(b.buttonArmed);
  CHECK_EQ(b.napUs, 3600000000ull);            // an hour, then it wakes to power off
  // Everything on the board is switched off by hand before the nap.
  CHECK(has(b, "display sleep"));
  CHECK(has(b, "speaker off"));
  CHECK(has(b, "i2c 69 7C=01"));
  CHECK(has(b, "ext output 0"));
  CHECK(has(b, "pm1 gpio2 0"));
  CHECK(has(b, "pm1 led 0"));
}

TEST(a_nap_that_runs_out_powers_off) {
  Device d = hatchedPet();
  Boot nap = d.boot(Wake::Button, [](Stick& s) { s.run(120000); });
  CHECK_EQ(nap.end, End::Nap);
  Boot off = d.boot(Wake::Timer);
  CHECK_EQ(off.end, End::Off);
}

TEST(a_button_wakes_the_pet_from_bed_without_selecting) {
  Device d = hatchedPet();
  d.boot(Wake::Button, [](Stick& s) {
    s.run(62000);
    CHECK_EQ(probe::screen(), "bed");
    s.tap(A);
    CHECK_EQ(probe::screen(), "home");
    CHECK_EQ(s.brightness(), 102);
    s.run(62000);
    CHECK_EQ(probe::screen(), "bed");
    s.tap(B);
    CHECK_EQ(probe::screen(), "home");
    s.tap(A);                                  // the frame did not move: still on feed
    CHECK_EQ(probe::screen(), "food");
  });
}

TEST(bed_icon_plays_a_lullaby_then_naps) {
  Device d = hatchedPet();
  Boot b = d.boot(Wake::Button, [](Stick& s) {
    s.tap(B);
    s.tap(B);
    int before = s.beeps();
    s.tap(A);
    CHECK_EQ(probe::screen(), "bed");
    CHECK_EQ(s.beeps() - before, 4);           // the select beep and three notes
    s.run(20000);
  });
  CHECK_EQ(b.end, End::Nap);
}

TEST(face_down_tucks_the_pet_in) {
  Device d = hatchedPet();
  d.boot(Wake::Button, [](Stick& s) {
    s.run(2000);
    s.faceDown();
    s.run(300);
    CHECK_EQ(probe::screen(), "home");         // a toss does not count
    s.run(1000);
    CHECK_EQ(probe::screen(), "bed");
  });
}

TEST(stay_on_never_goes_to_bed_by_itself) {
  Device d = hatchedPet();
  d.nvs.set("pet/stayon", 1);
  Boot b = d.boot(Wake::Button, [](Stick& s) {
    s.run(10 * 60000);
    CHECK_EQ(probe::screen(), "home");
  });
  CHECK_EQ(b.end, End::Running);
}

TEST(stay_on_can_be_put_to_bed_but_does_not_nap) {
  Device d = hatchedPet();
  d.nvs.set("pet/stayon", 1);
  Boot b = d.boot(Wake::Button, [](Stick& s) {
    s.tap(B);
    s.tap(B);
    s.tap(A);
    s.run(5 * 60000);
    CHECK_EQ(probe::screen(), "bed");
  });
  CHECK_EQ(b.end, End::Running);
}

TEST(egg_left_alone_naps) {
  Device d = unhatchedEgg();
  Boot b = d.boot(Wake::Cold, [](Stick& s) {
    s.run(74000);
    CHECK_EQ(probe::screen(), "egg");
    s.run(60000);
  });
  CHECK_EQ(b.end, End::Nap);
  CHECK_EQ(d.nvs.get("pet/hatched"), 0u);
}

TEST(toy_box_left_alone_goes_back_to_the_pet) {
  Device d = hatchedPet();
  d.boot(Wake::Button, [](Stick& s) {
    s.hold(A, 800);
    CHECK_EQ(probe::screen(), "menu");
    s.run(14000);
    CHECK_EQ(probe::screen(), "menu");
    s.run(2000);
    CHECK_EQ(probe::screen(), "home");
  });
}

TEST(an_activity_left_alone_goes_to_bed) {
  Device d = hatchedPet();
  Boot b = d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 10);
    CHECK_EQ(probe::screen(), "hide");
    s.run(59000);
    CHECK_EQ(probe::screen(), "hide");
    s.run(2500);
    CHECK_EQ(probe::screen(), "bed");
    s.run(30000);
  });
  CHECK_EQ(b.end, End::Nap);
}

TEST(moving_the_stick_keeps_an_activity_awake) {
  Device d = hatchedPet();
  d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 10);
    s.wiggle();
    s.run(3 * 60000);
    CHECK_EQ(probe::screen(), "hide");
  });
}

TEST(a_nap_saves_the_pet_and_the_awake_minutes) {
  Device d = hatchedPet(5, 2, 3);
  Boot b = d.boot(Wake::Button, [](Stick& s) {
    s.tap(A);
    s.tap(A);                                  // feed, then leave it
    s.run(120000);
  });
  CHECK_EQ(b.end, End::Nap);
  CHECK_EQ(d.nvs.get("pet/hunger"), 3u);
  CHECK_EQ(d.nvs.get("pet/awake", 0), 1u);
}

TEST(awake_minutes_are_saved_every_five) {
  Device d = hatchedPet();
  d.nvs.set("pet/stayon", 1);
  d.boot(Wake::Button, [](Stick& s) { s.run(4 * 60000 + 30000); });
  CHECK_EQ(d.nvs.get("pet/awake", 0), 0u);
  d.boot(Wake::Button, [](Stick& s) { s.run(11 * 60000 + 30000); });
  CHECK_EQ(d.nvs.get("pet/awake", 0), 10u);
}
