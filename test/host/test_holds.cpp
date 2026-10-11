// The long presses: toy box, back to the pet, mute, the parent screen and
// the egg tile's reset.
#include "play.h"

using namespace sim;

TEST(hold_blue_opens_the_toy_box_and_shuts_it) {
  Device d = hatchedPet();
  d.boot(Wake::Button, [](Stick& s) {
    s.hold(A, 800);
    CHECK_EQ(probe::screen(), "menu");
    CHECK_EQ(probe::menuTile(), 0);
    s.hold(A, 800);
    CHECK_EQ(probe::screen(), "home");
  });
}

TEST(a_short_press_is_not_a_hold) {
  Device d = hatchedPet();
  d.boot(Wake::Button, [](Stick& s) {
    s.hold(A, 500);
    CHECK_EQ(probe::screen(), "food");
  });
}

TEST(hold_blue_leaves_an_activity) {
  Device d = hatchedPet();
  d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 4);
    CHECK_EQ(probe::screen(), "dress");
    s.hold(A, 800);
    CHECK_EQ(probe::screen(), "home");
  });
  CHECK_EQ(d.nvs.get("pet/hat", 0), 0u);       // and the hold did not change the hat
}

TEST(toy_box_walks_both_pages_and_wraps) {
  Device d = hatchedPet();
  d.boot(Wake::Button, [](Stick& s) {
    s.hold(A, 800);
    for (int i = 1; i <= 11; i++) {
      s.tap(B);
      CHECK_EQ(probe::menuTile(), i);
    }
    s.tap(B);
    CHECK_EQ(probe::menuTile(), 0);
  });
}

TEST(toy_box_without_the_egg_tile_has_eleven) {
  Device d = hatchedPet();
  d.nvs.set("pet/eggtile", 0);
  d.boot(Wake::Button, [](Stick& s) {
    s.hold(A, 800);
    for (int i = 0; i < 10; i++) s.tap(B);
    CHECK_EQ(probe::menuTile(), 10);
    s.tap(B);
    CHECK_EQ(probe::menuTile(), 0);
  });
}

TEST(hold_side_mutes_and_unmutes) {
  Device d = hatchedPet();
  d.boot(Wake::Button, [](Stick& s) {
    CHECK(probe::sound());
    int before = s.beeps();
    s.hold(B, 1600);
    CHECK(!probe::sound());
    CHECK_EQ(s.beeps(), before);               // silent going quiet
    s.tap(B);
    CHECK_EQ(s.beeps(), before);               // and silent after
    s.tap(A);
    CHECK_EQ(probe::screen(), "dance");        // the hold itself did not move the frame
  });
  CHECK_EQ(d.nvs.get("pet/sound", 1), 0u);
  d.boot(Wake::Button, [](Stick& s) {
    CHECK(!probe::sound());
    s.hold(B, 1600);
    CHECK(probe::sound());
    CHECK_EQ(s.beeps(), 1);                    // a chirp when it comes back
  });
  CHECK_EQ(d.nvs.get("pet/sound", 0), 1u);
}

TEST(hold_both_opens_the_parent_screen) {
  Device d = hatchedPet();
  d.boot(Wake::Button, [](Stick& s) {
    // Let go too soon and it is two ordinary presses: side moves the frame
    // to the dance icon and blue picks it.
    s.holdBoth(1400);
    CHECK_EQ(probe::screen(), "dance");
  });
  d.boot(Wake::Button, [](Stick& s) {
    s.holdBoth(1600);
    CHECK_EQ(probe::screen(), "parent");
    s.run(500);
    CHECK_EQ(probe::screen(), "parent");       // letting go changed nothing
    CHECK(probe::sound());
  });
  CHECK(!d.nvs.has("pet/bright"));             // nothing was saved either
}

TEST(hold_both_opens_the_parent_screen_from_an_activity) {
  Device d = hatchedPet();
  d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 0);
    CHECK_EQ(probe::screen(), "walk");
    s.holdBoth(1600);
    CHECK_EQ(probe::screen(), "parent");
  });
}

TEST(hold_both_in_the_lesson_skips_it) {
  Device d;
  d.boot(Wake::Cold, [](Stick& s) {
    s.run(1000);
    s.holdBoth(1600);
    CHECK_EQ(probe::screen(), "parent");
    CHECK(probe::learned());
    for (int i = 0; i < 8; i++) s.tap(B);      // down to "back"
    s.tap(A);
    CHECK_EQ(probe::screen(), "egg");
  });
  CHECK_EQ(d.nvs.get("pet/learned"), 1u);
}

TEST(a_tap_on_the_egg_tile_does_nothing) {
  Device d = hatchedPet();
  d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 11);
    CHECK_EQ(probe::screen(), "menu");
    CHECK(probe::hatched());
  });
}

TEST(letting_go_early_keeps_the_pet) {
  Device d = hatchedPet();
  d.boot(Wake::Button, [](Stick& s) {
    s.hold(A, 800);
    for (int i = 0; i < 11; i++) s.tap(B);
    s.hold(A, 2500);
    CHECK_EQ(probe::screen(), "menu");
    CHECK(probe::hatched());
  });
  CHECK_EQ(d.nvs.get("pet/hatched"), 1u);
}

TEST(holding_the_egg_tile_starts_a_new_egg) {
  Device d = hatchedPet(5, 4, 4);
  d.nvs.set("pet/stickers", 0x0155, 2);
  d.nvs.set("pet/color", 2);
  d.nvs.set("pet/hat", 3);
  d.nvs.set("pet/sound", 0);
  d.nvs.set("pet/bright", 3);
  d.boot(Wake::Button, [](Stick& s) {
    s.hold(A, 800);
    for (int i = 0; i < 11; i++) s.tap(B);
    s.hold(A, 3300);
    CHECK_EQ(probe::screen(), "egg");
    CHECK(!probe::hatched());
    CHECK_EQ(probe::stickers(), 0);
  });
  CHECK_EQ(d.nvs.get("pet/hatched"), 0u);
  CHECK_EQ(d.nvs.get("pet/stickers"), 0u);
  CHECK_EQ(d.nvs.get("pet/color"), 0u);
  CHECK_EQ(d.nvs.get("pet/hat"), 0u);
  // Settings and the lesson are the grown-up's, and stay.
  CHECK_EQ(d.nvs.get("pet/sound"), 0u);
  CHECK_EQ(d.nvs.get("pet/bright"), 3u);
  CHECK_EQ(d.nvs.get("pet/learned"), 1u);
}
