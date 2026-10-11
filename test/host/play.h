#pragma once
#include "probe.h"
#include "sim.h"

// Starting points and stretches of play that several tests share.

// A stick that has been through the lesson and hatched a pet. Animals are
// numbered as in SPECIES_TABLE (src/buddy.cpp); 5 is the octopus.
inline sim::Device hatchedPet(int species = 5, int hunger = 3, int happy = 3) {
  sim::Device d;
  d.nvs.set("pet/roster", 1);
  d.nvs.set("pet/learned", 1);
  d.nvs.set("pet/hatched", 1);
  d.nvs.set("pet/species", (uint32_t)species);
  d.nvs.set("pet/hunger", (uint32_t)hunger);
  d.nvs.set("pet/happy", (uint32_t)happy);
  return d;
}

// A stick that has seen the lesson and is waiting on its egg.
inline sim::Device unhatchedEgg() {
  sim::Device d;
  d.nvs.set("pet/roster", 1);
  d.nvs.set("pet/learned", 1);
  return d;
}

// Play the buttons lesson through from its first step.
inline void finishLesson(sim::Stick& s) {
  s.run(2500);                               // the screen shows a blue tap
  for (int i = 0; i < 3; i++) s.tap(sim::A);
  s.run(3500);                               // cheer, then it shows a side tap
  for (int i = 0; i < 4; i++) s.tap(sim::B);
  s.run(1500);                               // cheer; the third step is not shown first
  // Blue scores when the frame is on the star, side moves the frame: twelve
  // rounds of both is enough to land on three stars wherever they are.
  for (int i = 0; i < 12; i++) { s.tap(sim::A); s.tap(sim::B); }
  s.run(5000);                               // cheer, then it shows a hold
  s.hold(sim::A, 900);
  s.run(300);
  s.hold(sim::A, 900);
  s.runUntil([] { return probe::screen() != "learn"; }, 8000);
}

// Open the toy box from the pet screen and start the tile at this position
// (0 walk, 1 bath, 2 rock, 3 freeze, 4 dress, 5 stickers, 6 catch, 7 draw,
// 8 marble, 9 copycat, 10 hide and seek, 11 new egg).
inline void openTile(sim::Stick& s, int tile) {
  s.hold(sim::A, 800);
  CHECK_EQ(probe::screen(), "menu");
  for (int i = 0; i < tile; i++) s.tap(sim::B);
  CHECK_EQ(probe::menuTile(), tile);
  s.tap(sim::A);
}
