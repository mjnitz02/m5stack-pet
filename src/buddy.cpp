#include "buddy.h"
#include "buddy_common.h"
#include "compat.h"
#include <string.h>

extern TFT_eSprite spr;

// Mirrors PetState in app.h
enum { B_SLEEP, B_IDLE, B_PLAY, B_ATTENTION, B_CELEBRATE, B_DIZZY, B_HEART };

// ──────────────── shared geometry ────────────────
const int BUDDY_X_CENTER = 67;
const int BUDDY_CANVAS_W = 135;
const int BUDDY_Y_BASE   = 30;
const int BUDDY_Y_OVERLAY = 6;
const int BUDDY_CHAR_W   = 6;
const int BUDDY_CHAR_H   = 8;

// ──────────────── shared rendering helpers ────────────────
// 2× on home screen, 1× in peek (mini-game). Species
// art is space-padded to a fixed width for alignment at 1×; at 2× we trim
// and re-center per line so the padding doesn't push ink off-screen.
static uint8_t _scale = 1;

void buddyPrintLine(const char* line, int yPx, uint16_t color, int xOff) {
  int len = strlen(line);
  if (_scale > 1) {
    while (len && line[len-1] == ' ') len--;
    while (len && *line == ' ')       { line++; len--; }
  }
  int w = len * BUDDY_CHAR_W * _scale;
  int x = BUDDY_X_CENTER - w / 2 + xOff * _scale;
  spr.setTextColor(color, BG);
  spr.setCursor(x, yPx);
  for (int i = 0; i < len; i++) spr.print(line[i]);
}

// ──────────────── dress-up ────────────────
static uint16_t _tint = 0;
static uint8_t  _hat  = HAT_NONE;

void buddyDrawHat(uint8_t hat, int x, int y, int k) {
  switch (hat) {
    case HAT_BOW:
      spr.fillTriangle(x, y - 4*k, x - 6*k, y - 8*k, x - 6*k, y, ROSE);
      spr.fillTriangle(x, y - 4*k, x + 6*k, y - 8*k, x + 6*k, y, ROSE);
      spr.fillCircle(x, y - 4*k, k + 1, INK);
      break;
    case HAT_CROWN:
      spr.fillRect(x - 7*k, y - 3*k, 14*k + 1, 3*k, YEL);
      spr.fillTriangle(x - 7*k, y - 3*k, x - 7*k, y - 8*k, x - 2*k, y - 3*k, YEL);
      spr.fillTriangle(x - 4*k, y - 3*k, x,       y - 9*k, x + 4*k, y - 3*k, YEL);
      spr.fillTriangle(x + 7*k, y - 3*k, x + 7*k, y - 8*k, x + 2*k, y - 3*k, YEL);
      break;
    case HAT_PARTY:
      spr.fillTriangle(x - 5*k, y, x + 5*k, y, x, y - 12*k, AQUA);
      spr.fillRect(x - 3*k, y - 6*k, 6*k + 1, k, ROSE);
      spr.fillCircle(x, y - 12*k, 2*k, YEL);
      break;
    case HAT_FLOWER: {
      static const int8_t PX[5] = { 0, 3, 2, -2, -3 }, PY[5] = { -3, -1, 2, 2, -1 };
      for (int i = 0; i < 5; i++)
        spr.fillCircle(x + PX[i]*k, y - 5*k + PY[i]*k, 2*k, ROSE);
      spr.fillCircle(x, y - 5*k, 2*k - 1, YEL);
      break;
    }
  }
}

void buddyPrintBody(const char* const* lines, int yOffset, int xOff) {
  const uint8_t nLines = 5;
  spr.setTextSize(_scale);
  int yBase = BUDDY_Y_BASE * _scale - (_scale - 1) * 14;
  for (uint8_t i = 0; i < nLines; i++) {
    buddyPrintLine(lines[i], yBase + (yOffset + i * BUDDY_CHAR_H) * _scale, buddyInkColor(), xOff);
  }

  // The hat sits on the first row with ink in it, so it follows bobs, jumps
  // and the curled-up sleep pose. 2× re-centres every row; 1× keeps the
  // padding, so find the ink's own centre there.
  if (_hat == HAT_NONE) return;
  for (uint8_t i = 0; i < nLines; i++) {
    const char* l = lines[i];
    int len = strlen(l), first = 0, last = len - 1;
    while (first < len && l[first] == ' ') first++;
    if (first == len) continue;
    while (l[last] == ' ') last--;
    int x = BUDDY_X_CENTER + xOff * _scale;
    if (_scale == 1) x += (first + last + 1 - len) * BUDDY_CHAR_W / 2;
    buddyDrawHat(_hat, x, yBase + (yOffset + i * BUDDY_CHAR_H + 3) * _scale, _scale);
    return;
  }
}

// Species pass 1× coords (relative to BUDDY_X_CENTER / BUDDY_Y_OVERLAY);
// transform here so the species files stay scale-agnostic.
void buddySetCursor(int x, int y) {
  spr.setCursor(BUDDY_X_CENTER + (x - BUDDY_X_CENTER) * _scale, y * _scale);
}
void buddySetColor(uint16_t fg)   { spr.setTextColor(fg, BG); }
void buddyPrint(const char* s)    { spr.setTextSize(_scale); spr.print(s); }

// ──────────────── shared overlays ────────────────
static void glyphAt(const char* s, uint16_t color, int dx, int dy) {
  buddySetColor(color);
  buddySetCursor(BUDDY_X_CENTER + dx, BUDDY_Y_OVERLAY + dy);
  buddyPrint(s);
}

void buddyZs(uint32_t t, uint8_t period, uint8_t lag2, uint8_t lag3, int x, int y, int y3, uint16_t first) {
  int p1 = t % period;
  int p2 = (t + lag2) % period;
  int p3 = (t + lag3) % period;
  glyphAt("z", first,       x + p1,         y - p1 * 2);
  glyphAt("Z", INK, x + 6 + p2,     y - 4 - p2);
  glyphAt("z", DIM,   x - 4 + p3 / 2, y3 - p3 / 2);
}

void buddyBangs(uint32_t t, int dx) {
  if ((t / 2) & 1) glyphAt("!", YEL, -dx, 0);
  if ((t / 3) & 1) glyphAt("!", YEL, dx, 4);
}

void buddyConfetti(uint32_t t, const char* other, uint16_t fourth, uint16_t fifth) {
  const uint16_t cols[] = { YEL, HEART, AQUA, fourth, fifth };
  for (int i = 0; i < 6; i++) {
    int phase = (t * 2 + i * 11) % 22;
    int y = BUDDY_Y_OVERLAY - 6 + phase;
    if (y > BUDDY_Y_BASE + 20 || y < 0) continue;
    glyphAt((i + (int)(t / 2)) & 1 ? "*" : other, cols[i % 5], -36 + i * 14, -6 + phase);
  }
}

void buddyOrbit(uint32_t t, const char* glyph, uint16_t color, int y) {
  static const int8_t OX[] = { 0, 5, 7, 5, 0, -5, -7, -5 };
  static const int8_t OY[] = { -5, -3, 0, 3, 5, 3, 0, -3 };
  uint8_t p = t % 8;
  glyphAt(glyph, color, OX[p] - 2, y + OY[p]);
}

void buddyStars(uint32_t t, int y, uint16_t second) {
  buddyOrbit(t, "*", AQUA, y);
  buddyOrbit(t + 4, "*", second, y);
}

void buddyHearts(uint32_t t) {
  buddySetColor(HEART);
  for (int i = 0; i < 5; i++) {
    int phase = (t + i * 4) % 16;
    int y = BUDDY_Y_OVERLAY + 16 - phase;
    if (y < -2 || y > BUDDY_Y_BASE) continue;
    buddySetCursor(BUDDY_X_CENTER - 20 + i * 8 + ((phase / 3) & 1) * 2 - 1, y);
    buddyPrint("v");
  }
}

// ──────────────── species registry ────────────────
extern const Species BLOB_SPECIES;
extern const Species CAT_SPECIES;
extern const Species CHICK_SPECIES;
extern const Species DOG_SPECIES;
extern const Species GHOST_SPECIES;
extern const Species OCTOPUS_SPECIES;
extern const Species PENGUIN_SPECIES;

// The saved pet is an index into this table. Reordering or removing an entry
// needs a new PET_ROSTER and a row in petMigrateRoster() (src/pet.h), or
// every hatched pet turns into a different animal.
static const Species* SPECIES_TABLE[] = {
  &BLOB_SPECIES, &CAT_SPECIES, &CHICK_SPECIES, &DOG_SPECIES,
  &GHOST_SPECIES, &OCTOPUS_SPECIES, &PENGUIN_SPECIES,
};
static const uint8_t N_SPECIES = sizeof(SPECIES_TABLE) / sizeof(SPECIES_TABLE[0]);
static uint8_t currentSpeciesIdx = 0;

// ──────────────── tick state ────────────────
static uint32_t tickCount  = 0;
static uint32_t nextTickAt = 0;
static const uint32_t TICK_MS = 200;

void buddyInit() {
  tickCount = 0;
  nextTickAt = 0;
}

void buddySetSpeciesIdx(uint8_t idx) {
  if (idx < N_SPECIES) currentSpeciesIdx = idx;
}

const char* buddySpeciesName() {
  return SPECIES_TABLE[currentSpeciesIdx]->name;
}

uint8_t buddySpeciesCount() { return N_SPECIES; }

uint16_t buddyBodyColor() { return SPECIES_TABLE[currentSpeciesIdx]->bodyColor; }

void buddySetLook(uint16_t tint, uint8_t hat) {
  _tint = tint;
  _hat = hat < HAT_N ? hat : (uint8_t)HAT_NONE;
  buddyInvalidate();
}

uint16_t buddyInkColor() { return _tint ? _tint : buddyBodyColor(); }

int buddyHatHeight() {
  static const uint8_t TALL[HAT_N] = { 0, 8, 9, 14, 10 };
  return TALL[_hat];
}

// Only redraw when tickCount actually changes — animations run at TICK_MS
// (5 fps), the loop runs faster, and the redraw is identical between
// ticks. State changes
// also need a redraw even mid-tick so transitions appear instantly.
static uint8_t lastDrawnState = 0xFF;
static uint8_t lastDrawnSpecies = 0xFF;
void buddyInvalidate() { lastDrawnState = 0xFF; }

void buddySetPeek(bool peek) {
  uint8_t s = peek ? 1 : 2;
  if (s == _scale) return;
  _scale = s;
  buddyInvalidate();
}

void buddyTick(uint8_t personaState) {
  uint32_t now = millis();
  bool ticked = false;
  if ((int32_t)(now - nextTickAt) >= 0) {
    nextTickAt = now + TICK_MS;
    tickCount++;
    ticked = true;
  }

  if (personaState >= 7) personaState = B_IDLE;
  if (!ticked && personaState == lastDrawnState
              && currentSpeciesIdx == lastDrawnSpecies) {
    return;
  }
  lastDrawnState = personaState;
  lastDrawnSpecies = currentSpeciesIdx;

  // Clear the whole render strip — at 2× the body reaches y≈126, at 1× ≈82.
  spr.fillRect(0, 0, BUDDY_CANVAS_W,
               (BUDDY_Y_BASE + 5 * BUDDY_CHAR_H + 12) * _scale, BG);

  const Species* sp = SPECIES_TABLE[currentSpeciesIdx];
  if (sp->states[personaState]) sp->states[personaState](tickCount);
}
