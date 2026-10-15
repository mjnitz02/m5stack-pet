#pragma once
#include <Arduino.h>

// Everything the toy remembers across power-offs, backed by NVS. Save on
// events only (fed, played, hatched, setting changed, power-off) — NVS
// sectors have ~100K write cycles, so never on a per-frame timer.

static const uint8_t PET_MAX = 4;        // pips per need row
static const uint8_t PET_NO_SPECIES = 0xFF;   // nothing has hatched yet
static const uint8_t PET_ROSTER = 1;          // which animal list the saved indexes refer to

// Screen brightness. Two steps is plenty: the screen is mostly black and
// reads fine in daylight, and brighter mostly shows the backlight bleeding
// through the black. Saves from when there were five steps (0..4) map 0 to
// low and the rest to high. The debug build has a third, dimmer step to try
// out before picking where low should sit.
enum : uint8_t { BRIGHT_LOW, BRIGHT_HIGH, BRIGHT_DIM };
#if defined(PET_LOG)
const uint8_t BRIGHT_STEPS = 3;
#else
const uint8_t BRIGHT_STEPS = 2;
#endif

struct PetSave {
  bool     hatched;
  uint8_t  species;
  uint8_t  hunger;      // 0..PET_MAX, 0 = hungry
  uint8_t  happy;       // 0..PET_MAX
  bool     sound;
  uint8_t  bright;      // BRIGHT_LOW, BRIGHT_HIGH (or BRIGHT_DIM in a debug build)
  bool     stayOn;      // battery testing: never auto power-off
  uint32_t awakeMin;    // cumulative powered-on minutes since last zero
  uint8_t  food;        // last food picked, so the tray opens on it
  uint16_t stickers;    // one bit per sticker earned
  uint8_t  color;       // dress-up colour index, 0 = the animal's own
  uint8_t  hat;         // dress-up hat, 0 = none
  bool     eggTile;     // show "new egg" in the toy box (testing aid)
  uint8_t  pick;        // what an egg hatches: 0 = any animal, else species + 1
  uint8_t  roster;      // PET_ROSTER when species and pick were saved
  bool     learned;     // the buttons lesson has been shown
};

// What the parent screen's tilt calibration measured: which way the sensor
// reads when the stick is tipped right (u) and tipped down the screen (v),
// and how much to scale each so a comfortable tilt is a full one.
struct TiltCal {
  bool  set;
  float u[3], v[3];
  float gainX, gainY;
};

PetSave& pet();
TiltCal& tiltCal();

void petLoad();
void petSave();

// Roster 0 was eleven animals in another order. Carry a saved pet over to
// the same animal in today's list; one whose animal is gone goes back to an
// egg and keeps everything it has collected. Call once after petLoad().
void petMigrateRoster();

// Kept out of petSave(): it only changes when a grown-up calibrates.
void tiltCalSave();

// Back to an egg: the pet and everything it has collected. Settings, the
// awake timer and having seen the buttons lesson survive.
void petReset();

// The parent screen's reset: all of the above plus every setting and the
// awake timer, as if freshly flashed, so the buttons lesson plays again
// before the egg. Only the tilt calibration is kept; it
// describes the stick, not the pet, and has its own way to be cleared.
void petFactoryReset();
