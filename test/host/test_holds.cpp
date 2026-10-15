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
  d.nvs.set("pet/bright", 1);
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
  CHECK_EQ(d.nvs.get("pet/bright"), 1u);
  CHECK_EQ(d.nvs.get("pet/learned"), 1u);
}

// --- A button down while the screen changes ---------------------------------------
static bool atHome() { return probe::screen() == "home"; }

// Feed once and say how long the eating took, so the next press can be timed
// to straddle its end.
static uint32_t feedOnce(Stick& s) {
  s.tap(A);
  s.tap(A);
  CHECK_EQ(probe::screen(), "feed");
  uint32_t from = s.now();
  CHECK(s.runUntil(atHome, 8000));
  return s.now() - from;
}

TEST(blue_held_through_the_end_of_feeding_does_not_open_the_toy_box) {
  Device d = hatchedPet(5, 1, 3);
  d.boot(Wake::Button, [](Stick& s) {
    uint32_t eating = feedOnce(s);
    s.tap(A);
    s.tap(A);
    s.run(eating - 300);
    CHECK_EQ(probe::screen(), "feed");
    s.press(A);
    CHECK(s.runUntil(atHome, 1000));
    s.run(1500);
    CHECK_EQ(probe::screen(), "home");         // the hold belonged to the screen before
    s.release(A);
    s.run(100);
    CHECK_EQ(probe::screen(), "home");         // and so did letting go
    s.hold(A, 800);
    CHECK_EQ(probe::screen(), "menu");         // a fresh hold still works
  });
}

TEST(side_let_go_after_feeding_ends_does_not_move_the_frame) {
  Device d = hatchedPet(5, 1, 3);
  d.boot(Wake::Button, [](Stick& s) {
    uint32_t eating = feedOnce(s);
    s.tap(A);
    s.tap(A);
    s.run(eating - 300);
    s.press(B);
    CHECK(s.runUntil(atHome, 1000));
    s.run(100);
    s.release(B);
    s.run(100);
    s.tap(A);
    CHECK_EQ(probe::screen(), "food");         // the frame is still on feed
  });
}

TEST(egg_tile_ring_sounds_the_same_every_time) {
  Device d = hatchedPet();
  d.boot(Wake::Button, [](Stick& s) {
    s.hold(A, 800);
    for (int i = 0; i < 11; i++) s.tap(B);
    int before = s.beeps();
    s.hold(A, 1000);
    int first = s.beeps() - before;
    s.run(500);
    before = s.beeps();
    s.hold(A, 1000);
    CHECK_EQ(s.beeps() - before, first);
  });
}

// --- The mute picture ---------------------------------------------------------------
// It is drawn in the top corner for a moment. Hide and seek only repaints
// the top of its screen now and then, so it has to be wiped away.
TEST(mute_picture_is_wiped_off_in_hide_and_seek) {
  Device d = hatchedPet();
  d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 10);
    s.run(1300);
    s.watchDrawing();
    s.hold(B, 1600);
    CHECK(!probe::sound());
    s.run(2500);
    CHECK_EQ(probe::screen(), "hide");
    int shown = -1, wiped = -1;
    const auto& calls = s.drawn();
    for (int i = 0; i < (int)calls.size(); i++) {
      int x, y, w, h;
      if (calls[i] == "spr fillTri 8 7 8 11 13 4 FA20") shown = i;   // the speaker's cone
      if (calls[i].rfind("spr fill 0000", 0) == 0) wiped = i;
      if (sscanf(calls[i].c_str(), "spr fillRect %d %d %d %d 0000", &x, &y, &w, &h) == 4 &&
          x <= 2 && y <= 2 && x + w >= 24 && y + h >= 16 && !(x == 2 && y == 2 && w == 22 && h == 14 &&
          i + 1 < (int)calls.size() && calls[i + 1].rfind("spr fillRect 4 7 4 5", 0) == 0)) wiped = i;
    }
    CHECK(shown >= 0);
    CHECK(wiped > shown);
  });
}
