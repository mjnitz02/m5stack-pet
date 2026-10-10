#pragma once
#include <math.h>

// Every picture on the kid-facing screens, drawn from circles, rectangles,
// lines and triangles — no bitmaps. Header-only like pet.h: include from
// main.cpp only, after the sprite, the colours and the tile/food enums.

static void drawText(const char* s, int cx, int y, uint8_t size, uint16_t col) {
  spr.setTextSize(size);
  spr.setTextColor(col, BG);
  spr.setTextDatum(TC_DATUM);
  spr.drawString(s, cx, y);
  spr.setTextDatum(TL_DATUM);
  spr.setTextSize(1);
}

static void drawApple(int x, int y, int r, bool filled = true) {
  if (filled) {
    spr.fillCircle(x, y, r, RED);
    spr.drawLine(x, y - r, x + 1, y - r - r / 2 - 1, DIM);
    if (r >= 6) spr.fillTriangle(x + 1, y - r - 1, x + r - 1, y - r - 3, x + r / 2, y - r + 1, GREEN);
  } else {
    spr.drawCircle(x, y, r, DIM);
  }
}

// k = 1 is the pip on the home screen's happiness row.
static void drawHeart(int x, int y, int k, bool filled, uint16_t col) {
  if (filled) {
    spr.fillCircle(x - 2*k, y, 2*k, col);
    spr.fillCircle(x + 2*k, y, 2*k, col);
    spr.fillTriangle(x - 4*k, y + k, x + 4*k, y + k, x, y + 5*k, col);
  } else {
    spr.drawCircle(x - 2*k, y, 2*k, col);
    spr.drawCircle(x + 2*k, y, 2*k, col);
    spr.drawLine(x - 4*k, y + k, x, y + 5*k, col);
    spr.drawLine(x + 4*k, y + k, x, y + 5*k, col);
  }
}

static void drawBall(int x, int y, int r) {
  spr.fillCircle(x, y, r, AQUA);
  spr.drawLine(x - r, y, x + r, y, INK);
  spr.drawLine(x, y - r, x, y + r, INK);
  spr.drawCircle(x, y, r, INK);
}

static void drawCrescent(int x, int y, int r, uint16_t col) {
  spr.fillCircle(x - 2, y, r, col);
  spr.fillCircle(x + r / 4, y - r / 3, r - 1, BG);
}

static void drawMoon(int x, int y, int r) {
  spr.fillCircle(x - 2, y, r, YEL);
  spr.fillCircle(x + 2, y - 3, r - 1, BG);
  spr.setTextSize(1);
  spr.setTextColor(INK, BG);
  spr.setCursor(x + 5, y - 2);
  spr.print("z");
}

static void drawStar(int x, int y, int r, uint16_t col, bool filled = true) {
  int px[10], py[10];
  for (int i = 0; i < 10; i++) {
    float a = -1.5708f + i * 0.62832f, q = (i & 1) ? r * 0.45f : r;
    px[i] = x + (int)lroundf(cosf(a) * q);
    py[i] = y + (int)lroundf(sinf(a) * q);
  }
  if (filled) {
    for (int i = 0; i < 10; i += 2)
      spr.fillTriangle(px[i], py[i], px[(i + 9) % 10], py[(i + 9) % 10], px[i + 1], py[i + 1], col);
    spr.fillCircle(x, y, (int)(r * 0.4f), col);
  } else {
    for (int i = 0; i < 10; i++)
      spr.drawLine(px[i], py[i], px[(i + 1) % 10], py[(i + 1) % 10], col);
  }
}

static void drawFlower(int x, int y, int r) {
  for (int i = 0; i < 5; i++) {
    float a = i * 1.2566f - 1.5708f;
    spr.fillCircle(x + (int)lroundf(cosf(a) * r), y + (int)lroundf(sinf(a) * r), r * 7 / 10, ROSE);
  }
  spr.fillCircle(x, y, r * 6 / 10, YEL);
}

static void drawBush(int x, int y) {
  spr.fillCircle(x - 10, y + 3, 9, GREEN);
  spr.fillCircle(x + 10, y + 3, 9, GREEN);
  spr.fillCircle(x, y - 4, 11, GREEN);
  spr.fillRect(x - 18, y + 3, 37, 10, GREEN);
}

static void drawEye(int x, int y, bool open) {
  if (open) {
    spr.fillEllipse(x, y, 21, 12, INK);
    spr.fillCircle(x, y, 8, BTN_BLUE);
    spr.fillCircle(x, y, 3, BG);
    return;
  }
  // Shut: a drooping lid with three lashes.
  static const int8_t LX[7] = { -21, -14, -7, 0, 7, 14, 21 }, LY[7] = { 0, 4, 6, 7, 6, 4, 0 };
  for (int i = 0; i < 6; i++) {
    spr.drawLine(x + LX[i], y + LY[i], x + LX[i + 1], y + LY[i + 1], INK);
    spr.drawLine(x + LX[i], y + LY[i] + 1, x + LX[i + 1], y + LY[i + 1] + 1, INK);
  }
  spr.drawLine(x - 10, y + 6, x - 13, y + 11, INK);
  spr.drawLine(x, y + 8, x, y + 13, INK);
  spr.drawLine(x + 10, y + 6, x + 13, y + 11, INK);
}

static void drawEggShape(int x, int y, int rx, int ry) {
  spr.fillEllipse(x, y, rx, ry, SHELL);
  int cx[5] = { x - rx * 6 / 10, x - rx * 3 / 10, x, x + rx * 3 / 10, x + rx * 6 / 10 };
  int cy[5] = { y, y - ry / 5, y + ry / 16, y - ry / 5, y };
  for (int i = 0; i < 4; i++) {
    spr.drawLine(cx[i], cy[i], cx[i + 1], cy[i + 1], BG);
    if (rx > 20) spr.drawLine(cx[i], cy[i] + 1, cx[i + 1], cy[i + 1] + 1, BG);
  }
}

static void drawBubble(int x, int y, int r) {
  spr.drawCircle(x, y, r, AQUA);
  if (r > 4) spr.drawCircle(x, y, r - 1, AQUA);
  spr.fillCircle(x - r / 3, y - r / 3, 1, INK);
}

static void drawPencil(int x, int y) {
  spr.fillTriangle(x - 11, y + 3, x - 5, y + 9, x + 9, y - 5, YEL);
  spr.fillTriangle(x - 11, y + 3, x + 9, y - 5, x + 3, y - 11, YEL);
  spr.fillTriangle(x - 11, y + 3, x - 5, y + 9, x - 13, y + 11, TAN);
  spr.fillTriangle(x + 9, y - 5, x + 3, y - 11, x + 12, y - 8, ROSE);
  spr.fillTriangle(x + 3, y - 11, x + 12, y - 8, x + 6, y - 14, ROSE);
  spr.fillCircle(x - 12, y + 10, 1, INK);
  for (int i = 0; i < 4; i++) {
    int x0 = x - 12 + i * 7, y0 = y + ((i & 1) ? 12 : 16), y1 = y + ((i & 1) ? 16 : 12);
    spr.drawLine(x0, y0, x0 + 7, y1, AQUA);
  }
}

// --- Foods ------------------------------------------------------------------
static void drawFood(uint8_t food, int x, int y) {
  switch (food) {
    case FOOD_CARROT:
      spr.fillTriangle(x - 5, y - 5, x + 5, y - 5, x, y + 10, AMBER);
      spr.drawLine(x, y - 5, x - 3, y - 10, GREEN);
      spr.drawLine(x, y - 5, x, y - 11, GREEN);
      spr.drawLine(x, y - 5, x + 3, y - 10, GREEN);
      break;
    case FOOD_STRAWBERRY:
      spr.fillCircle(x - 3, y - 3, 4, RED);
      spr.fillCircle(x + 3, y - 3, 4, RED);
      spr.fillTriangle(x - 7, y - 2, x + 7, y - 2, x, y + 9, RED);
      spr.fillRect(x - 4, y - 8, 8, 2, GREEN);
      spr.fillRect(x - 1, y - 10, 2, 2, GREEN);
      spr.drawPixel(x - 3, y - 1, YEL);
      spr.drawPixel(x + 2, y - 1, YEL);
      spr.drawPixel(x, y + 3, YEL);
      break;
    case FOOD_ICECREAM:
      spr.fillTriangle(x - 5, y - 1, x + 5, y - 1, x, y + 10, TAN);
      spr.fillCircle(x, y - 4, 6, ROSE);
      break;
    default:
      drawApple(x, y + 1, 8);
  }
}

// --- Stickers ---------------------------------------------------------------
const uint8_t  STICKER_N   = 12;
const uint16_t STICKER_ALL = (1u << STICKER_N) - 1;

static void drawSticker(uint8_t idx, int x, int y) {
  switch (idx) {
    case 0:  drawStar(x, y, 12, YEL); break;
    case 1:  drawHeart(x, y - 4, 2, true, RED); break;
    case 2:  drawApple(x, y + 2, 9); break;
    case 3:  drawCrescent(x + 2, y, 10, YEL); break;
    case 4:  drawFlower(x, y, 7); break;
    case 5:  drawBall(x, y, 10); break;
    case 6:  buddyDrawHat(&spr, HAT_CROWN, x, y + 9, 2); break;
    case 7:  drawFood(FOOD_CARROT, x, y); break;
    case 8:  drawFood(FOOD_STRAWBERRY, x, y); break;
    case 9:  drawBubble(x - 3, y + 2, 9); drawBubble(x + 8, y - 7, 4); break;
    case 10: drawFood(FOOD_ICECREAM, x, y - 2); break;
    default: buddyDrawHat(&spr, HAT_BOW, x, y + 8, 2); break;
  }
}

// --- Toy box tiles ----------------------------------------------------------
static void drawTileIcon(uint8_t tile, int x, int y) {
  switch (tile) {
    case T_WALK:
      for (int i = 0; i < 4; i++) spr.fillCircle(x - 14 + i * 6, y + 12 - i * 4, 2, i < 2 ? INK : DIM);
      drawFlower(x + 11, y - 8, 5);
      break;
    case T_BATH:
      drawBubble(x - 6, y + 4, 8);
      drawBubble(x + 8, y - 5, 6);
      drawBubble(x + 7, y + 10, 3);
      break;
    case T_ROCK:
      drawCrescent(x + 1, y - 4, 10, YEL);
      drawText("<  >", x, y + 10, 1, INK);
      break;
    case T_FREEZE:   drawEye(x, y, true); break;
    case T_DRESS:    buddyDrawHat(&spr, HAT_CROWN, x, y + 9, 2); break;
    case T_STICKERS: drawStar(x, y, 15, YEL); break;
    case T_CATCH:
      drawApple(x + 2, y - 9, 6);
      drawText("\\___/", x, y + 4, 1, INK);
      break;
    case T_DRAW:     drawPencil(x, y); break;
    case T_MARBLE:
      for (int i = 0; i < 4; i++) spr.fillCircle(x - 16 + i * 4, y + 12 - i * 2, 1, DIM);
      spr.fillCircle(x + 4, y - 2, 9, AQUA);
      spr.fillCircle(x + 1, y - 5, 2, INK);
      break;
    case T_COPY:
      spr.fillCircle(x - 13, y, 6, BTN_BLUE);
      spr.fillRect(x - 4, y - 6, 8, 12, INK);
      spr.fillCircle(x + 13, y, 6, BTN_BLUE);
      break;
    case T_HIDE:
      drawText("o  o", x, y - 19, 1, buddyInkColor());
      drawBush(x, y + 3);
      break;
    case T_EGG:      drawEggShape(x, y, 13, 17); break;
  }
}
