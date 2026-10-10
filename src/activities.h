#pragma once

// The toy box activities. Header-only with file-static state, like pet.h:
// include from main.cpp only, after the IMU helpers and finishActivity().
//
// Rules they all keep: no words, no score, no way to lose. Each ends by
// calling finishActivity(), which adds a heart and sometimes a sticker.
// The ones on the first page of the toy box are the ones that get a kid
// moving; the numbers that decide "enough movement" are guesses until they
// have been tried in small hands, so they sit at the top of each section.

uint32_t actUntil = 0;     // one shared timer for the short cheer/dizzy beats
static bool actBeat(uint32_t now) { return (int32_t)(now - actUntil) < 0; }

static void actBegin(Mode m, bool peek) {
  mode = m;
  buddySetPeek(peek);
  spr.fillSprite(BG);
  buddyInvalidate();
  actUntil = 0;
  lastInteractMs = millis();
}

static void drawPips(uint8_t n, uint8_t done, int y) {
  for (int i = 0; i < n; i++) {
    int x = CX - (n - 1) * 8 + i * 16;
    if (i < done) spr.fillCircle(x, y, 3, INK);
    else          spr.drawCircle(x, y, 3, DIM);
  }
}

// --- Walk -------------------------------------------------------------------
// Not a step counter. Any movement of the stick — walking, jumping, dancing —
// adds up, and enough of it lights the next dot on the path to the flower.
// Standing still just pauses. Aim: about a minute of real moving.
const float   WALK_FLOOR   = 0.06f;   // ignore hand tremor below this
const float   WALK_PER_DOT = 18.0f;
const uint8_t WALK_DOTS    = 10;
float   walkAcc = 0;
uint8_t walkDots = 0;

static void startWalk() {
  actBegin(M_WALK, true);
  walkAcc = 0;
  walkDots = 0;
}

static void tickWalk(uint32_t now) {
  if (imuFresh && imuDelta > WALK_FLOOR) walkAcc += fminf(imuDelta, 1.0f) - WALK_FLOOR;
  if (walkAcc >= WALK_PER_DOT) {
    walkAcc = 0;
    walkDots++;
    beep(1100 + walkDots * 90, 50);
    actUntil = now + 700;
    if (walkDots >= WALK_DOTS) { finishActivity(now); return; }
  }

  buddyTick(actBeat(now) ? P_HEART : P_IDLE);
  spr.fillRect(0, FIELD_TOP, W, H - FIELD_TOP, BG);
  spr.drawLine(112, 110, 112, 124, GREEN);
  drawFlower(112, 104, 6);
  for (int i = 0; i < WALK_DOTS; i++) {
    int x = 20 + (int)(i * 9.5f + sinf(i * 0.9f) * 10), y = 222 - i * 11;
    if (i < walkDots) { spr.fillCircle(x, y, 3, INK); continue; }
    spr.drawCircle(x, y, 3, DIM);
    // The next dot swells as they move, so the screen answers right away.
    if (i == walkDots) spr.fillCircle(x, y, (int)(walkAcc * 3 / WALK_PER_DOT), INK);
  }
}

// --- Dance party ------------------------------------------------------------
// Walk in party clothes, opened from the middle icon on the pet screen. The
// same "any movement adds up", shown as five rainbow bars that fill one after
// another while the pet parties. Aim: about half a minute of dancing.
const float   DANCE_PER_BAR = 18.0f;
const float   DANCE_LIVELY  = 0.12f;   // movement big enough to set the party off
const uint8_t DANCE_BARS    = 5;
const int     DANCE_BAR_H   = 58;
static const uint16_t DANCE_COLORS[DANCE_BARS] = { ROSE, AMBER, YEL, GREEN, AQUA };
float    danceAcc = 0;
uint8_t  danceBars = 0;
uint32_t danceMovedMs = 0;

static void startDance(uint32_t now) {
  actBegin(M_DANCE, false);
  danceAcc = 0;
  danceBars = 0;
  danceMovedMs = now - 1000;
}

static void tickDance(uint32_t now) {
  if (imuFresh && imuDelta > WALK_FLOOR) danceAcc += fminf(imuDelta, 1.0f) - WALK_FLOOR;
  if (imuFresh && imuDelta > DANCE_LIVELY) danceMovedMs = now;
  if (danceAcc >= DANCE_PER_BAR) {
    danceAcc = 0;
    danceBars++;
    beep(900 + danceBars * 180, 60);
    actUntil = now + 700;
    if (danceBars >= DANCE_BARS) { finishActivity(now); return; }
  }
  bool moving = now - danceMovedMs < 800;

  buddyTick(actBeat(now) ? P_HEART : moving ? P_CELEBRATE : P_IDLE);
  if (moving) {                      // sparkles round the pet, colours chasing
    static const int16_t SPARK[4][2] = { { 16, 50 }, { 118, 44 }, { 14, 128 }, { 120, 124 } };
    uint32_t beat = now / 200;
    for (int i = 0; i < 4; i++)
      drawStar(SPARK[i][0], SPARK[i][1], ((beat + i) & 1) ? 6 : 4, DANCE_COLORS[(beat + i) % DANCE_BARS]);
  }

  spr.fillRect(0, BOTTOM_TOP, W, H - BOTTOM_TOP, BG);
  for (int i = 0; i < DANCE_BARS; i++) {
    int x = 10 + i * 24, h = 0;
    spr.drawRoundRect(x, 172, 19, DANCE_BAR_H + 4, 4, GRID);
    if (i < danceBars) {
      // Full bars bounce like a music meter while the dancing goes on.
      h = DANCE_BAR_H - (moving ? (int)((now / 110 + i * 3) % 4) * 5 : 0);
    } else if (i == danceBars) {
      h = (int)(danceAcc * DANCE_BAR_H / DANCE_PER_BAR);
    }
    if (h > 0) spr.fillRoundRect(x + 2, 174 + DANCE_BAR_H - h, 15, h, 2, DANCE_COLORS[i]);
  }
}

// --- Bath -------------------------------------------------------------------
// Shake the bubbles off.
const uint8_t BATH_N = 8;
static const int16_t BUBBLE[BATH_N][3] = {
  { 34, 70, 9 }, { 98, 66, 11 }, { 60, 58, 6 }, { 84, 100, 8 },
  { 42, 108, 10 }, { 106, 116, 7 }, { 66, 122, 9 }, { 24, 92, 6 }
};
uint8_t  bathLeft = 0;
bool     bathDone = false;
uint32_t bathShakeMs = 0;

static void startBath() {
  actBegin(M_BATH, false);
  bathLeft = BATH_N;
  bathDone = false;
}

static void tickBath(uint32_t now, bool shook) {
  if (bathDone) {
    if (!actBeat(now)) { finishActivity(now); return; }
    buddyTick(P_HEART);
    bool tw = (now / 200) & 1;
    drawStar(20, 60, tw ? 6 : 4, YEL);
    drawStar(114, 52, tw ? 4 : 6, YEL);
    drawStar(18, 130, tw ? 4 : 5, YEL);
    drawStar(116, 128, tw ? 5 : 4, YEL);
    spr.fillRect(0, BOTTOM_TOP, W, H - BOTTOM_TOP, BG);
    return;
  }
  if (shook && now - bathShakeMs > 250) {
    bathShakeMs = now;
    bathLeft -= bathLeft >= 2 ? 2 : bathLeft;
    beep(1400 + (BATH_N - bathLeft) * 100, 40);
    buddyInvalidate();              // wipe the popped bubbles straight away
    if (!bathLeft) { bathDone = true; actUntil = now + 1800; }
  }

  buddyTick(P_IDLE);
  for (int i = 0; i < bathLeft; i++) drawBubble(BUBBLE[i][0], BUBBLE[i][1], BUBBLE[i][2]);
  spr.fillRect(0, BOTTOM_TOP, W, H - BOTTOM_TOP, BG);
  drawText("((      ))", CX + (((now / 250) & 1) ? 2 : -2), 190, 2, DIM);
}

// --- Rock to sleep ----------------------------------------------------------
// Not a sway detector. Any gentle rocking, tipping or swinging of the stick
// pushes the ball along its track; it runs to one end, turns round and runs
// back, as if every bit of movement were a perfect sway. Each crossing fills
// a moon; six and it's bed. Holding still just parks the ball.
const float   ROCK_FLOOR     = 0.04f;  // a hand trying to hold still stays under this
const float   ROCK_CAP       = 0.15f;  // a hard shake is no faster than a gentle rock
const float   ROCK_PER_CROSS = 2.0f;   // about two seconds of rocking per crossing
const uint8_t ROCK_MOONS     = 6;
const int     ROCK_REACH     = 44;     // px the ball travels either side of centre
float   rockPos = 0, rockShown = 0;    // 0..1 along the current crossing; eased x offset
int8_t  rockDir = 1;
uint8_t rockMoons = 0;

static void startRock() {
  actBegin(M_ROCK, false);
  rockPos = 0.5f;                      // starts in the middle, so the first moon comes quickly
  rockShown = 0;
  rockDir = 1;
  rockMoons = 0;
}

static void tickRock(uint32_t now) {
  if (imuFresh && imuTurn > ROCK_FLOOR) {
    lastInteractMs = now;
    rockPos += (fminf(imuTurn, ROCK_CAP) - ROCK_FLOOR) / ROCK_PER_CROSS;
    if (rockPos >= 1.0f) {
      rockPos = 0;
      rockDir = -rockDir;
      rockMoons++;
      beep(520, 70);
      if (rockMoons >= ROCK_MOONS) {
        if (pet().happy < PET_MAX) pet().happy++;
        lastHappyMs = now;
        petSave();
        startBed(now, true);
        return;
      }
    }
  }

  buddyTick(rockMoons >= ROCK_MOONS / 2 ? P_SLEEP : P_IDLE);
  spr.fillRect(0, BOTTOM_TOP, W, H - BOTTOM_TOP, BG);
  drawText("<        >", CX, 172, 2, DIM);
  // Slow into each end and out of it again, like something swinging.
  float u = rockPos * rockPos * (3 - 2 * rockPos);
  if (rockDir < 0) u = 1 - u;
  rockShown += ((u * 2 - 1) * ROCK_REACH - rockShown) * 0.35f;
  spr.fillCircle(CX + (int)lroundf(rockShown), 180, 4, INK);
  for (int i = 0; i < ROCK_MOONS; i++) drawCrescent(19 + i * 20, 216, 7, i < rockMoons ? YEL : GRID);
}

// --- Freeze -----------------------------------------------------------------
// Wiggle while the pet's eyes are shut. When the big eye opens, hold still
// to earn a star. Moving only makes the pet dizzy and the round comes again.
const float    FREEZE_MOVE     = 0.25f;
const uint32_t FREEZE_GRACE_MS = 600;    // time to notice the eye and stop
const uint32_t FREEZE_HOLD_MS  = 2500;
const uint8_t  FREEZE_STARS    = 5;
enum { FZ_WIGGLE, FZ_WATCH, FZ_CAUGHT, FZ_GOOD };
uint8_t  fzPhase = FZ_WIGGLE, fzStars = 0;
uint32_t fzUntil = 0, fzGraceUntil = 0;

static void fzWiggle(uint32_t now) {
  fzPhase = FZ_WIGGLE;
  fzUntil = now + 2000 + esp_random() % 2500;
}

static void startFreeze(uint32_t now) {
  actBegin(M_FREEZE, false);
  fzStars = 0;
  fzWiggle(now);
}

static void tickFreeze(uint32_t now) {
  bool over = (int32_t)(now - fzUntil) >= 0;
  PetState st = P_SLEEP;
  switch (fzPhase) {
    case FZ_WIGGLE:
      if (over) {
        fzPhase = FZ_WATCH;
        fzUntil = now + FREEZE_HOLD_MS;
        fzGraceUntil = now + FREEZE_GRACE_MS;
        beep(1400, 120);
      }
      break;
    case FZ_WATCH:
      st = P_ATTENTION;
      lastInteractMs = now;          // standing still is the game, not idling
      if ((int32_t)(now - fzGraceUntil) >= 0 && imuFresh && imuDelta > FREEZE_MOVE) {
        fzPhase = FZ_CAUGHT;
        fzUntil = now + 1600;
        beep(400, 150);
      } else if (over) {
        fzStars++;
        beep(2000, 80);
        if (fzStars >= FREEZE_STARS) { finishActivity(now); return; }
        fzPhase = FZ_GOOD;
        fzUntil = now + 900;
      }
      break;
    case FZ_CAUGHT: st = P_DIZZY; if (over) fzWiggle(now); break;
    case FZ_GOOD:   st = P_HEART; if (over) fzWiggle(now); break;
  }

  buddyTick(st);
  spr.fillRect(0, BOTTOM_TOP, W, H - BOTTOM_TOP, BG);
  drawEye(CX, 182, fzPhase == FZ_WATCH);
  for (int i = 0; i < FREEZE_STARS; i++)
    drawStar(CX - (FREEZE_STARS - 1) * 11 + i * 22, 222, 7, i < fzStars ? YEL : DIM, i < fzStars);
}

// --- Draw -------------------------------------------------------------------
// Tilt moves the pen, blue changes colour, a shake wipes the page. The
// sprite keeps whatever was drawn, so the field is never cleared per frame.
const float PEN_SPEED = 9.0f;          // px per frame at 1 g of tilt
const float PEN_DEAD  = 0.04f;
static const uint16_t PEN_COLORS[] = { INK, AQUA, ROSE, YEL, GREEN, AMBER };
const uint8_t PEN_N = sizeof(PEN_COLORS) / sizeof(PEN_COLORS[0]);
float   penX = 0, penY = 0, drawTravel = 0;
uint8_t penCol = 0;

static float penDead(float v) {
  if (fabsf(v) < PEN_DEAD) return 0;
  return v > 0 ? v - PEN_DEAD : v + PEN_DEAD;
}

static void startDraw(uint32_t now) {
  actBegin(M_DRAW, true);
  penX = CX;
  penY = (FIELD_TOP + H) / 2;
  drawTravel = 0;
  tiltBegin(now);
}

static void tickDraw(uint32_t now, bool a, bool shook) {
  uint16_t col = PEN_COLORS[penCol];
  if (a) { penCol = (penCol + 1) % PEN_N; col = PEN_COLORS[penCol]; beep(2400, 25); }
  if (shook) {
    spr.fillRect(0, FIELD_TOP, W, H - FIELD_TOP, BG);
    beep(700, 60);
    tiltBegin(now);                 // let the hand settle before drawing again
  }
  if (tiltUpdate(now)) {
    float nx = penX + TILT_SIGN * penDead(tiltX) * PEN_SPEED;
    float ny = penY + TILT_SIGN_Y * penDead(tiltY) * PEN_SPEED;
    if (nx < 2) nx = 2;
    if (nx > W - 3) nx = W - 3;
    if (ny < FIELD_TOP + 2) ny = FIELD_TOP + 2;
    if (ny > H - 3) ny = H - 3;
    int x0 = (int)penX, y0 = (int)penY, x1 = (int)nx, y1 = (int)ny;
    spr.drawLine(x0, y0, x1, y1, col);
    spr.drawLine(x0 + 1, y0, x1 + 1, y1, col);
    spr.drawLine(x0, y0 + 1, x1, y1 + 1, col);
    drawTravel += fabsf(nx - penX) + fabsf(ny - penY);
    penX = nx;
    penY = ny;
  }
  spr.fillCircle((int)penX, (int)penY, 1, col);

  buddyTick(P_IDLE);
  spr.fillCircle(W - 11, 11, 5, col);            // which colour is loaded
  spr.drawCircle(W - 11, 11, 7, DIM);
}

// --- Marble -----------------------------------------------------------------
// Roll the ball onto three apples, one at a time.
const float   MARBLE_ACCEL = 1.2f, MARBLE_DRAG = 0.92f;
const uint8_t MARBLE_GOAL  = 3;
float   mbX = 0, mbY = 0, mbVX = 0, mbVY = 0;
int     mbAX = 0, mbAY = 0;
uint8_t mbGot = 0;

static void mbSpawn() {
  do {
    mbAX = 16 + esp_random() % (W - 32);
    mbAY = FIELD_TOP + 14 + esp_random() % (H - FIELD_TOP - 44);
  } while (fabsf(mbAX - mbX) + fabsf(mbAY - mbY) < 50);
}

static void startMarble(uint32_t now) {
  actBegin(M_MARBLE, true);
  mbX = CX;
  mbY = (FIELD_TOP + H) / 2;
  mbVX = mbVY = 0;
  mbGot = 0;
  mbSpawn();
  tiltBegin(now);
}

static void tickMarble(uint32_t now) {
  if (tiltUpdate(now)) {
    mbVX = (mbVX + TILT_SIGN * tiltX * MARBLE_ACCEL) * MARBLE_DRAG;
    mbVY = (mbVY + TILT_SIGN_Y * tiltY * MARBLE_ACCEL) * MARBLE_DRAG;
    mbX += mbVX;
    mbY += mbVY;
    const float L = 8, R = W - 9, T = FIELD_TOP + 8, B = H - 22;
    if (mbX < L) { mbX = L; mbVX = -mbVX * 0.4f; }
    if (mbX > R) { mbX = R; mbVX = -mbVX * 0.4f; }
    if (mbY < T) { mbY = T; mbVY = -mbVY * 0.4f; }
    if (mbY > B) { mbY = B; mbVY = -mbVY * 0.4f; }
  }
  if (fabsf(mbAX - mbX) < 12 && fabsf(mbAY - mbY) < 12) {
    mbGot++;
    beep(2000, 60);
    actUntil = now + 700;
    if (mbGot >= MARBLE_GOAL) { finishActivity(now); return; }
    mbSpawn();
  }

  buddyTick(actBeat(now) ? P_HEART : P_IDLE);
  spr.fillRect(0, FIELD_TOP, W, H - FIELD_TOP, BG);
  drawApple(mbAX, mbAY, 7);
  spr.fillCircle((int)mbX, (int)mbY, 7, AQUA);
  spr.fillCircle((int)mbX - 2, (int)mbY - 2, 2, INK);
  spr.drawCircle((int)mbX, (int)mbY, 7, INK);
  for (int i = 0; i < MARBLE_GOAL; i++)
    drawApple(CX - (MARBLE_GOAL - 1) * 8 + i * 16, 232, 3, i < mbGot);
}

// --- Copycat ----------------------------------------------------------------
// The pet lights the two buttons in a short pattern and the kid repeats it.
// The blue circle sits low in the middle like the blue button; the white bar
// hugs the edge the side button is on. A miss just replays the pattern.
const int     SIDE_BAR_X = W - 16;     // move to 6 if the side button is on the left
const uint8_t COPY_ROUNDS = 4;
enum { CC_SHOW, CC_INPUT, CC_GOOD, CC_BAD };
uint8_t  ccSeq[COPY_ROUNDS], ccRound = 0, ccIdx = 0, ccPhase = CC_SHOW, ccLit = 0;
uint32_t ccStepMs = 0, ccLitUntil = 0;

static void ccLight(uint8_t which, uint32_t now, uint16_t ms) {
  ccLit = which;
  ccLitUntil = now + ms;
  beep(which ? 660 : 880, ms > 60 ? ms - 40 : ms);
}

static void ccShow(uint32_t now) {
  ccPhase = CC_SHOW;
  ccIdx = 0;
  ccStepMs = now + 700;
}

static void ccNewRound(uint32_t now) {
  for (int i = 0; i <= ccRound; i++) ccSeq[i] = esp_random() & 1;
  ccShow(now);
}

static void startCopy(uint32_t now) {
  actBegin(M_COPY, true);
  ccRound = 0;
  ccLitUntil = 0;
  ccNewRound(now);
}

static void tickCopy(uint32_t now, bool a, bool b) {
  PetState st = P_IDLE;
  switch (ccPhase) {
    case CC_SHOW:
      if ((int32_t)(now - ccStepMs) >= 0) {
        if (ccIdx <= ccRound) { ccLight(ccSeq[ccIdx++], now, 450); ccStepMs = now + 700; }
        else                  { ccPhase = CC_INPUT; ccIdx = 0; }
      }
      break;
    case CC_INPUT:
      if (a || b) {
        uint8_t pressed = b ? 1 : 0;
        ccLight(pressed, now, 200);
        if (pressed == ccSeq[ccIdx]) {
          if (++ccIdx > ccRound) { ccPhase = CC_GOOD; actUntil = now + 900; }
        } else {
          ccPhase = CC_BAD;
          actUntil = now + 1200;
          beep(300, 200);
        }
      }
      break;
    case CC_GOOD:
      st = P_HEART;
      if (!actBeat(now)) {
        if (++ccRound >= COPY_ROUNDS) { finishActivity(now); return; }
        ccNewRound(now);
      }
      break;
    case CC_BAD:
      st = P_DIZZY;
      if (!actBeat(now)) ccShow(now);
      break;
  }

  buddyTick(st);
  spr.fillRect(0, FIELD_TOP, W, H - FIELD_TOP, BG);
  bool lit = (int32_t)(now - ccLitUntil) < 0;
  if (lit && ccLit == 0) spr.fillCircle(CX, 178, 22, BTN_BLUE);
  else { spr.drawCircle(CX, 178, 22, BTN_BLUE); spr.drawCircle(CX, 178, 21, BTN_BLUE); }
  if (lit && ccLit == 1) spr.fillRoundRect(SIDE_BAR_X, 98, 10, 46, 4, INK);
  else                   spr.drawRoundRect(SIDE_BAR_X, 98, 10, 46, 4, DIM);
  drawPips(COPY_ROUNDS, ccRound, 232);
}

// --- Hide and seek ----------------------------------------------------------
// The pet's eyes peek over one of three bushes, then duck. Side moves, blue
// looks. A wrong bush rustles and the eyes peek again.
const int     BUSH_X[3] = { 24, 67, 110 };
const int     HIDE_TOP  = 150;          // everything below the pet strip's ink
const uint8_t HIDE_FINDS = 3;
enum { HD_PEEK, HD_PICK, HD_MISS, HD_FOUND };
uint8_t hdSpot = 0, hdSel = 0, hdFound = 0, hdPhase = HD_PEEK, hdMissAt = 0;

static void hdHide(uint32_t now) {
  hdSpot = esp_random() % 3;
  hdPhase = HD_PEEK;
  actUntil = now + 1000;
  beep(1200, 40);
}

static void startHide(uint32_t now) {
  actBegin(M_HIDE, false);
  hdSel = 0;
  hdFound = 0;
  hdHide(now);
}

static void tickHide(uint32_t now, bool a, bool b) {
  switch (hdPhase) {
    case HD_PEEK:
      if (!actBeat(now)) hdPhase = HD_PICK;
      break;
    case HD_PICK:
      if (b) { hdSel = (hdSel + 1) % 3; beep(1800, 25); }
      if (a) {
        if (hdSel == hdSpot) {
          hdPhase = HD_FOUND;
          actUntil = now + 1800;
          beep(2000, 80);
          buddyInvalidate();
        } else {
          hdPhase = HD_MISS;
          hdMissAt = hdSel;
          actUntil = now + 500;
          beep(500, 60);
        }
      }
      break;
    case HD_MISS:
      if (!actBeat(now)) { hdPhase = HD_PEEK; actUntil = now + 700; }
      break;
    case HD_FOUND:
      if (!actBeat(now)) {
        if (++hdFound >= HIDE_FINDS) { finishActivity(now); return; }
        spr.fillRect(0, 0, W, HIDE_TOP, BG);       // the pet goes off to hide again
        hdHide(now);
      }
      break;
  }

  if (hdPhase == HD_FOUND) buddyTick(P_HEART);
  spr.fillRect(0, HIDE_TOP, W, H - HIDE_TOP, BG);
  if (hdPhase == HD_PEEK) drawText("o  o", BUSH_X[hdSpot], 153, 2, buddyInkColor());
  for (int i = 0; i < 3; i++) {
    int x = BUSH_X[i];
    if (hdPhase == HD_MISS && i == hdMissAt) x += ((now / 60) & 1) ? 2 : -2;
    drawBush(x, 186);
    if (i == hdSel && hdPhase != HD_FOUND) {
      spr.drawRoundRect(BUSH_X[i] - 21, HIDE_TOP + 1, 42, 52, 6, INK);
      spr.drawRoundRect(BUSH_X[i] - 20, HIDE_TOP + 2, 40, 50, 5, INK);
    }
  }
  spr.drawFastHLine(0, 204, W, GREEN);
  drawPips(HIDE_FINDS, hdFound, 232);
}

// --- Dress-up ---------------------------------------------------------------
// Side changes the colour, blue changes the hat. Saved as it changes.
static void startDress() { actBegin(M_DRESS, false); }

static void tickDress(bool a, bool b) {
  PetSave& s = pet();
  if (b) { s.color = (s.color + 1) % LOOK_N; beep(1800, 25); }
  if (a) { s.hat = (s.hat + 1) % HAT_N; beep(2400, 25); }
  if (a || b) { applyLook(); petSave(); }

  buddyTick(P_IDLE);
  spr.fillRect(0, BOTTOM_TOP, W, H - BOTTOM_TOP, BG);
  for (int i = 0; i < LOOK_N; i++) {
    int x = 15 + i * 21;
    spr.fillCircle(x, 180, 7, i ? LOOK_TINT[i] : buddyBodyColor());
    if (i == s.color) { spr.drawCircle(x, 180, 10, INK); spr.drawCircle(x, 180, 9, INK); }
  }
  spr.drawFastHLine(0, 197, W, GRID);
  for (int i = 0; i < HAT_N; i++) {
    int x = 16 + i * 26;
    if (i == HAT_NONE) spr.drawCircle(x, 219, 6, DIM);
    else               buddyDrawHat(&spr, i, x, 226, 1);
    if (i == s.hat) spr.drawRoundRect(x - 12, 203, 25, 32, 5, INK);
  }
}

// --- Stickers ---------------------------------------------------------------
static void startStickers() { actBegin(M_STICKERS, false); }

static void tickStickers() {
  spr.fillSprite(BG);
  for (int i = 0; i < STICKER_N; i++) {
    int x = 24 + (i % 3) * 43 + (i % 3) / 2, y = 36 + (i / 3) * 54;
    if (pet().stickers & (1u << i)) drawSticker(i, x, y);
    else                            spr.drawCircle(x, y, 13, GRID);
  }
}
