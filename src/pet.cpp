#include "pet.h"
#include <Preferences.h>

static PetSave     _pet;
static TiltCal     _tiltCal;
static Preferences _prefs;

PetSave& pet() { return _pet; }
TiltCal& tiltCal() { return _tiltCal; }

void petLoad() {
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

void petSave() {
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

void petMigrateRoster() {
  if (_pet.roster == PET_ROSTER) return;
  // capybara, duck, blob, cat, octopus, owl, penguin, turtle, snail, ghost, rabbit
  static const uint8_t FROM0[11] = { 0xFF, 2, 0, 1, 5, 0xFF, 6, 0xFF, 0xFF, 4, 0xFF };
  if (_pet.species < 11) _pet.species = FROM0[_pet.species];
  if (_pet.species == PET_NO_SPECIES) _pet.hatched = false;
  if (_pet.pick) _pet.pick = _pet.pick <= 11 ? (uint8_t)(FROM0[_pet.pick - 1] + 1) : 0;   // 0xFF + 1 wraps to "any"
  _pet.roster = PET_ROSTER;
  petSave();
}

void tiltCalSave() {
  _prefs.begin("pet", false);
  _prefs.putBytes("tiltcal", &_tiltCal, sizeof(_tiltCal));
  _prefs.end();
}

void petReset() {
  _pet.hatched  = false;
  _pet.hunger   = 2;
  _pet.happy    = 2;
  _pet.food     = 0;
  _pet.stickers = 0;
  _pet.color    = 0;
  _pet.hat      = 0;
  petSave();
}

void petFactoryReset() {
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
