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
};

static PetSave     _pet;
static Preferences _prefs;

inline PetSave& pet() { return _pet; }

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
  _prefs.end();
}

// Back to an egg. Settings and the awake timer survive.
inline void petReset() {
  _pet.hatched = false;
  _pet.hunger  = 2;
  _pet.happy   = 2;
  petSave();
}
