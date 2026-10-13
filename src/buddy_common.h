#pragma once
#include <stdint.h>

// Shared constants and helpers for buddy species files.
// Each species file (src/buddies/<name>.cpp) includes this header
// and defines its 7 state functions.
#include "buddy.h"
#include "palette.h"

// Geometry — shared layout for all species
extern const int BUDDY_X_CENTER;
extern const int BUDDY_CANVAS_W;
extern const int BUDDY_Y_BASE;
extern const int BUDDY_Y_OVERLAY;
extern const int BUDDY_CHAR_W;
extern const int BUDDY_CHAR_H;

// Print one line centered around BUDDY_X_CENTER, optionally x-offset.
void buddyPrintLine(const char* line, int yPx, uint16_t color, int xOff = 0);

// Print a 5-line pose in the animal's colour, or its dress-up tint, with its
// hat on. yOffset is added to BUDDY_Y_BASE for the top row.
void buddyPrintBody(const char* const* lines, int yOffset = 0, int xOff = 0);

// Set sprite text color directly + cursor (for ad-hoc particle drawing).
void buddySetCursor(int x, int y);
void buddySetColor(uint16_t fg);
void buddyPrint(const char* s);
// How far today's hat stands above the head at 1x, for anything that has to
// land on top of it.
int  buddyHatHeight();

// The drifting characters most animals share. Positions are 1x pixels from
// (BUDDY_X_CENTER, BUDDY_Y_OVERLAY); the arguments are where animals differ.
// Three Zs floating up and right. The second sits 6 right and 4 up of the
// first; y3 is the third's own height.
void buddyZs(uint32_t t, uint8_t period, uint8_t lag2, uint8_t lag3, int x, int y, int y3,
             uint16_t first = DIM);
void buddyBangs(uint32_t t, int dx);                                  // two blinking "!"
// Falling "*" and one other glyph, in five colours.
void buddyConfetti(uint32_t t, const char* other, uint16_t fourth = INK, uint16_t fifth = GREEN);
void buddyOrbit(uint32_t t, const char* glyph, uint16_t color, int y = 6);   // one glyph circling the head
void buddyStars(uint32_t t, int y = 6, uint16_t second = YEL);         // two of them, opposite
void buddyHearts(uint32_t t);                                         // rising "v" hearts
