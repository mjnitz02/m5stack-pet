#pragma once
#include <Arduino.h>
#include <Preferences.h>

// Header-only with file-static state: include from exactly one translation
// unit (main.cpp). Including from a second .cpp produces duplicate symbols.

// Everything the toy remembers across power-offs, backed by NVS. Save on
// events only (fed, played, hatched, setting changed, power-off) — NVS
// sectors have ~100K write cycles, so never on a per-frame timer.

static const uint8_t PET_MAX = 4;        // pips per need row
static const uint8_t PET_NO_SPECIES = 0xFF;   // nothing has hatched yet
static const uint8_t PET_ROSTER = 1;          // which animal list the saved indexes refer to

struct PetSave {
  bool     hatched;
  uint8_t  species;
  uint8_t  hunger;      // 0..PET_MAX, 0 = hungry
  uint8_t  happy;       // 0..PET_MAX
  bool     sound;
  uint8_t  bright;      // 0..4 → display brightness 51..255
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

static PetSave     _pet;
static TiltCal     _tiltCal;
static Preferences _prefs;

inline PetSave& pet() { return _pet; }
inline TiltCal& tiltCal() { return _tiltCal; }

inline void petLoad() {
  _prefs.begin("pet", true);
  _pet.hatched  = _prefs.getBool("hatched", false);
  _pet.species  = _prefs.getUChar("species", PET_NO_SPECIES);
  _pet.hunger   = _prefs.getUChar("hunger", 2);
  _pet.happy    = _prefs.getUChar("happy", 2);
  _pet.sound    = _prefs.getBool("sound", true);
  _pet.bright   = _prefs.getUChar("bright", 1);
  _pet.stayOn   = _prefs.getBool("stayon", false);
  _pet.awakeMin = _prefs.getUInt("awake", 0);
  _pet.food     = _prefs.getUChar("food", 0);
  _pet.stickers = _prefs.getUShort("stickers", 0);
  _pet.color    = _prefs.getUChar("color", 0);
  _pet.hat      = _prefs.getUChar("hat", 0);
  _pet.eggTile  = _prefs.getBool("eggtile", true);
  _pet.pick     = _prefs.getUChar("pick", 0);
  _pet.roster   = _prefs.getUChar("roster", 0);
  // A stick that already has a pet from before the lesson existed has seen it.
  _pet.learned  = _prefs.getBool("learned", _pet.hatched);
  _tiltCal.set = false;
  if (_prefs.isKey("tiltcal") && _prefs.getBytesLength("tiltcal") == sizeof(_tiltCal))
    _prefs.getBytes("tiltcal", &_tiltCal, sizeof(_tiltCal));
  _prefs.end();
  if (_pet.hunger > PET_MAX) _pet.hunger = PET_MAX;
  if (_pet.happy  > PET_MAX) _pet.happy  = PET_MAX;
  if (_pet.bright > 4)       _pet.bright = 1;
}

inline void petSave() {
  _prefs.begin("pet", false);
  _prefs.putBool("hatched", _pet.hatched);
  _prefs.putUChar("species", _pet.species);
  _prefs.putUChar("hunger", _pet.hunger);
  _prefs.putUChar("happy", _pet.happy);
  _prefs.putBool("sound", _pet.sound);
  _prefs.putUChar("bright", _pet.bright);
  _prefs.putBool("stayon", _pet.stayOn);
  _prefs.putUInt("awake", _pet.awakeMin);
  _prefs.putUChar("food", _pet.food);
  _prefs.putUShort("stickers", _pet.stickers);
  _prefs.putUChar("color", _pet.color);
  _prefs.putUChar("hat", _pet.hat);
  _prefs.putBool("eggtile", _pet.eggTile);
  _prefs.putUChar("pick", _pet.pick);
  _prefs.putUChar("roster", _pet.roster);
  _prefs.putBool("learned", _pet.learned);
  _prefs.end();
}

// Roster 0 was eleven animals in another order. Carry a saved pet over to
// the same animal in today's list; one whose animal is gone goes back to an
// egg and keeps everything it has collected. Call once after petLoad().
inline void petMigrateRoster() {
  if (_pet.roster == PET_ROSTER) return;
  // capybara, duck, blob, cat, octopus, owl, penguin, turtle, snail, ghost, rabbit
  static const uint8_t FROM0[11] = { 0xFF, 2, 0, 1, 5, 0xFF, 6, 0xFF, 0xFF, 4, 0xFF };
  if (_pet.species < 11) _pet.species = FROM0[_pet.species];
  if (_pet.species == PET_NO_SPECIES) _pet.hatched = false;
  if (_pet.pick) _pet.pick = _pet.pick <= 11 ? (uint8_t)(FROM0[_pet.pick - 1] + 1) : 0;   // 0xFF + 1 wraps to "any"
  _pet.roster = PET_ROSTER;
  petSave();
}

// Kept out of petSave(): it only changes when a grown-up calibrates.
inline void tiltCalSave() {
  _prefs.begin("pet", false);
  _prefs.putBytes("tiltcal", &_tiltCal, sizeof(_tiltCal));
  _prefs.end();
}

// Back to an egg: the pet and everything it has collected. Settings, the
// awake timer and having seen the buttons lesson survive.
inline void petReset() {
  _pet.hatched  = false;
  _pet.hunger   = 2;
  _pet.happy    = 2;
  _pet.food     = 0;
  _pet.stickers = 0;
  _pet.color    = 0;
  _pet.hat      = 0;
  petSave();
}

// The parent screen's reset: all of the above plus every setting and the
// awake timer, as if freshly flashed, so the buttons lesson plays again
// before the egg. Only the tilt calibration is kept; it
// describes the stick, not the pet, and has its own way to be cleared.
inline void petFactoryReset() {
  TiltCal keep = _tiltCal;
  _prefs.begin("pet", false);
  _prefs.clear();
  _prefs.end();
  petLoad();
  _pet.roster = PET_ROSTER;   // nothing saved, so nothing to carry over
  _tiltCal = keep;
  if (keep.set) tiltCalSave();
  petSave();
}
