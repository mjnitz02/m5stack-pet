// The parent screen: bright, sound, stay on, egg tile, pet, tilt cal,
// lesson, reset, back, in that order from the top.
#include "play.h"

using namespace sim;

enum { BRIGHT, SOUND, STAYON, EGGTILE, PET, CAL, LESSON, RESET, BACK };

static void openParent(Stick& s, int item) {
  s.holdBoth(1600);
  CHECK_EQ(probe::screen(), "parent");
  for (int i = 0; i < item; i++) s.tap(B);
}

TEST(parent_brightness_steps_and_wraps) {
  Device d = hatchedPet();
  d.boot(Wake::Button, [](Stick& s) {
    openParent(s, BRIGHT);
    const int want[] = { 153, 204, 255, 51, 102 };   // from the default of 1/4
    for (int v : want) {
      s.tap(A);
      CHECK_EQ(s.brightness(), v);
    }
    s.tap(A);
  });
  CHECK_EQ(d.nvs.get("pet/bright"), 2u);
  d.boot(Wake::Button, [](Stick& s) { CHECK_EQ(s.brightness(), 153); });
}

TEST(parent_switches_are_saved) {
  Device d = hatchedPet();
  d.boot(Wake::Button, [](Stick& s) {
    openParent(s, SOUND);
    s.tap(A);
    s.tap(B);
    s.tap(A);                                  // stay on
    s.tap(B);
    s.tap(A);                                  // egg tile
  });
  CHECK_EQ(d.nvs.get("pet/sound", 1), 0u);
  CHECK_EQ(d.nvs.get("pet/stayon", 0), 1u);
  CHECK_EQ(d.nvs.get("pet/eggtile", 1), 0u);
}

TEST(parent_back_returns_to_where_the_pet_is) {
  Device d = hatchedPet();
  d.boot(Wake::Button, [](Stick& s) {
    openParent(s, BACK);
    s.tap(A);
    CHECK_EQ(probe::screen(), "home");
  });
  Device e = unhatchedEgg();
  e.boot(Wake::Cold, [](Stick& s) {
    openParent(s, BACK);
    s.tap(A);
    CHECK_EQ(probe::screen(), "egg");
  });
}

TEST(parent_can_name_the_animal) {
  Device d = hatchedPet(5);
  d.boot(Wake::Button, [](Stick& s) {
    openParent(s, PET);
    s.tap(A);
    CHECK_EQ(probe::species(), "blob");        // the first in the list, there and then
    s.tap(A);
    CHECK_EQ(probe::species(), "cat");
  });
  CHECK_EQ(d.nvs.get("pet/pick"), 2u);
  CHECK_EQ(d.nvs.get("pet/species"), 1u);
}

TEST(parent_back_on_random_gives_the_pet_back) {
  Device d = hatchedPet(5);
  d.boot(Wake::Button, [](Stick& s) {
    openParent(s, PET);
    for (int i = 0; i < probe::speciesCount(); i++) s.tap(A);
    CHECK_EQ(probe::species(), "penguin");
    s.tap(A);                                  // round to random again
    CHECK_EQ(probe::species(), "octopus");
  });
  CHECK_EQ(d.nvs.get("pet/pick"), 0u);
  CHECK_EQ(d.nvs.get("pet/species"), 5u);
}

TEST(parent_reset_needs_two_taps) {
  Device d = hatchedPet();
  d.boot(Wake::Button, [](Stick& s) {
    openParent(s, RESET);
    s.tap(A);
    CHECK(probe::hatched());
    s.run(3500);                               // "really?" times out
    s.tap(A);
    CHECK(probe::hatched());
    s.tap(B);                                  // moving off it lets go too
    for (int i = 0; i < 8; i++) s.tap(B);
    s.tap(A);
    CHECK(probe::hatched());
    CHECK_EQ(probe::screen(), "parent");
  });
  CHECK_EQ(d.nvs.get("pet/hatched"), 1u);
}

TEST(parent_reset_wipes_everything_but_the_tilt_calibration) {
  Device d = hatchedPet(5, 4, 4);
  d.nvs.set("pet/stickers", 0x0FFF, 2);
  d.nvs.set("pet/sound", 0);
  d.nvs.set("pet/bright", 4);
  d.nvs.set("pet/stayon", 1);
  d.nvs.set("pet/awake", 500, 4);
  d.nvs.set("pet/pick", 3);
  d.boot(Wake::Button, [](Stick& s) {
    // Calibrate first, so there is something to keep.
    openParent(s, CAL);
    s.tap(A);
    const float pose[5][3] = { { -0.55f, 0.00f, 0.84f }, { -0.54f, 0.63f, 0.55f }, { -0.56f, -0.57f, 0.61f },
                               { 0.05f, 0.01f, 1.00f }, { -0.98f, 0.01f, 0.14f } };
    for (auto& p : pose) { s.pose(p[0], p[1], p[2]); s.tap(A); s.run(700); }
    s.tap(A);
    CHECK_EQ(probe::screen(), "parent");
    s.tap(B);
    s.tap(B);                                  // from tilt cal down to reset
    s.tap(A);
    s.tap(A);
    CHECK_EQ(probe::screen(), "learn");        // as if freshly flashed
    CHECK(!probe::hatched());
    CHECK(probe::sound());
    CHECK_EQ(s.brightness(), 102);
  });
  CHECK_EQ(d.nvs.get("pet/hatched"), 0u);
  CHECK_EQ(d.nvs.get("pet/learned"), 0u);
  CHECK_EQ(d.nvs.get("pet/stickers"), 0u);
  CHECK_EQ(d.nvs.get("pet/stayon"), 0u);
  CHECK_EQ(d.nvs.get("pet/awake", 9), 0u);
  CHECK_EQ(d.nvs.get("pet/pick"), 0u);
  CHECK_EQ(d.nvs.get("pet/roster"), 1u);
  CHECK(d.nvs.has("pet/tiltcal"));
  CHECK_EQ((int)d.nvs.kv["pet/tiltcal"][0], 1);
}

TEST(parent_lesson_plays_and_comes_back_to_the_pet) {
  Device d = hatchedPet(5, 3, 3);
  d.boot(Wake::Button, [](Stick& s) {
    openParent(s, LESSON);
    s.tap(A);
    CHECK_EQ(probe::screen(), "learn");
    finishLesson(s);
    CHECK_EQ(probe::screen(), "home");
    CHECK(probe::hatched());
    CHECK_EQ(probe::hunger(), 3);
  });
}

TEST(tilt_calibration_is_saved_from_five_poses) {
  Device d = hatchedPet();
  d.boot(Wake::Button, [](Stick& s) {
    openParent(s, CAL);
    s.tap(A);
    CHECK_EQ(probe::screen(), "cal");
    const float pose[5][3] = { { -0.55f, 0.00f, 0.84f }, { -0.54f, 0.63f, 0.55f }, { -0.56f, -0.57f, 0.61f },
                               { 0.05f, 0.01f, 1.00f }, { -0.98f, 0.01f, 0.14f } };
    for (auto& p : pose) { s.pose(p[0], p[1], p[2]); s.tap(A); s.run(700); }
    CHECK_EQ(probe::screen(), "cal");          // the results stay up
    s.tap(A);
    CHECK_EQ(probe::screen(), "parent");
  });
  CHECK(d.nvs.has("pet/tiltcal"));
  CHECK_EQ((int)d.nvs.kv["pet/tiltcal"][0], 1);
}

TEST(tilt_calibration_without_tilting_is_refused) {
  Device d = hatchedPet();
  d.boot(Wake::Button, [](Stick& s) {
    openParent(s, CAL);
    s.tap(A);
    for (int i = 0; i < 5; i++) { s.tap(A); s.run(700); }
    s.tap(A);
    CHECK_EQ(probe::screen(), "parent");
  });
  CHECK(!d.nvs.has("pet/tiltcal"));
}

TEST(tilt_calibration_side_button_backs_out_and_clears) {
  Device d = hatchedPet();
  auto calibrate = [](Stick& s) {
    openParent(s, CAL);
    s.tap(A);
    const float pose[5][3] = { { -0.55f, 0.00f, 0.84f }, { -0.54f, 0.63f, 0.55f }, { -0.56f, -0.57f, 0.61f },
                               { 0.05f, 0.01f, 1.00f }, { -0.98f, 0.01f, 0.14f } };
    for (auto& p : pose) { s.pose(p[0], p[1], p[2]); s.tap(A); s.run(700); }
  };
  d.boot(Wake::Button, calibrate);
  CHECK_EQ((int)d.nvs.kv["pet/tiltcal"][0], 1);
  d.boot(Wake::Button, [](Stick& s) {
    openParent(s, CAL);
    s.tap(A);
    s.tap(A);
    s.run(700);                                // one pose in
    s.tap(B);                                  // cancel: the saved one stays
    CHECK_EQ(probe::screen(), "parent");
  });
  CHECK_EQ((int)d.nvs.kv["pet/tiltcal"][0], 1);
  d.boot(Wake::Button, [](Stick& s) {
    openParent(s, CAL);
    s.tap(A);
    s.tap(B);                                  // on the first pose: clear it
    CHECK_EQ(probe::screen(), "parent");
  });
  CHECK_EQ((int)d.nvs.kv["pet/tiltcal"][0], 0);
}
