#pragma once

// The toy box menu. Header-only like activities.h, whose activities it
// starts: include from main.cpp only, after it.

// Two pages of six tiles. Side walks through them in reading order and rolls
// from one page onto the next. It always opens on the first tile.
const int TILE_W = 58, TILE_H = 66;
const uint32_t EGG_HOLD_MS = 3000;
uint8_t  menuSel = 0;
uint32_t eggHoldMs = 0, tileWobbleUntil = 0;

static uint8_t menuCount() { return pet().eggTile ? T_N : T_N - 1; }

static void openMenu(uint8_t sel) {
  mode = M_MENU;
  menuSel = sel < menuCount() ? sel : 0;
  eggHoldMs = 0;
  buddySetPeek(false);
  applyBrightness();
  lastInteractMs = millis();
}

static void backToEgg() {
  petReset();
  applyLook();
  enterEgg();
}

static void launchTile(uint8_t tile, uint32_t now) {
  switch (tile) {
    case T_WALK:     startWalk(); break;
    case T_BATH:     startBath(); break;
    case T_ROCK:     startRock(); break;
    case T_FREEZE:   startFreeze(now); break;
    case T_DRESS:    startDress(); break;
    case T_STICKERS: startStickers(); break;
    case T_CATCH:    startPlay(now); break;
    case T_DRAW:     startDraw(now); break;
    case T_MARBLE:   startMarble(now); break;
    case T_COPY:     startCopy(now); break;
    case T_HIDE:     startHide(now); break;
    case T_EGG:      tileWobbleUntil = now + 400; break;   // a tap only wobbles it
  }
}

// Hold blue on the egg tile: a ring fills over three seconds, then
// everything goes back to an unhatched egg. Letting go early cancels.
static bool tickEggHold(uint32_t now) {
  bool holding = menuSel == T_EGG && !swallowA && M5.BtnA.isPressed() && !M5.BtnB.isPressed();
  if (!holding) { eggHoldMs = 0; return false; }
  static uint8_t lastStep = 0;
  if (!eggHoldMs) { eggHoldMs = now; lastStep = 0; }
  uint32_t held = now - eggHoldMs;
  if (held < 250) return false;                  // an ordinary tap never shows the ring
  lastInteractMs = now;
  if (held >= EGG_HOLD_MS) {
    swallowA = true;
    eggHoldMs = 0;
    spr.fillSprite(INK);
    spr.pushSprite(0, 0);
    static const uint16_t TUNE[] = { 1568, 1047, 784 };
    jingle(TUNE, 3, 110);
    backToEgg();
    return true;
  }
  uint8_t step = held / 400;
  if (step != lastStep) { lastStep = step; beep(500 + step * 120, 50); }

  spr.fillSprite(BG);
  drawEggShape(CX, 108, 34, 44);
  int lit = held * 48 / EGG_HOLD_MS;
  for (int i = 0; i < 48; i++) {
    float ang = -1.5708f + i * 0.1309f;
    spr.fillCircle(CX + (int)(cosf(ang) * 58), 108 + (int)(sinf(ang) * 58), i < lit ? 3 : 1, i < lit ? HOT : GRID);
  }
  spr.fillCircle(CX, 210, 10 + ((now / 400) & 1) * 3, BTN_BLUE);
  return true;
}

static void tickMenu(uint32_t now, bool a, bool b) {
  uint8_t n = menuCount();
  if (b) { menuSel = (menuSel + 1) % n; beep(1800, 25); }
  if (tickEggHold(now)) return;
  if (a) { beep(2400, 25); launchTile(menuSel, now); if (mode != M_MENU) return; }

  spr.fillSprite(BG);
  uint8_t page = menuSel / 6;
  for (uint8_t i = page * 6; i < n && i < page * 6 + 6; i++) {
    int x = 6 + (i % 2) * 65, y = 12 + ((i % 6) / 2) * 72;
    if (i == menuSel) {
      drawFrame(x, y, TILE_W, TILE_H, 8);
    } else {
      spr.drawRoundRect(x, y, TILE_W, TILE_H, 8, GRID);
    }
    int wob = (i == T_EGG && before(now, tileWobbleUntil)) ? (((now / 60) & 1) ? 3 : -3) : 0;
    drawTileIcon(i, x + TILE_W / 2 + wob, y + TILE_H / 2);
  }
  uint8_t pages = (n + 5) / 6;
  for (uint8_t p = 0; p < pages; p++)
    spr.fillCircle(CX - (pages - 1) * 6 + p * 12, 233, 2, p == page ? INK : DIM);
}

// Hold-blue from inside something: back to the pet. A drawing that got some
// real pen time still counts as playing.
static void leaveToHome(uint32_t now) {
  if (mode == M_DRAW && drawTravel > 300) { finishActivity(now); return; }
  enterHome();
}
