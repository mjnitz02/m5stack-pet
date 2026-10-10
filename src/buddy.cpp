#include "buddy.h"
#include "buddy_common.h"
#include "compat.h"
#include <string.h>

extern TFT_eSprite spr;

// Mirrors PetState in main.cpp
enum { B_SLEEP, B_IDLE, B_BUSY, B_ATTENTION, B_CELEBRATE, B_DIZZY, B_HEART };

// ──────────────── shared geometry ────────────────
const int BUDDY_X_CENTER = 67;
const int BUDDY_CANVAS_W = 135;
const int BUDDY_Y_BASE   = 30;
const int BUDDY_Y_OVERLAY = 6;
const int BUDDY_CHAR_W   = 6;
const int BUDDY_CHAR_H   = 8;

// ──────────────── shared colors ────────────────
const uint16_t BUDDY_BG     = 0x0000;
const uint16_t BUDDY_HEART  = 0xF810;
const uint16_t BUDDY_DIM    = 0x8410;
const uint16_t BUDDY_YEL    = 0xFFE0;
const uint16_t BUDDY_WHITE  = 0xFFFF;
const uint16_t BUDDY_CYAN   = 0x07FF;
const uint16_t BUDDY_GREEN  = 0x07E0;
const uint16_t BUDDY_PURPLE = 0xA01F;
const uint16_t BUDDY_RED    = 0xF800;
const uint16_t BUDDY_BLUE   = 0x041F;

// ──────────────── shared rendering helpers ────────────────
// Render target indirection: defaults to the sprite, but can retarget to
// M5.Lcd via buddyRenderTo (both inherit TFT_eSPI). Coords stay
// fixed — species hardcode BUDDY_X_CENTER/BUDDY_Y_OVERLAY in their
// particle calls, so retargeting position would only move the body.
static TFT_eSPI* _tgt = &spr;
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
  _tgt->setTextColor(color, BUDDY_BG);
  _tgt->setCursor(x, yPx);
  for (int i = 0; i < len; i++) _tgt->print(line[i]);
}

// ──────────────── dress-up ────────────────
static uint16_t _tint = 0;
static uint8_t  _hat  = HAT_NONE;

void buddyDrawHat(TFT_eSPI* tgt, uint8_t hat, int x, int y, int k) {
  switch (hat) {
    case HAT_BOW:
      tgt->fillTriangle(x, y - 4*k, x - 6*k, y - 8*k, x - 6*k, y, 0xFB76);
      tgt->fillTriangle(x, y - 4*k, x + 6*k, y - 8*k, x + 6*k, y, 0xFB76);
      tgt->fillCircle(x, y - 4*k, k + 1, BUDDY_WHITE);
      break;
    case HAT_CROWN:
      tgt->fillRect(x - 7*k, y - 3*k, 14*k + 1, 3*k, BUDDY_YEL);
      tgt->fillTriangle(x - 7*k, y - 3*k, x - 7*k, y - 8*k, x - 2*k, y - 3*k, BUDDY_YEL);
      tgt->fillTriangle(x - 4*k, y - 3*k, x,       y - 9*k, x + 4*k, y - 3*k, BUDDY_YEL);
      tgt->fillTriangle(x + 7*k, y - 3*k, x + 7*k, y - 8*k, x + 2*k, y - 3*k, BUDDY_YEL);
      break;
    case HAT_PARTY:
      tgt->fillTriangle(x - 5*k, y, x + 5*k, y, x, y - 12*k, BUDDY_CYAN);
      tgt->fillRect(x - 3*k, y - 6*k, 6*k + 1, k, 0xFB76);
      tgt->fillCircle(x, y - 12*k, 2*k, BUDDY_YEL);
      break;
    case HAT_FLOWER: {
      static const int8_t PX[5] = { 0, 3, 2, -2, -3 }, PY[5] = { -3, -1, 2, 2, -1 };
      for (int i = 0; i < 5; i++)
        tgt->fillCircle(x + PX[i]*k, y - 5*k + PY[i]*k, 2*k, 0xFB76);
      tgt->fillCircle(x, y - 5*k, 2*k - 1, BUDDY_YEL);
      break;
    }
  }
}

void buddyPrintSprite(const char* const* lines, uint8_t nLines, int yOffset, uint16_t color, int xOff) {
  bool body = (color == buddyBodyColor());
  if (body && _tint) color = _tint;
  _tgt->setTextSize(_scale);
  int yBase = BUDDY_Y_BASE * _scale - (_scale - 1) * 14;
  for (uint8_t i = 0; i < nLines; i++) {
    buddyPrintLine(lines[i], yBase + (yOffset + i * BUDDY_CHAR_H) * _scale, color, xOff);
  }

  // The hat sits on the first row with ink in it, so it follows bobs, jumps
  // and the curled-up sleep pose. 2× re-centres every row; 1× keeps the
  // padding, so find the ink's own centre there.
  if (!body || _hat == HAT_NONE || nLines != 5) return;
  for (uint8_t i = 0; i < nLines; i++) {
    const char* l = lines[i];
    int len = strlen(l), first = 0, last = len - 1;
    while (first < len && l[first] == ' ') first++;
    if (first == len) continue;
    while (l[last] == ' ') last--;
    int x = BUDDY_X_CENTER + xOff * _scale;
    if (_scale == 1) x += (first + last + 1 - len) * BUDDY_CHAR_W / 2;
    buddyDrawHat(_tgt, _hat, x, yBase + (yOffset + i * BUDDY_CHAR_H + 3) * _scale, _scale);
    return;
  }
}

// Species pass 1× coords (relative to BUDDY_X_CENTER / BUDDY_Y_OVERLAY);
// transform here so the species files stay scale-agnostic.
void buddySetCursor(int x, int y) {
  _tgt->setCursor(BUDDY_X_CENTER + (x - BUDDY_X_CENTER) * _scale, y * _scale);
}
void buddySetColor(uint16_t fg)   { _tgt->setTextColor(fg, BUDDY_BG); }
void buddyPrint(const char* s)    { _tgt->setTextSize(_scale); _tgt->print(s); }

// ──────────────── species registry ────────────────
extern const Species CAPYBARA_SPECIES;
extern const Species DUCK_SPECIES;
extern const Species BLOB_SPECIES;
extern const Species CAT_SPECIES;
extern const Species OCTOPUS_SPECIES;
extern const Species OWL_SPECIES;
extern const Species PENGUIN_SPECIES;
extern const Species TURTLE_SPECIES;
extern const Species SNAIL_SPECIES;
extern const Species GHOST_SPECIES;
extern const Species RABBIT_SPECIES;

static const Species* SPECIES_TABLE[] = {
  &CAPYBARA_SPECIES, &DUCK_SPECIES, &BLOB_SPECIES, &CAT_SPECIES,
  &OCTOPUS_SPECIES, &OWL_SPECIES, &PENGUIN_SPECIES, &TURTLE_SPECIES,
  &SNAIL_SPECIES, &GHOST_SPECIES, &RABBIT_SPECIES,
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

void buddySetSpecies(const char* name) {
  for (uint8_t i = 0; i < N_SPECIES; i++) {
    if (strcmp(SPECIES_TABLE[i]->name, name) == 0) {
      currentSpeciesIdx = i;
      return;
    }
  }
}

const char* buddySpeciesName() {
  return SPECIES_TABLE[currentSpeciesIdx]->name;
}

uint8_t buddySpeciesCount() { return N_SPECIES; }

uint8_t buddySpeciesIdx() { return currentSpeciesIdx; }

uint16_t buddyBodyColor() { return SPECIES_TABLE[currentSpeciesIdx]->bodyColor; }

void buddySetLook(uint16_t tint, uint8_t hat) {
  _tint = tint;
  _hat = hat < HAT_N ? hat : (uint8_t)HAT_NONE;
  buddyInvalidate();
}

uint16_t buddyInkColor() { return _tint ? _tint : buddyBodyColor(); }

void buddyNextSpecies() {
  currentSpeciesIdx = (currentSpeciesIdx + 1) % N_SPECIES;
}

// Only redraw when tickCount actually changes — animations run at TICK_MS
// (5 fps), the loop runs at 60 fps, and the redraw is identical between
// ticks. Gating saves ~12× the fillRect + sprite-print work. State changes
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

// One-shot render to an arbitrary TFT_eSPI surface. Bypasses tick gating
// and the sprite fillRect — caller owns clearing. Advances the frame
// counter so animation runs even when buddyTick is bypassed. Always 1×.
void buddyRenderTo(TFT_eSPI* tgt, uint8_t personaState) {
  uint8_t prevS = _scale; _scale = 1;
  if (personaState >= 7) personaState = B_IDLE;
  uint32_t now = millis();
  if ((int32_t)(now - nextTickAt) >= 0) { nextTickAt = now + TICK_MS; tickCount++; }
  TFT_eSPI* prev = _tgt;
  _tgt = tgt;
  const Species* sp = SPECIES_TABLE[currentSpeciesIdx];
  if (sp->states[personaState]) sp->states[personaState](tickCount);
  _tgt = prev; _scale = prevS;
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
               (BUDDY_Y_BASE + 5 * BUDDY_CHAR_H + 12) * _scale, BUDDY_BG);

  const Species* sp = SPECIES_TABLE[currentSpeciesIdx];
  if (sp->states[personaState]) sp->states[personaState](tickCount);
}
