#pragma once
#include <Arduino.h>
#include <Preferences.h>

// Header-only with file-static state: include from exactly one translation
// unit (main.cpp). Including from a second .cpp produces duplicate symbols.

// Everything the toy remembers across power-offs, backed by NVS. Save on
// events only (fed, played, hatched, setting changed, power-off) — NVS
// sectors have ~100K write cycles, so never on a per-frame timer.

static const uint8_t PET_MAX = 4;        // pips per need row

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
  _pet.species  = _prefs.getUChar("species", 0);
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
  _prefs.end();
}

// Kept out of petSave(): it only changes when a grown-up calibrates.
inline void tiltCalSave() {
  _prefs.begin("pet", false);
  _prefs.putBytes("tiltcal", &_tiltCal, sizeof(_tiltCal));
  _prefs.end();
}

// Back to an egg: the pet and everything it has collected. Settings and
// the awake timer survive.
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
