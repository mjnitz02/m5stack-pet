#pragma once

// The toy box activities. Header-only with file-static state: include from
// main.cpp only, after home.h, which has finishActivity().
//
// Rules they all keep: no words, no score, no way to lose. Each ends by
// calling finishActivity(), which adds a heart and sometimes a sticker.
// The ones on the first page of the toy box are the ones that get a kid
// moving; the numbers that decide "enough movement" are guesses until they
// have been tried in small hands, so they sit at the top of each section.

const int FIELD_TOP = 86;  // the games' field starts under the small pet

uint32_t actUntil = 0;     // one shared timer for the short cheer/dizzy beats
static bool actBeat(uint32_t now) { return before(now, actUntil); }

static void actBegin(Mode m, bool peek) {
  mode = m;
  clearScreen(peek);
  actUntil = 0;
  lastInteractMs = millis();
}

// "Any movement adds up": what this frame's motion sample is worth to the
// activities that fill something by moving. Nothing for a still hand, and a
// hard shake counts no more than a lively one.
const float MOVE_FLOOR = 0.06f;   // ignore hand tremor below this
static float movedBy() {
  return imuFresh && imuDelta > MOVE_FLOOR ? fminf(imuDelta, 1.0f) - MOVE_FLOOR : 0;
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
// adds up, and enough of it lights the next stone on the path. Standing still
// just pauses. Aim: about a minute of real moving.
//
// A little walker slides along the path with every bit of movement and hops
// while the stick is moving, leaving a coloured trail, so the screen answers
// from the first step and not only when a stone lights. Each walk picks one
// of a few routes, never the one just walked.
const float   WALK_PER_DOT = 18.0f;
const uint8_t WALK_DOTS    = 10;
const int     WALK_STONE_R = 5;

enum { WG_FLOWER, WG_STAR, WG_APPLE, WG_BALL };
struct WalkRoute {
  int16_t  pt[WALK_DOTS + 1][2];      // where the walker starts, then the stones
  int16_t  gx, gy;                    // what it is walking to
  uint8_t  goal;
  uint16_t col;                       // lit stones and the trail
};
static const WalkRoute WALK_ROUTES[] = {
  // Winding up from the bottom left to a flower.
  { { { 14, 232 }, { 32, 226 }, { 52, 222 }, { 72, 216 }, { 90, 206 }, { 100, 190 },
      { 88, 174 }, { 68, 166 }, { 48, 158 }, { 34, 144 }, { 40, 126 } },
    62, 106, WG_FLOWER, ROSE },
  // Stairs from the bottom right up to a star.
  { { { 124, 230 }, { 104, 230 }, { 104, 212 }, { 84, 212 }, { 84, 194 }, { 64, 194 },
      { 64, 176 }, { 44, 176 }, { 44, 158 }, { 24, 158 }, { 24, 140 } },
    24, 114, WG_STAR, YEL },
  // Round the edge and curling in to an apple.
  { { { 8, 222 }, { 28, 228 }, { 50, 230 }, { 72, 230 }, { 94, 226 }, { 112, 212 },
      { 116, 190 }, { 108, 170 }, { 90, 158 }, { 70, 160 }, { 58, 176 } },
    68, 198, WG_APPLE, AMBER },
  // Up and over a hill to a ball.
  { { { 14, 226 }, { 17, 203 }, { 22, 184 }, { 31, 168 }, { 44, 156 }, { 59, 151 },
      { 75, 151 }, { 89, 157 }, { 102, 170 }, { 111, 186 }, { 116, 205 } },
    116, 227, WG_BALL, AQUA },
};
const uint8_t WALK_ROUTE_N = sizeof(WALK_ROUTES) / sizeof(WALK_ROUTES[0]);

float    walkAcc = 0;
uint8_t  walkDots = 0;
uint8_t  walkRoute = 0;
uint32_t walkMovedMs = 0;

static void startWalk() {
  actBegin(M_WALK, true);
  walkAcc = 0;
  walkDots = 0;
  walkRoute = (walkRoute + 1 + esp_random() % (WALK_ROUTE_N - 1)) % WALK_ROUTE_N;   // any route but the last
  walkMovedMs = millis() - 1000;
}

static void tickWalk(uint32_t now) {
  float moved = movedBy();
  walkAcc += moved;
  if (moved > 0) walkMovedMs = now;
  if (walkAcc >= WALK_PER_DOT) {
    walkAcc = 0;
    walkDots++;
    beep(1100 + walkDots * 90, 50);
    actUntil = now + 700;
    if (walkDots >= WALK_DOTS) { finishActivity(now); return; }
  }
  bool moving = now - walkMovedMs < 400;

  buddyTick(actBeat(now) ? P_HEART : P_IDLE);
  clearBelow(FIELD_TOP);
  const WalkRoute& r = WALK_ROUTES[walkRoute];

  // The walker is part-way from the last lit stone to the next one.
  float f = walkAcc / WALK_PER_DOT;
  int wx = r.pt[walkDots][0] + (int)((r.pt[walkDots + 1][0] - r.pt[walkDots][0]) * f);
  int wy = r.pt[walkDots][1] + (int)((r.pt[walkDots + 1][1] - r.pt[walkDots][1]) * f);

  for (int i = walkDots; i < WALK_DOTS; i++)          // the way still to go
    spr.drawLine(r.pt[i][0], r.pt[i][1], r.pt[i + 1][0], r.pt[i + 1][1], GRID);
  for (int i = 0; i < walkDots; i++)                  // the way walked
    drawThickLine(r.pt[i][0], r.pt[i][1], r.pt[i + 1][0], r.pt[i + 1][1], r.col);
  drawThickLine(r.pt[walkDots][0], r.pt[walkDots][1], wx, wy, r.col);

  for (int i = 0; i < WALK_DOTS; i++) {
    int x = r.pt[i + 1][0], y = r.pt[i + 1][1];
    if (i < walkDots) { spr.fillCircle(x, y, WALK_STONE_R, r.col); continue; }
    spr.fillCircle(x, y, WALK_STONE_R, BG);
    spr.drawCircle(x, y, WALK_STONE_R, DIM);
  }

  int big = moving ? (int)((now / 300) & 1) : 0;      // the goal twinkles while they move
  switch (r.goal) {
    case WG_FLOWER:
      spr.drawLine(r.gx, r.gy + 6, r.gx, r.gy + 18, GREEN);
      drawFlower(r.gx, r.gy, 6 + big);
      break;
    case WG_STAR:  drawStar(r.gx, r.gy, 10 + big * 2, YEL); break;
    case WG_APPLE: drawApple(r.gx, r.gy, 8 + big); break;
    default:       drawBall(r.gx, r.gy, 8 + big); break;
  }

  int hop = moving ? (int)((now / 120) & 1) * 3 : 0;
  spr.fillCircle(wx, wy - hop, 6, INK);
  spr.fillRect(wx - 3, wy - hop - 2, 2, 2, BG);
  spr.fillRect(wx + 2, wy - hop - 2, 2, 2, BG);
}

// --- Dance party ------------------------------------------------------------
// Walk in party clothes, opened from the middle icon on the pet screen. The
// same "any movement adds up", shown as five rainbow bars that fill one after
// another while the pet parties. Aim: about half a minute of dancing.
const float   DANCE_PER_BAR = 20.7f;
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
  danceAcc += movedBy();
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

  clearBelow(BOTTOM_TOP);
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
// Shake the bubbles off. Like the walk, any movement adds up; the next bubble
// swells as it does and pops when there has been enough. Aim: about half a
// dance party.
const float   BATH_PER_BUBBLE = 6.0f;
const uint8_t BATH_N = 8;
static const int16_t BUBBLE[BATH_N][3] = {
  { 34, 70, 9 }, { 98, 66, 11 }, { 60, 58, 6 }, { 84, 100, 8 },
  { 42, 108, 10 }, { 106, 116, 7 }, { 66, 122, 9 }, { 24, 92, 6 }
};
uint8_t  bathLeft = 0;
bool     bathDone = false;
float    bathAcc = 0;

static void startBath() {
  actBegin(M_BATH, false);
  bathLeft = BATH_N;
  bathDone = false;
  bathAcc = 0;
}

static void tickBath(uint32_t now) {
  if (bathDone) {
    if (!actBeat(now)) { finishActivity(now); return; }
    buddyTick(P_HEART);
    bool tw = (now / 200) & 1;
    drawStar(20, 60, tw ? 6 : 4, YEL);
    drawStar(114, 52, tw ? 4 : 6, YEL);
    drawStar(18, 130, tw ? 4 : 5, YEL);
    drawStar(116, 128, tw ? 5 : 4, YEL);
    clearBelow(BOTTOM_TOP);
    return;
  }
  float moved = movedBy();
  bathAcc += moved;
  if (moved > 0) buddyInvalidate();  // wipe the last size of the swelling bubble
  if (bathAcc >= BATH_PER_BUBBLE) {
    bathAcc = 0;
    bathLeft--;
    beep(1400 + (BATH_N - bathLeft) * 100, 40);
    if (!bathLeft) { bathDone = true; actUntil = now + 1800; }
  }

  buddyTick(P_IDLE);
  for (int i = 0; i < bathLeft; i++) {
    int swell = i == bathLeft - 1 ? (int)(bathAcc * 5 / BATH_PER_BUBBLE) : 0;
    drawBubble(BUBBLE[i][0], BUBBLE[i][1], BUBBLE[i][2] + swell);
  }
  clearBelow(BOTTOM_TOP);
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
  clearBelow(BOTTOM_TOP);
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
  bool over = !before(now, fzUntil);
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
      if (!before(now, fzGraceUntil) && imuFresh && imuDelta > FREEZE_MOVE) {
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
  clearBelow(BOTTOM_TOP);
  drawEye(CX, 182, fzPhase == FZ_WATCH);
  for (int i = 0; i < FREEZE_STARS; i++)
    drawStar(CX - (FREEZE_STARS - 1) * 11 + i * 22, 222, 7, i < fzStars ? YEL : DIM, i < fzStars);
}

// --- Catch ------------------------------------------------------------------
// Snacks fall one at a time; tilt the stick left/right to slide the basket
// under them. No way to lose — every round ends in a celebration.
const uint8_t PLAY_ROUNDS = 5;
const int     BASKET_Y    = 214, CATCH_HALF = 30;
const float   FALL_PX_MS  = 0.055f;
uint8_t  playRound = 0, playCaught = 0, playHits = 0, dropFood = 0;
float    basketX = CX;
int      dropX = CX;
uint32_t dropStartMs = 0, playCheerUntil = 0;

static void spawnDrop(uint32_t now) {
  dropX = 16 + esp_random() % (W - 32);
  dropFood = esp_random() % FOOD_N;
  dropStartMs = now;
}

static void startPlay(uint32_t now) {
  mode = M_PLAY;
  playRound = playCaught = playHits = 0;
  basketX = CX;
  clearScreen(true);
  spawnDrop(now);
  tiltBegin(now);
}

static void endPlay(uint32_t now) {
  finishActivity(now, playCaught >= 3 ? 2 : 1);
}

static void tickPlay(uint32_t now) {
  lastInteractMs = now;

  // Nothing falls until the tilt centre has been captured.
  if (!tiltUpdate(now)) dropStartMs = now;
  float target = CX + tiltX * 110.0f;
  if (target < 30) target = 30;
  if (target > W - 30) target = W - 30;
  basketX += (target - basketX) * 0.3f;

  int dropY = FIELD_TOP + 8 + (int)((now - dropStartMs) * FALL_PX_MS);
  if (dropY >= BASKET_Y - 4) {
    bool hit = fabsf(dropX - basketX) < CATCH_HALF;
    if (hit) { playCaught++; playHits |= 1 << playRound; playCheerUntil = now + 700; beep(2000, 60); }
    else     { beep(500, 60); }
    if (++playRound >= PLAY_ROUNDS) { endPlay(now); return; }
    spawnDrop(now);
    dropY = FIELD_TOP + 8;
  }

  buddyTick(before(now, playCheerUntil) ? P_HEART : P_IDLE);

  clearBelow(FIELD_TOP);
  drawFood(dropFood, dropX, dropY);
  drawText("\\___/", (int)basketX, BASKET_Y, 2, INK);
  for (int i = 0; i < PLAY_ROUNDS; i++) {
    int x = CX - (PLAY_ROUNDS - 1) * 8 + i * 16;
    if (i < playRound) drawApple(x, 234, 3, playHits & (1 << i));
    else               spr.drawPixel(x, 234, DIM);
  }
}

// --- Draw -------------------------------------------------------------------
// Tilt moves the pen and the line drifts through the rainbow as it goes.
// Blue starts a fresh page with the pen back in the middle; a shake wipes
// the page where the pen is. The sprite keeps whatever was drawn, so the
// field is never cleared per frame.
const float PEN_SPEED  = 9.0f;         // px per frame at a full tilt
const float PEN_DEAD   = 0.04f;
const float PEN_RAINBOW_PX = 700.0f;   // pen travel for one trip round the colours
float penX = 0, penY = 0, drawTravel = 0;

static float penDead(float v) {
  if (fabsf(v) < PEN_DEAD) return 0;
  return v > 0 ? v - PEN_DEAD : v + PEN_DEAD;
}

// Full-brightness colour wheel, h in 0..1.
static uint16_t rainbow(float h) {
  h = (h - floorf(h)) * 6;
  int seg = (int)h;
  uint8_t up = (uint8_t)((h - seg) * 255), down = 255 - up, r = 0, g = 0, b = 0;
  switch (seg) {
    case 0:  r = 255;  g = up;                break;
    case 1:  r = down; g = 255;               break;
    case 2:            g = 255;  b = up;      break;
    case 3:            g = down; b = 255;     break;
    case 4:  r = up;             b = 255;     break;
    default: r = 255;            b = down;    break;
  }
  return spr.color565(r, g, b);
}

static void drawFresh() {
  clearBelow(FIELD_TOP);
  penX = CX;
  penY = (FIELD_TOP + H) / 2;
}

static void startDraw(uint32_t now) {
  actBegin(M_DRAW, true);
  drawFresh();
  drawTravel = 0;
  tiltBegin(now);
}

static void tickDraw(uint32_t now, bool a, bool shook) {
  if (a) { drawFresh(); beep(2400, 25); }
  if (shook) {
    clearBelow(FIELD_TOP);
    beep(700, 60);
  }
  uint16_t col = rainbow(drawTravel / PEN_RAINBOW_PX);
  if (tiltUpdate(now)) {
    float nx = penX + penDead(tiltX) * PEN_SPEED;
    float ny = penY + penDead(tiltY) * PEN_SPEED;
    if (nx < 2) nx = 2;
    if (nx > W - 3) nx = W - 3;
    if (ny < FIELD_TOP + 2) ny = FIELD_TOP + 2;
    if (ny > H - 3) ny = H - 3;
    drawThickLine((int)penX, (int)penY, (int)nx, (int)ny, col);
    drawTravel += fabsf(nx - penX) + fabsf(ny - penY);
    penX = nx;
    penY = ny;
  }
  spr.fillCircle((int)penX, (int)penY, 1, col);

  buddyTick(P_IDLE);
  spr.fillCircle(W - 11, 11, 5, col);            // the colour the pen is on now
  spr.drawCircle(W - 11, 11, 7, DIM);
}

// --- Marble -----------------------------------------------------------------
// Roll the ball onto three apples, one at a time. Blue starts the game over.
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

static void mbFresh() {
  mbX = CX;
  mbY = (FIELD_TOP + H) / 2;
  mbVX = mbVY = 0;
  mbGot = 0;
  mbSpawn();
}

static void startMarble(uint32_t now) {
  actBegin(M_MARBLE, true);
  mbFresh();
  tiltBegin(now);
}

static void tickMarble(uint32_t now, bool a) {
  if (a) { mbFresh(); beep(2400, 25); }
  if (tiltUpdate(now)) {
    mbVX = (mbVX + tiltX * MARBLE_ACCEL) * MARBLE_DRAG;
    mbVY = (mbVY + tiltY * MARBLE_ACCEL) * MARBLE_DRAG;
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
  clearBelow(FIELD_TOP);
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

// The two button pictures. A cue is the "press this one" pulse the lesson
// uses: the egg's throbbing blue dot, or an arrow nudging at the white bar.
static void ccDraw(uint32_t now, bool cueBlue = false, bool cueSide = false) {
  bool lit = before(now, ccLitUntil), pulse = (now / 400) & 1;
  if (lit && ccLit == 0) spr.fillCircle(CX, 178, 22, BTN_BLUE);
  else {
    spr.drawCircle(CX, 178, 22, BTN_BLUE);
    spr.drawCircle(CX, 178, 21, BTN_BLUE);
    if (cueBlue) spr.fillCircle(CX, 178, 10 + pulse * 3, BTN_BLUE);
  }
  if (lit && ccLit == 1) spr.fillRoundRect(SIDE_BAR_X, 98, 10, 46, 4, INK);
  else                   spr.drawRoundRect(SIDE_BAR_X, 98, 10, 46, 4, cueSide ? INK : DIM);
  if (cueSide) {
    // Points away from the middle of the screen, at whichever edge the bar is on.
    int tip = SIDE_BAR_X > CX ? SIDE_BAR_X - 4 + pulse * 3 : SIDE_BAR_X + 14 - pulse * 3;
    int back = SIDE_BAR_X > CX ? tip - 7 : tip + 7;
    spr.fillTriangle(tip, 121, back, 114, back, 128, INK);
  }
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
      if (!before(now, ccStepMs)) {
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
  clearBelow(FIELD_TOP);
  ccDraw(now);
  drawPips(COPY_ROUNDS, ccRound, 232);
}

// --- Lesson -----------------------------------------------------------------
// Not a toy box activity: it plays before the first egg, and again from the
// parent screen. It teaches what the two buttons do with Copycat's pictures
// of them, in four steps: blue picks, side moves along, the two together,
// and holding blue opens and shuts. The screen does each one once, then
// waits for it to be copied a few times. A press lights the picture of the
// button that was pressed whether or not it was the one wanted, and nothing
// can be got wrong: the step just waits.
enum { LS_TAP, LS_NEXT, LS_BOTH, LS_HOLD, LS_N };
enum { LP_SHOW, LP_SHOWN, LP_INPUT, LP_CHEER, LP_PARTY };
static const uint8_t LS_REPS[LS_N] = { 3, 4, 3, 2 };
const int LS_MID_Y = 112;            // middle of the box, or of the four tiles
uint8_t  lsStep = 0, lsPhase = LP_SHOW, lsDone = 0, lsSel = 0, lsTarget = 0;
bool     lsOpen = false;
uint32_t lsUntil = 0, lsHoldMs = 0, lsPopUntil = 0;
int      lsPopX = 0, lsPopY = 0;

static int lsTileX(uint8_t i) { return CX - 17 + (i % 2) * 34; }
static int lsTileY(uint8_t i) { return LS_MID_Y - 17 + (i / 2) * 34; }

static void lsPop(int x, int y, uint32_t now) {
  lsPopX = x;
  lsPopY = y;
  lsPopUntil = now + 400;
  beep(1568, 80);
}

static void lsBeginStep(uint32_t now) {
  lsDone = 0;
  lsSel = 0;
  lsOpen = false;
  lsHoldMs = 0;
  lsTarget = 1 + esp_random() % 3;
  // The third step is the first two put together, so it is not shown again.
  lsPhase = lsStep == LS_BOTH ? LP_INPUT : LP_SHOW;
  lsUntil = now + 900;
}

static void startLearn(uint32_t now) {
  mode = M_LEARN;
  buddySetPeek(false);
  applyBrightness();
  lsStep = 0;
  ccLitUntil = 0;
  lsPopUntil = 0;
  lsBeginStep(now);
  lastInteractMs = now;
}

// What a tap of blue, a tap of the side button or a finished hold does in
// this step. True when it was the thing the step is teaching.
static bool lsAct(bool a, bool b, bool held, uint32_t now) {
  switch (lsStep) {
    case LS_TAP:
      if (a) { lsPop(CX, LS_MID_Y, now); return true; }
      break;
    case LS_NEXT:
      if (b) { lsSel = (lsSel + 1) % 4; return true; }
      break;
    case LS_BOTH:
      if (b) lsSel = (lsSel + 1) % 4;
      if (a && lsSel == lsTarget) {
        lsPop(lsTileX(lsSel), lsTileY(lsSel), now);
        lsTarget = (lsTarget + 1 + esp_random() % 3) % 4;   // any tile but this one
        return true;
      }
      break;
    case LS_HOLD:
      if (held) { lsOpen = !lsOpen; lsPop(CX, LS_MID_Y, now); return true; }
      break;
  }
  return false;
}

static void lsIcon(uint8_t i, int x, int y) {
  switch (i % 4) {
    case 0: drawApple(x, y + 1, 7); break;
    case 1: drawHeart(x, y - 4, 2, true, RED); break;
    case 2: drawBall(x, y, 7); break;
    case 3: drawFlower(x, y, 5); break;
  }
}

static void tickLearn(uint32_t now, bool a, bool b) {
  uint32_t held = 0;                 // how long blue has been held, in the last step

  switch (lsPhase) {
    case LP_SHOW:                    // the screen presses the button itself
      if (before(now, lsUntil)) break;
      if (lsStep == LS_HOLD) {
        held = now - lsUntil;
        ccLit = 0;
        ccLitUntil = now + 100;
        if (held < HOLD_BLUE_MS) break;
      } else {
        ccLight(lsStep == LS_NEXT ? 1 : 0, now, 450);
      }
      lsAct(lsStep == LS_TAP, lsStep == LS_NEXT, lsStep == LS_HOLD, now);
      lsPhase = LP_SHOWN;
      lsUntil = now + 1100;
      break;
    case LP_SHOWN:
      if (before(now, lsUntil)) break;
      lsSel = 0;
      lsOpen = false;
      lsPhase = LP_INPUT;
      break;
    case LP_INPUT: {
      // The picture lights as the button goes down; the action waits for the
      // release like everywhere else.
      if (M5.BtnA.wasPressed()) ccLight(0, now, 200);
      if (M5.BtnB.wasPressed()) ccLight(1, now, 200);
      bool full = false;
      if (lsStep == LS_HOLD && !swallowA && M5.BtnA.isPressed() && !M5.BtnB.isPressed()) {
        if (!lsHoldMs) lsHoldMs = now;
        held = now - lsHoldMs;
        ccLit = 0;
        ccLitUntil = now + 100;
        if (held >= HOLD_BLUE_MS) { full = true; swallowA = true; lsHoldMs = 0; held = 0; }
      } else {
        lsHoldMs = 0;
      }
      if (lsAct(a, b, full, now) && ++lsDone >= LS_REPS[lsStep]) {
        lsPhase = LP_CHEER;
        lsUntil = now + 1100;
      }
      break;
    }
    case LP_CHEER:
      lastInteractMs = now;
      if (before(now, lsUntil)) break;
      if (++lsStep < LS_N) { lsBeginStep(now); break; }
      spr.fillSprite(BG);
      spr.pushSprite(0, 0);
      {
        static const uint16_t TUNE[] = { 1047, 1319, 1568, 2093 };
        jingle(TUNE, 4, 90);
      }
      lsPhase = LP_PARTY;
      lsUntil = millis() + 2500;
      return;
    case LP_PARTY: {
      lastInteractMs = now;
      if (!before(now, lsUntil)) {
        pet().learned = true;
        petSave();
        if (pet().hatched) enterHome();
        else               enterEgg();
        return;
      }
      // Stars all over, each one staying a couple of seconds, colours chasing.
      spr.fillSprite(BG);
      uint32_t beat = now / 200;
      for (uint32_t i = beat; i < beat + 10; i++) {
        uint32_t h = i * 2654435761u;
        drawStar(12 + (h >> 8) % (W - 24), 12 + (h >> 18) % (H - 24), (i & 1) ? 6 : 4, DANCE_COLORS[i % DANCE_BARS]);
      }
      return;
    }
  }

  bool popping = before(now, lsPopUntil), cheer = lsPhase == LP_CHEER;
  spr.fillSprite(BG);

  // The egg watches from the top and jiggles when something goes right.
  drawEggShape(CX + ((popping || cheer) ? (((now / 60) & 1) ? 3 : -3) : 0), 40, 20, 26);
  if (cheer) {
    bool tw = (now / 200) & 1;
    drawStar(24, 40, tw ? 6 : 4, YEL);
    drawStar(W - 24, 40, tw ? 4 : 6, YEL);
  }

  if (lsStep == LS_TAP || (lsStep == LS_HOLD && !lsOpen)) {
    // One thing in a box: something to pick, or something to open.
    drawFrame(CX - 22, LS_MID_Y - 22, 44, 44, 8);
    if (lsStep == LS_TAP) lsIcon(lsDone, CX, LS_MID_Y);
    else                  drawStar(CX, LS_MID_Y, 10, YEL);
  } else {
    // Four tiles, the toy box in small. The frame goes right, then down.
    for (uint8_t i = 0; i < 4; i++) {
      int x = lsTileX(i), y = lsTileY(i);
      if (i == lsSel) drawFrame(x - 15, y - 15, 30, 30, 6);
      else            spr.drawRoundRect(x - 15, y - 15, 30, 30, 6, GRID);
      if (lsStep != LS_BOTH)                lsIcon(i, x, y);
      else if (i == lsTarget && !cheer)     drawStar(x, y, ((now / 300) & 1) ? 8 : 6, YEL);
    }
  }
  if (popping) {
    int r = 30 - (int)(lsPopUntil - now) / 25;         // four stars flying outward
    for (int i = 0; i < 4; i++)
      drawStar(lsPopX + ((i & 1) ? r : -r), lsPopY + ((i & 2) ? r : -r), 4, YEL);
  }

  // Which button to press now pulses, but only once it is the kid's turn.
  bool turn = lsPhase == LP_INPUT, onStar = lsSel == lsTarget;
  ccDraw(now,
         turn && (lsStep == LS_TAP || lsStep == LS_HOLD || (lsStep == LS_BOTH && onStar)),
         turn && (lsStep == LS_NEXT || (lsStep == LS_BOTH && !onStar)));
  if (lsStep == LS_HOLD) {
    // A ring round the blue circle that fills while it is held.
    int lit = held * 24 / HOLD_BLUE_MS;
    for (int i = 0; i < 24; i++) {
      float ang = -1.5708f + i * 0.2618f;
      spr.fillCircle(CX + (int)(cosf(ang) * 29), 178 + (int)(sinf(ang) * 29), i < lit ? 3 : 1, i < lit ? AQUA : GRID);
    }
  }
  drawPips(LS_REPS[lsStep], lsDone, 232);
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
      drawFrame(BUSH_X[i] - 21, HIDE_TOP + 1, 42, 52, 6);
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
  clearBelow(BOTTOM_TOP);
  for (int i = 0; i < LOOK_N; i++) {
    int x = 15 + i * 21;
    spr.fillCircle(x, 180, 7, i ? LOOK_TINT[i] : buddyBodyColor());
    if (i == s.color) { spr.drawCircle(x, 180, 10, INK); spr.drawCircle(x, 180, 9, INK); }
  }
  spr.drawFastHLine(0, 197, W, GRID);
  for (int i = 0; i < HAT_N; i++) {
    int x = 16 + i * 26;
    if (i == HAT_NONE) spr.drawCircle(x, 219, 6, DIM);
    else               buddyDrawHat(i, x, 226, 1);
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
