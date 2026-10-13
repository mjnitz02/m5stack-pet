// What is remembered across power-offs, and what an old or damaged save
// turns into.
#include "play.h"

using namespace sim;

TEST(changes_survive_a_power_cycle) {
  Device d = hatchedPet(1, 2, 2);
  d.boot(Wake::Button, [](Stick& s) {
    s.tap(A);
    s.tap(A);                                  // feed
    CHECK(s.runUntil([] { return probe::screen() == "home"; }, 6000));
    s.hold(B, 1600);                           // mute
    openTile(s, 4);
    s.tap(B);                                  // a colour
    s.tap(A);
    s.tap(A);                                  // the second hat
  });
  d.boot(Wake::Button, [](Stick& s) {
    CHECK_EQ(probe::species(), "cat");
    CHECK_EQ(probe::hunger(), 3);
    CHECK(!probe::sound());
  });
  CHECK_EQ(d.nvs.get("pet/color"), 1u);
  CHECK_EQ(d.nvs.get("pet/hat"), 2u);
}

TEST(dress_up_wraps_round_to_plain) {
  Device d = hatchedPet();
  d.boot(Wake::Button, [](Stick& s) {
    openTile(s, 4);
    for (int i = 0; i < 6; i++) s.tap(B);      // six colours
    for (int i = 0; i < 5; i++) s.tap(A);      // five hats, counting none
  });
  CHECK_EQ(d.nvs.get("pet/color", 9), 0u);
  CHECK_EQ(d.nvs.get("pet/hat", 9), 0u);
}

TEST(out_of_range_needs_and_brightness_are_pulled_in) {
  Device d = hatchedPet(5, 200, 9);
  d.nvs.set("pet/bright", 77);
  d.boot(Wake::Button, [](Stick& s) {
    CHECK_EQ(probe::hunger(), 4);
    CHECK_EQ(probe::happy(), 4);
    CHECK_EQ(s.brightness(), 102);
  });
}

TEST(out_of_range_animal_and_look_do_not_crash) {
  Device d = hatchedPet(99);
  d.nvs.set("pet/color", 200);
  d.nvs.set("pet/hat", 200);
  d.nvs.set("pet/food", 200);
  d.nvs.set("pet/pick", 200);
  d.boot(Wake::Button, [](Stick& s) {
    s.run(3000);
    CHECK_EQ(probe::screen(), "home");
    s.tap(A);                                  // the food tray, on a food that exists
    s.tap(A);
    CHECK(s.runUntil([] { return probe::screen() == "home"; }, 6000));
    openTile(s, 4);
    s.run(1000);
    s.tap(B);
    s.tap(A);
  });
  CHECK(d.nvs.get("pet/color") < 6u);
  CHECK(d.nvs.get("pet/hat") < 5u);
  CHECK(d.nvs.get("pet/food") < 4u);
}

// The first roster was capybara, duck, blob, cat, octopus, owl, penguin,
// turtle, snail, ghost, rabbit. Today's is blob, cat, chick, dog, ghost,
// octopus, penguin; the duck became the chick.
TEST(old_roster_pet_keeps_its_animal) {
  const struct { int was; const char* name; unsigned now; } kept[] = {
    { 1, "chick", 2 }, { 2, "blob", 0 }, { 3, "cat", 1 }, { 4, "octopus", 5 }, { 6, "penguin", 6 }, { 9, "ghost", 4 } };
  for (const auto& k : kept) {
    Device d;
    d.nvs.set("pet/hatched", 1);
    d.nvs.set("pet/species", (uint32_t)k.was);
    d.nvs.set("pet/hunger", 3);
    const char* name = k.name;
    d.boot(Wake::Button, [name](Stick& s) {
      CHECK_EQ(probe::screen(), "home");       // and no lesson: it had a pet before there was one
      CHECK_EQ(probe::species(), name);
      CHECK_EQ(probe::hunger(), 3);
    });
    CHECK_EQ(d.nvs.get("pet/species"), k.now);
    CHECK_EQ(d.nvs.get("pet/roster"), 1u);
  }
}

TEST(old_roster_pet_whose_animal_is_gone_becomes_an_egg) {
  for (int was : { 0, 5, 7, 8, 10 }) {
    Device d;
    d.nvs.set("pet/hatched", 1);
    d.nvs.set("pet/species", (uint32_t)was);
    d.nvs.set("pet/stickers", 0x0007, 2);
    d.nvs.set("pet/hat", 2);
    d.boot(Wake::Button, [](Stick& s) {
      CHECK_EQ(probe::screen(), "egg");
      CHECK_EQ(probe::stickers(), 3);          // it keeps what it collected
    });
    CHECK_EQ(d.nvs.get("pet/hatched"), 0u);
    CHECK_EQ(d.nvs.get("pet/hat"), 2u);
  }
}

TEST(old_roster_parent_pick_is_carried_over) {
  Device d;
  d.nvs.set("pet/pick", 5);                    // the octopus, as species + 1
  d.boot(Wake::Cold);
  CHECK_EQ(d.nvs.get("pet/pick"), 6u);

  Device gone;
  gone.nvs.set("pet/pick", 1);                 // the capybara
  gone.boot(Wake::Cold);
  CHECK_EQ(gone.nvs.get("pet/pick"), 0u);      // back to any animal
}

TEST(migration_runs_once) {
  Device d = hatchedPet(4);                    // roster 1: the ghost, not old index 4
  d.boot(Wake::Button, [](Stick&) { CHECK_EQ(probe::species(), "ghost"); });
  CHECK_EQ(d.nvs.get("pet/species"), 4u);
}

TEST(out_of_range_look_is_put_right_at_the_next_save) {
  Device d = hatchedPet();
  d.nvs.set("pet/color", 200);
  d.nvs.set("pet/hat", 200);
  d.nvs.set("pet/food", 200);
  Boot b = d.boot(Wake::Button, [](Stick& s) { s.run(3 * 60000); });
  CHECK_EQ(b.end, End::Nap);
  CHECK_EQ(d.nvs.get("pet/color"), 0u);
  CHECK_EQ(d.nvs.get("pet/hat"), 0u);
  CHECK_EQ(d.nvs.get("pet/food"), 0u);
}
