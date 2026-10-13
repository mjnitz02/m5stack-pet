#pragma once
#include <stdint.h>

// Every colour on the screen, as 16-bit RGB565. The animals' own body
// colours are the one exception: each sits in its Species row.
const uint16_t BG    = 0x0000;
const uint16_t INK   = 0xFFFF;
const uint16_t DIM   = 0x8410;
const uint16_t GRID  = 0x39E7;   // unlit outlines
const uint16_t HOT   = 0xFA20;   // red-orange: warnings
const uint16_t BTN_BLUE = 0x041F;   // matches the front button
const uint16_t YEL   = 0xFFE0;
const uint16_t AQUA  = 0x07FF;
const uint16_t SHELL = 0xFFF4;   // egg
const uint16_t AMBER = 0xFD20;
const uint16_t ROSE  = 0xFB76;
const uint16_t TAN   = 0xE58E;
const uint16_t HEART = 0xF810;   // the hearts that float up from a happy pet
const uint16_t PLUM  = 0xA01F;

// Bare names the original board library supplied; M5GFX may or may not.
// M5GFX does bring its own VIOLET, PURPLE, PINK and the like into scope, so
// a new colour here needs a name it has not taken.
#ifndef GREEN
#define GREEN 0x07E0
#endif
#ifndef RED
#define RED 0xF800
#endif
