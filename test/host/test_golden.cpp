// Scripted play-throughs compared with a stored trace (test/golden/*.txt):
// every beep, save, brightness change and power event with its time, and a
// checksum of everything drawn in each second.
//
// These say nothing about what is right. They say the firmware still does
// exactly what it did when the trace was stored, which is the check a
// tidy-up of the code needs. After a change that is meant to alter what a
// screen shows or does, run `make test-host-update` and read the diff of
// test/golden/ before keeping it.
#include "play.h"

using namespace sim;

static bool atHome() { return probe::screen() == "home"; }

TEST(golden_first_boot) {
  Device d;
  Boot b = d.boot(Wake::Cold, [](Stick& s) {
    finishLesson(s);
    for (int i = 0; i < 7; i++) { s.tap(A); s.run(400); }
    s.run(8000);
  });
  golden("first_boot", b.trace);
}

TEST(golden_home) {
  Device d = hatchedPet(5, 2, 2);
  Boot day = d.boot(Wake::Cold, [](Stick& s) {
    s.run(3000);
    for (int i = 0; i < 3; i++) { s.tap(B); s.run(300); }
    s.shake();
    s.run(1500);
    s.still();
    s.run(3000);
    for (int food = 0; food < 4; food++) {
      s.tap(A);
      s.run(500);
      s.tap(B);                                // one along from the last food each time
      s.tap(A);
      s.runUntil(atHome, 6000);
      s.run(500);
    }
    s.hold(B, 1600);                           // mute, and the glyph that shows it
    s.run(2000);
    s.hold(B, 1600);
    s.run(120000);                             // left alone: bed, then a nap
  });
  Boot night = d.boot(Wake::Timer);
  golden("home", day.trace + "--- nap timer ---\n" + night.trace);
}

TEST(golden_moods) {
  std::string all;
  const struct { const char* name; int hunger, happy, mv; } moods[] = {
    { "hungry", 0, 3, 3900 }, { "bored", 3, 0, 3900 }, { "content", 4, 4, 3900 },
    { "low battery", 3, 3, 3250 }, { "full battery", 3, 3, 4200 } };
  for (const auto& m : moods) {
    Device d = hatchedPet(1, m.hunger, m.happy);
    d.batteryMv = m.mv;
    Boot b = d.boot(Wake::Button, [](Stick& s) { s.run(20000); });
    all += std::string("--- ") + m.name + " ---\n" + b.trace;
  }
  golden("moods", all);
}

TEST(golden_toy_box) {
  Device d = hatchedPet();
  Boot b = d.boot(Wake::Button, [](Stick& s) {
    s.hold(A, 800);
    for (int i = 0; i < 12; i++) { s.tap(B); s.run(300); }
    for (int i = 0; i < 11; i++) s.tap(B);
    s.tap(A);                                  // the egg tile wobbles
    s.run(800);
    s.hold(A, 2000);                           // the ring part-fills, then lets go
    s.run(500);
    s.hold(A, 3300);                           // all the way: a new egg
    s.run(4000);
  });
  golden("toy_box", b.trace);
}

TEST(golden_walk) {
  std::string all;
  // The route is picked at random, never the same twice running.
  for (uint32_t seed = 1; seed <= 4; seed++) {
    Device d = hatchedPet(5, 3, 1);
    d.seed = seed;
    Boot b = d.boot(Wake::Button, [](Stick& s) {
      openTile(s, 0);
      s.run(2000);
      s.wiggle();
      s.run(10000);
      s.still();
      s.run(2000);
      s.wiggle();
      s.runUntil(atHome, 180000);
      s.run(4000);
    });
    all += "--- seed " + std::to_string(seed) + " ---\n" + b.trace;
  }
  golden("walk", all);
}

TEST(golden_bath) {
  Device d = hatchedPet(5, 3, 1);
  Boot b = d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 1);
    s.run(2000);
    s.wiggle();
    s.runUntil(atHome, 120000);
    s.run(4000);
  });
  golden("bath", b.trace);
}

TEST(golden_dance) {
  Device d = hatchedPet(5, 3, 1);
  Boot b = d.boot(Wake::Button, [](Stick& s) {
    s.tap(B);
    s.tap(A);
    s.run(2000);
    s.wiggle();
    s.run(8000);
    s.still();
    s.run(2000);
    s.wiggle();
    s.runUntil(atHome, 180000);
    s.run(4000);
  });
  golden("dance", b.trace);
}

TEST(golden_rock) {
  Device d = hatchedPet(5, 3, 1);
  Boot b = d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 2);
    s.run(2000);
    s.sway();
    s.runUntil([] { return probe::screen() == "bed"; }, 120000);
    s.still();
    s.run(30000);
  });
  golden("rock", b.trace);
}

TEST(golden_freeze) {
  Device d = hatchedPet(5, 3, 1);
  Boot b = d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 3);
    s.shake();                                 // caught moving a few times first
    s.run(15000);
    s.still();
    s.runUntil(atHome, 90000);
    s.run(4000);
  });
  golden("freeze", b.trace);
}

TEST(golden_dress_up) {
  Device d = hatchedPet(1);
  Boot b = d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 4);
    for (int i = 0; i < 6; i++) { s.tap(B); s.run(600); }
    for (int i = 0; i < 5; i++) { s.tap(A); s.run(600); }
    s.tap(A);
    s.tap(B);
    s.hold(A, 800);
    s.run(3000);                               // dressed, on the pet screen
    openTile(s, 10);                           // and its eyes in hide and seek take the colour
    s.run(2000);
  });
  golden("dress_up", b.trace);
}

TEST(golden_sticker_book) {
  std::string all;
  for (uint32_t have : { 0x0000u, 0x0A53u, 0x0FFFu }) {
    Device d = hatchedPet();
    d.nvs.set("pet/stickers", have, 2);
    Boot b = d.boot(Wake::Button, [](Stick& s) {
      openTile(s, 5);
      s.run(2000);
    });
    all += "--- " + std::to_string(have) + " ---\n" + b.trace;
  }
  golden("sticker_book", all);
}

TEST(golden_catch) {
  Device d = hatchedPet(5, 3, 1);
  Boot b = d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 6);
    s.run(1500);
    for (int i = 0; i < 8 && !atHome(); i++) { s.tilt(i % 2 ? 0.7f : -0.7f, 0); s.run(1500); }
    s.runUntil(atHome, 40000);
    s.run(4000);
  });
  golden("catch", b.trace);
}

TEST(golden_draw) {
  Device d = hatchedPet(5, 3, 1);
  Boot b = d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 7);
    s.run(800);
    const float path[][2] = { { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 }, { 0.5f, 0.5f }, { -0.5f, 0.2f } };
    for (auto& p : path) { s.tilt(p[0], p[1]); s.run(1000); }
    s.tap(A);                                  // a fresh page
    for (auto& p : path) { s.tilt(p[0], p[1]); s.run(700); }
    s.heldToPlay();
    s.shake();                                 // wipes it
    s.run(1000);
    s.still();
    s.tilt(1, 1);
    s.run(1500);
    s.hold(A, 800);
    s.runUntil(atHome, 6000);
    s.run(4000);
  });
  golden("draw", b.trace);
}

TEST(golden_marble) {
  Device d = hatchedPet(5, 3, 1);
  Boot b = d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 8);
    s.run(800);
    for (int i = 0; i < 1200 && probe::screen() == "marble"; i++) {
      int dx, dy;
      probe::marbleOffset(&dx, &dy);
      auto lean = [](int v) { return v > 30 ? 1.0f : v < -30 ? -1.0f : v / 30.0f; };
      s.tilt(lean(dx), lean(dy));
      s.run(100);
    }
    s.runUntil(atHome, 6000);
    s.run(4000);
  });
  golden("marble", b.trace);
}

TEST(golden_copycat) {
  Device d = hatchedPet(5, 3, 1);
  Boot b = d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 9);
    std::string first = probe::copySequence();
    s.run(2100);
    s.tap(first[0] == 'B' ? A : B);            // one miss, and the replay
    s.run(2900);
    for (int round = 0; round < 4; round++) {
      std::string want = probe::copySequence();
      if (round) s.run(700 * (round + 3));
      for (char c : want) s.tap(c == 'B' ? B : A);
      s.run(1100);
    }
    s.runUntil(atHome, 6000);
    s.run(4000);
  });
  golden("copycat", b.trace);
}

TEST(golden_hide_and_seek) {
  Device d = hatchedPet(5, 3, 1);
  Boot b = d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 10);
    s.run(1300);
    int wrong = (probe::hideSpot() + 1 - probe::hideFrame() + 3) % 3;
    for (int i = 0; i < wrong; i++) s.tap(B);
    s.tap(A);                                  // one wrong bush
    for (int find = 0; find < 3; find++) {
      s.run(1300);
      int moves = (probe::hideSpot() - probe::hideFrame() + 3) % 3;
      for (int i = 0; i < moves; i++) s.tap(B);
      s.tap(A);
      s.run(2000);
    }
    s.runUntil(atHome, 6000);
    s.run(4000);
  });
  golden("hide_and_seek", b.trace);
}

// The first seed on which a dance wins a sticker.
static Boot danceUntilSticker(Device start, uint32_t* seedOut) {
  for (uint32_t seed = 1; seed <= 60; seed++) {
    Device d = start;
    d.seed = seed;
    Boot b = d.boot(Wake::Button, [](Stick& s) {
      s.tap(B);
      s.tap(A);
      s.wiggle();
      s.runUntil(atHome, 180000);
      s.still();
      s.run(10000);
    });
    if (d.nvs.get("pet/stickers") != start.nvs.get("pet/stickers")) {
      *seedOut = seed;
      return b;
    }
  }
  throw Failure{ "no sticker in sixty dances" };
}

TEST(golden_sticker_reward) {
  uint32_t seed = 0;
  Boot first = danceUntilSticker(hatchedPet(5, 3, 1), &seed);
  std::string all = "--- first sticker, seed " + std::to_string(seed) + " ---\n" + first.trace;
  Device nearly = hatchedPet(5, 3, 1);
  nearly.nvs.set("pet/stickers", 0x0FFF & ~(1u << 4), 2);
  Boot last = danceUntilSticker(nearly, &seed);
  all += "--- last sticker, seed " + std::to_string(seed) + " ---\n" + last.trace;
  golden("sticker_reward", all);
}

TEST(golden_parent_screen) {
  Device d = hatchedPet(5, 3, 3);
  Boot b = d.boot(Wake::Button, [](Stick& s) {
    s.holdBoth(1600);
    s.run(1000);
    for (int item = 0; item < 5; item++) {     // bright, sound, stay on, egg tile, pet
      s.tap(A);
      s.run(300);
      s.tap(A);
      s.run(300);
      s.tap(B);
    }
    s.tap(A);                                  // tilt cal
    s.run(500);
    const float pose[5][3] = { { -0.55f, 0.00f, 0.84f }, { -0.54f, 0.63f, 0.55f }, { -0.56f, -0.57f, 0.61f },
                               { 0.05f, 0.01f, 1.00f }, { -0.98f, 0.01f, 0.14f } };
    for (auto& p : pose) { s.pose(p[0], p[1], p[2]); s.tap(A); s.run(700); }
    s.run(1000);                               // the results
    s.tap(B);                                  // again, and this time not enough tilt
    for (int i = 0; i < 5; i++) { s.tap(A); s.run(700); }
    s.run(1000);
    s.tap(A);                                  // back to the list
    s.heldToPlay();
    s.tap(B);
    s.tap(B);                                  // past lesson to reset
    s.tap(A);                                  // "really?"
    s.run(1000);
    s.tap(A);
    s.run(3000);                               // wiped: the lesson starts
  });
  golden("parent_screen", b.trace);
}

// Every animal through every animation for well over a full cycle, at both
// sizes. Run under the address sanitizer this is also the check that no
// animation's sequence table points at a pose that is not there.
TEST(golden_animals) {
  const char* const name[] = { "blob", "cat", "chick", "dog", "ghost", "octopus", "penguin" };
  for (int sp = 0; sp < 7; sp++) {
    Device d = hatchedPet(sp);
    const char* want = name[sp];
    Boot b = d.boot(Wake::Button, [sp, want](Stick&) {
      CHECK_EQ(probe::speciesCount(), 7);
      CHECK_EQ(probe::species(), want);
      for (int state = 0; state < 7; state++) probe::animate(sp, state, 400);
      for (int state = 0; state < 7; state++) probe::animate(sp, state, 100, true);
    });
    golden(std::string("animal_") + want, b.trace);
  }
}

TEST(golden_animals_dressed) {
  std::string all;
  for (int sp = 0; sp < 7; sp++) {
    Device d = hatchedPet(sp);
    d.nvs.set("pet/color", (uint32_t)(1 + sp % 5));
    d.nvs.set("pet/hat", (uint32_t)(1 + sp % 4));
    Boot b = d.boot(Wake::Button, [sp](Stick&) {
      for (int state = 0; state < 7; state++) probe::animate(sp, state, 100);
      for (int state = 0; state < 7; state++) probe::animate(sp, state, 40, true);
    });
    all += "--- " + std::to_string(sp) + " ---\n" + b.trace;
  }
  golden("animals_dressed", all);
}
