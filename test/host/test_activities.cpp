// Every toy box activity played through to its reward, and the stickers.
#include "play.h"

using namespace sim;

static bool atHome() { return probe::screen() == "home"; }

// Each of these starts with one heart, so the heart an activity adds shows.
static Device oneHeart() { return hatchedPet(5, 3, 1); }

TEST(walk_finishes_with_enough_moving) {
  Device d = oneHeart();
  d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 0);
    CHECK_EQ(probe::screen(), "walk");
    s.wiggle();
    CHECK(s.runUntil(atHome, 180000));
    CHECK_EQ(probe::happy(), 2);
  });
  CHECK_EQ(d.nvs.get("pet/happy"), 2u);
}

// A still stick gets nowhere on a walk: it waits, then goes back to the pet
// with nothing earned.
TEST(walk_waits_while_the_stick_is_still) {
  Device d = oneHeart();
  d.nvs.set("pet/stayon", 1);
  d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 0);
    s.run(15000);
    CHECK_EQ(probe::screen(), "walk");
    s.run(5 * 60000);
    CHECK_EQ(probe::screen(), "home");
    CHECK_EQ(probe::happy(), 1);
  });
}

TEST(bath_finishes_with_enough_moving) {
  Device d = oneHeart();
  d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 1);
    CHECK_EQ(probe::screen(), "bath");
    s.wiggle();
    CHECK(s.runUntil(atHome, 120000));
    CHECK_EQ(probe::happy(), 2);
  });
}

TEST(dance_finishes_with_enough_moving) {
  Device d = oneHeart();
  d.boot(Wake::Button, [](Stick& s) {
    s.tap(B);
    s.tap(A);
    CHECK_EQ(probe::screen(), "dance");
    s.wiggle();
    CHECK(s.runUntil(atHome, 180000));
    CHECK_EQ(probe::happy(), 2);
  });
}

TEST(rocking_puts_the_pet_to_bed) {
  Device d = oneHeart();
  Boot b = d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 2);
    CHECK_EQ(probe::screen(), "rock");
    s.sway();
    CHECK(s.runUntil([] { return probe::screen() == "bed"; }, 120000));
    CHECK_EQ(probe::happy(), 2);
    s.still();
    s.run(30000);
  });
  CHECK_EQ(b.end, End::Nap);
  CHECK_EQ(d.nvs.get("pet/happy"), 2u);
}

TEST(rocking_waits_while_the_stick_is_still) {
  Device d = oneHeart();
  d.nvs.set("pet/stayon", 1);
  d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 2);
    s.run(15000);
    CHECK_EQ(probe::screen(), "rock");
    s.run(3 * 60000);
    CHECK_EQ(probe::screen(), "home");      // not bed: nothing rocked it to sleep
  });
}

TEST(freeze_is_won_by_holding_still) {
  Device d = oneHeart();
  d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 3);
    CHECK_EQ(probe::screen(), "freeze");
    CHECK(s.runUntil(atHome, 90000));
    CHECK_EQ(probe::happy(), 2);
  });
}

TEST(freeze_is_never_won_while_shaking) {
  Device d = oneHeart();
  d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 3);
    s.shake();
    s.run(3 * 60000);
    CHECK_EQ(probe::screen(), "freeze");       // and never punished either
    CHECK_EQ(probe::happy(), 1);
  });
}

TEST(sticker_book_goes_back_to_its_tile) {
  Device d = oneHeart();
  d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 5);
    CHECK_EQ(probe::screen(), "stickers");
    s.tap(B);
    CHECK_EQ(probe::screen(), "menu");
    CHECK_EQ(probe::menuTile(), 5);
    s.tap(A);
    CHECK_EQ(probe::screen(), "stickers");
    s.run(16000);                              // or left alone, back to the pet
    CHECK_EQ(probe::screen(), "home");
    CHECK_EQ(probe::happy(), 1);
  });
}

TEST(catch_always_ends_in_a_reward) {
  for (uint32_t seed = 1; seed <= 6; seed++) {
    Device d = oneHeart();
    d.seed = seed;
    d.boot(Wake::Button, [](Stick& s) {
      openTile(s, 6);
      CHECK_EQ(probe::screen(), "play");
      CHECK(s.runUntil(atHome, 40000));
      CHECK(probe::happy() == 2 || probe::happy() == 3);   // two hearts for three catches
    });
  }
}

TEST(drawing_counts_once_the_pen_has_moved) {
  Device d = oneHeart();
  d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 7);
    CHECK_EQ(probe::screen(), "draw");
    s.run(800);                                // it reads level first
    for (int i = 0; i < 6; i++) { s.tilt(i % 2 ? -1.0f : 1.0f, 0.3f); s.run(1200); }
    s.hold(A, 800);
    CHECK(s.runUntil(atHome, 6000));
    CHECK_EQ(probe::happy(), 2);
  });
}

TEST(leaving_a_blank_drawing_is_not_playing) {
  Device d = oneHeart();
  d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 7);
    s.run(1500);
    s.hold(A, 800);
    CHECK_EQ(probe::screen(), "home");
    CHECK_EQ(probe::happy(), 1);
  });
}

TEST(marble_is_won_by_rolling_onto_three_apples) {
  Device d = oneHeart();
  d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 8);
    CHECK_EQ(probe::screen(), "marble");
    s.run(800);
    bool home = false;
    for (int i = 0; i < 1200 && !home; i++) {  // steer at the apple, ten times a second
      int dx, dy;
      probe::marbleOffset(&dx, &dy);
      auto lean = [](int d) { return d > 30 ? 1.0f : d < -30 ? -1.0f : d / 30.0f; };
      s.tilt(lean(dx), lean(dy));
      s.run(100);
      home = atHome() || probe::screen() == "reward";
    }
    CHECK(s.runUntil(atHome, 6000));
    CHECK_EQ(probe::happy(), 2);
  });
}

TEST(copycat_is_won_by_repeating_four_patterns) {
  Device d = oneHeart();
  d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 9);
    CHECK_EQ(probe::screen(), "copy");
    for (int round = 0; round < 4; round++) {
      CHECK_EQ(probe::copyRound(), round);
      std::string want = probe::copySequence();
      CHECK_EQ((int)want.size(), round + 1);
      s.run(700 * (round + 3));                // watch it play
      for (char c : want) s.tap(c == 'B' ? B : A);
      s.run(1100);                             // cheer
    }
    CHECK(s.runUntil(atHome, 6000));
    CHECK_EQ(probe::happy(), 2);
  });
}

TEST(copycat_replays_the_pattern_after_a_miss) {
  Device d = oneHeart();
  d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 9);
    std::string want = probe::copySequence();
    s.run(2100);
    s.tap(want[0] == 'B' ? A : B);             // the wrong one
    s.run(1400);
    CHECK_EQ(probe::copyRound(), 0);
    CHECK_EQ(probe::copySequence(), want);     // the same pattern again
    s.run(1500);
    s.tap(want[0] == 'B' ? B : A);
    s.run(1100);
    CHECK_EQ(probe::copyRound(), 1);
  });
}

TEST(hide_and_seek_is_won_by_three_finds) {
  Device d = oneHeart();
  d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 10);
    CHECK_EQ(probe::screen(), "hide");
    for (int find = 0; find < 3; find++) {
      s.run(1300);                             // the eyes peek, then duck
      int moves = (probe::hideSpot() - probe::hideFrame() + 3) % 3;
      for (int i = 0; i < moves; i++) s.tap(B);
      s.tap(A);
      s.run(2000);                             // found
    }
    CHECK(s.runUntil(atHome, 6000));
    CHECK_EQ(probe::happy(), 2);
  });
}

TEST(hide_and_seek_wrong_bush_just_peeks_again) {
  Device d = oneHeart();
  d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 10);
    s.run(1300);
    int spot = probe::hideSpot();
    int wrong = (spot + 1 - probe::hideFrame() + 3) % 3;
    for (int i = 0; i < wrong; i++) s.tap(B);
    s.tap(A);
    s.run(1500);
    CHECK_EQ(probe::screen(), "hide");
    CHECK_EQ(probe::hideSpot(), spot);         // it has not moved
    CHECK_EQ(probe::happy(), 1);
  });
}

TEST(happiness_stops_at_four) {
  Device d = hatchedPet(5, 3, 4);
  d.boot(Wake::Button, [](Stick& s) {
    s.tap(B);
    s.tap(A);
    s.wiggle();
    CHECK(s.runUntil(atHome, 180000));
    CHECK_EQ(probe::happy(), 4);
  });
}

// --- Stickers -------------------------------------------------------------------
// A dance is the quickest activity to finish.
static void danceForSticker(Device& d) {
  d.boot(Wake::Button, [](Stick& s) {
    s.tap(B);
    s.tap(A);
    s.wiggle();
    CHECK(s.runUntil(atHome, 180000));
  });
}

TEST(stickers_come_sometimes_and_never_twice) {
  int earned = 0, plays = 0;
  Device d = oneHeart();
  for (uint32_t seed = 1; seed <= 30; seed++) {
    d.seed = seed;
    uint32_t before = d.nvs.get("pet/stickers");
    danceForSticker(d);
    uint32_t after = d.nvs.get("pet/stickers");
    CHECK_EQ(after & before, before);                        // none taken away
    CHECK(__builtin_popcount(after ^ before) <= 1);          // at most one new
    CHECK_EQ(after & ~0x0FFFu, 0u);                          // only the twelve that exist
    earned += after != before;
    plays++;
  }
  CHECK(earned >= 3);                                        // about one play in three
  CHECK(earned < plays);
}

TEST(a_full_sticker_book_gets_no_more) {
  Device d = oneHeart();
  d.nvs.set("pet/stickers", 0x0FFF, 2);
  for (uint32_t seed = 1; seed <= 6; seed++) {
    d.seed = seed;
    d.boot(Wake::Button, [](Stick& s) {
      s.tap(B);
      s.tap(A);
      s.wiggle();
      CHECK(s.runUntil([] {
        CHECK(probe::screen() != "reward");
        return atHome();
      }, 180000));
    });
    CHECK_EQ(d.nvs.get("pet/stickers"), 0x0FFFu);
  }
}

TEST(the_last_sticker_can_be_earned) {
  Device d = oneHeart();
  d.nvs.set("pet/stickers", 0x0FFF & ~(1u << 7), 2);         // all but one
  for (uint32_t seed = 1; seed <= 40 && d.nvs.get("pet/stickers") != 0x0FFF; seed++) {
    d.seed = seed;
    danceForSticker(d);
  }
  CHECK_EQ(d.nvs.get("pet/stickers"), 0x0FFFu);
}
