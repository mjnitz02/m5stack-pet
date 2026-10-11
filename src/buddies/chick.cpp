#include "../buddy.h"
#include "../buddy_common.h"
#include "../compat.h"
#include <string.h>

extern TFT_eSprite spr;

namespace chick {

// ─── SLEEP ───  ~12s cycle, sat down with its feet tucked under, rolling a little
static void doSleep(uint32_t t) {
  static const char* const TUCK[5]    = { "           ", "    ,,     ", "  <(- )__  ", "   (  ._>  ", "    `--'   " };
  static const char* const BREATHE[5] = { "           ", "    ,,     ", "  <(- )__  ", "   (  .->  ", "    `--'   " };
  static const char* const SNORE[5]   = { "           ", "    ,,     ", "  <(u )__  ", "   (  .->  ", "    `--'   " };
  static const char* const DRIFT_L[5] = { "           ", "   ,,      ", " <(- )__   ", "  (  ._>   ", "   `--'    " };
  static const char* const DRIFT_R[5] = { "           ", "     ,,    ", "   <(- )__ ", "    (  ._> ", "     `--'  " };
  static const char* const DREAM[5]   = { "           ", "    ,,     ", "  <(uu)__  ", "   (  ._>  ", "    `--'   " };

  const char* const* P[6] = { TUCK, BREATHE, SNORE, DRIFT_L, DRIFT_R, DREAM };
  static const uint8_t SEQ[] = {
    0,0,1,0,1,2,1,
    0,1,0,1,
    3,3,4,4,3,4,
    0,0,
    1,5,1,1
  };
  uint8_t beat = (t / 5) % sizeof(SEQ);
  buddyPrintSprite(P[SEQ[beat]], 5, 0, 0xFFE0);

  // Z particles drift up-right
  int p1 = (t)     % 10;
  int p2 = (t + 4) % 10;
  int p3 = (t + 7) % 10;
  buddySetColor(BUDDY_DIM);
  buddySetCursor(BUDDY_X_CENTER + 18 + p1, BUDDY_Y_OVERLAY + 18 - p1 * 2);
  buddyPrint("z");
  buddySetColor(BUDDY_WHITE);
  buddySetCursor(BUDDY_X_CENTER + 24 + p2, BUDDY_Y_OVERLAY + 14 - p2);
  buddyPrint("Z");
  buddySetColor(BUDDY_DIM);
  buddySetCursor(BUDDY_X_CENTER + 14 + p3 / 2, BUDDY_Y_OVERLAY + 10 - p3 / 2);
  buddyPrint("z");
}

// ─── IDLE ───  ~14s cycle, 10 micro-actions
static void doIdle(uint32_t t) {
  static const char* const REST[5]    = { "    ,,     ", "  <(o )__  ", "   (  ._>  ", "    `--'   ", "     ^^    " };
  static const char* const LOOK_L[5]  = { "    ,,     ", " <<(o )__  ", "   (  ._>  ", "    `--'   ", "     ^^    " };
  static const char* const LOOK_R[5]  = { "    ,,     ", "  <( o)__  ", "   (  ._>  ", "    `--'   ", "     ^^    " };
  static const char* const LOOK_U[5]  = { "    ''     ", "  <(^ )__  ", "   (  ._>  ", "    `--'   ", "     ^^    " };
  static const char* const BLINK[5]   = { "    ,,     ", "  <(- )__  ", "   (  ._>  ", "    `--'   ", "     ^^    " };
  static const char* const PEEP[5]    = { "    ,,     ", " <O(o )__  ", "   (  ._>  ", "    `--'   ", "     ^^    " };
  static const char* const PREEN_A[5] = { "    ,,     ", "  <(o )__  ", "   ( v.->  ", "    `--'   ", "     ^^    " };
  static const char* const PREEN_B[5] = { "    ,,     ", "  <(o )__  ", "   ( ^.->  ", "    `--'   ", "     ^^    " };
  static const char* const WAG_L[5]   = { "    ,,     ", "  <(o )__  ", "   (  ._<  ", "    `--'   ", "     ^^    " };
  static const char* const SHAKE[5]   = { "    ,,     ", "  <(o )__  ", "  ~(  ._>~ ", "   ~`--'~  ", "     ^^    " };

  const char* const* P[10] = { REST, LOOK_L, LOOK_R, LOOK_U, BLINK, PEEP, PREEN_A, PREEN_B, WAG_L, SHAKE };
  static const uint8_t SEQ[] = {
    0,0,0,1,0,2,0,4,
    0,5,0,0,
    6,7,6,7,
    0,0,3,3,0,4,
    8,0,8,0,
    9,9,0,0
  };
  uint8_t beat = (t / 5) % sizeof(SEQ);
  buddyPrintSprite(P[SEQ[beat]], 5, 0, 0xFFE0);
}

// ─── BUSY ───  ~10s cycle, walking, pecking at a seed + dot ticker
static void doBusy(uint32_t t) {
  static const char* const STEP_A[5] = { "    ,,     ", "  <(o )__  ", "   (  ._>  ", "    `--'   ", "    ^ ^    " };
  static const char* const STEP_B[5] = { "    ,,     ", "  <(o )__  ", "   (  ._>  ", "    `--'   ", "     ^^    " };
  static const char* const PECK_A[5] = { "    ,,     ", "  <(v )__  ", "   (  ._>  ", "    `--'   ", " .   ^^    " };
  static const char* const PECK_B[5] = { "   ,,      ", " <(v )___  ", "   (  ._>  ", "    `--'   ", " .   ^^    " };
  static const char* const GULP[5]   = { "    ,,     ", "  <(O )__  ", "  *(  ._>* ", "    `--'   ", "     ^^    " };
  static const char* const THINK[5]  = { "    ,,  ?  ", "  <(o )__  ", "   (  ._>  ", "    `--'   ", "     ^^    " };

  const char* const* P[6] = { STEP_A, STEP_B, PECK_A, PECK_B, GULP, THINK };
  static const uint8_t SEQ[] = {
    0,1,0,1,0,1, 5,5, 0,1,0,1, 2,3,3,2, 4,4, 0,1,0,1,5
  };
  uint8_t beat = (t / 5) % sizeof(SEQ);
  buddyPrintSprite(P[SEQ[beat]], 5, 0, 0xFFE0);

  static const char* const DOTS[] = { ".  ", ".. ", "...", " ..", "  .", "   " };
  buddySetColor(BUDDY_WHITE);
  buddySetCursor(BUDDY_X_CENTER + 22, BUDDY_Y_OVERLAY + 14);
  buddyPrint(DOTS[t % 6]);
}

// ─── ATTENTION ───  ~8s cycle, tuft straight up + ! pulse
static void doAttention(uint32_t t) {
  static const char* const ALERT[5]  = { "    ||     ", "  <(O )__  ", "   (  ._>  ", "    `--'   ", "     ^^    " };
  static const char* const SCAN_L[5] = { "    ||     ", " <<(O )__  ", "   (  ._>  ", "    `--'   ", "     ^^    " };
  static const char* const SCAN_R[5] = { "    ||     ", "  <( O)__  ", "   (  ._>  ", "    `--'   ", "     ^^    " };
  static const char* const TALL[5]   = { "    ||     ", "  <(O )__  ", "   (  ._>  ", "    `--'   ", "    ^  ^   " };
  static const char* const TENSE[5]  = { "    ||     ", "  <(O )__  ", "  /(  ._>\\ ", "    `--'   ", "     ^^    " };
  static const char* const CHEEP[5]  = { "    ||     ", " <O(O )__  ", "   (  ._>  ", "    `--'   ", "     ^^    " };

  const char* const* P[6] = { ALERT, SCAN_L, SCAN_R, TALL, TENSE, CHEEP };
  static const uint8_t SEQ[] = {
    0,5,0,1,0,2,0,3, 4,4,0,1,2,0, 5,0
  };
  uint8_t beat = (t / 5) % sizeof(SEQ);
  uint8_t pose = SEQ[beat];
  int xOff = (pose == 4) ? ((t & 1) ? 1 : -1) : 0;
  buddyPrintSprite(P[pose], 5, 0, 0xFFE0, xOff);

  if ((t / 2) & 1) {
    buddySetColor(BUDDY_YEL);
    buddySetCursor(BUDDY_X_CENTER - 4, BUDDY_Y_OVERLAY);
    buddyPrint("!");
  }
  if ((t / 3) & 1) {
    buddySetColor(BUDDY_YEL);
    buddySetCursor(BUDDY_X_CENTER + 4, BUDDY_Y_OVERLAY + 4);
    buddyPrint("!");
  }
}

// ─── CELEBRATE ───  ~5.6s cycle, hop and flap + confetti
static void doCelebrate(uint32_t t) {
  static const char* const CROUCH[5] = { "           ", "    ,,     ", "  <(^ )__  ", "   (  ._>  ", "   /`--'\\  " };
  static const char* const JUMP[5]   = { "    ,,     ", "  <(^ )__  ", "  \\(  ._>/ ", "    `--'   ", "    ^  ^   " };
  static const char* const PEAK[5]   = { "  \\ ,, /   ", "  <(^ )__  ", "   (  ._>  ", "    `--'   ", "           " };
  static const char* const FLAP_L[5] = { "    ,,     ", "  <(^ )__  ", "  ~(  ._>  ", "    `--'   ", "    ^^     " };
  static const char* const FLAP_R[5] = { "    ,,     ", "  <(^ )__  ", "   (  ._>~ ", "    `--'   ", "      ^^   " };
  static const char* const POSE[5]   = { "   \\,,/    ", "  <(^ )__  ", "  /(  ._>\\ ", "    `--'   ", "     ^^    " };

  const char* const* P[6] = { CROUCH, JUMP, PEAK, FLAP_L, FLAP_R, POSE };
  static const uint8_t SEQ[] = { 0,1,2,1,0, 3,4,3,4, 0,1,2,1,0, 5,5 };
  static const int8_t Y_SHIFT[] = { 0,-3,-6,-3,0, 0,0,0,0, 0,-3,-6,-3,0, 0,0 };
  uint8_t beat = (t / 3) % sizeof(SEQ);
  buddyPrintSprite(P[SEQ[beat]], 5, Y_SHIFT[beat], 0xFFE0);

  static const uint16_t cols[] = { BUDDY_YEL, BUDDY_HEART, BUDDY_CYAN, BUDDY_WHITE, BUDDY_BLUE };
  for (int i = 0; i < 6; i++) {
    int phase = (t * 2 + i * 11) % 22;
    int x = BUDDY_X_CENTER - 36 + i * 14;
    int y = BUDDY_Y_OVERLAY - 6 + phase;
    if (y > BUDDY_Y_BASE + 20 || y < 0) continue;
    buddySetColor(cols[i % 5]);
    buddySetCursor(x, y);
    buddyPrint((i + (int)(t/2)) & 1 ? "*" : ".");
  }
}

// ─── DIZZY ───  ~5.6s cycle, woozy wobble + orbiting stars
static void doDizzy(uint32_t t) {
  static const char* const TILT_L[5]  = { "   ,,      ", " <(@ )__   ", "  (  .~>   ", "   `--'    ", "    ^^     " };
  static const char* const TILT_R[5]  = { "     ,,    ", "   <(@ )__ ", "    (  .~> ", "     `--'  ", "      ^^   " };
  static const char* const WOOZY[5]   = { "    ,,     ", "  <(x@)__  ", "   ( ~~>   ", "    `--'   ", "     ^^    " };
  static const char* const WOOZY2[5]  = { "    ,,     ", "  <(@x)__  ", "   ( ~~>   ", "    `--'   ", "     ^^    " };
  static const char* const STUMBLE[5] = { "    ,,     ", "  <(@ )__  ", "   (  ~~>  ", "    `--'   ", "   ^    ^  " };

  const char* const* P[5] = { TILT_L, TILT_R, WOOZY, WOOZY2, STUMBLE };
  static const uint8_t SEQ[] = { 0,1,0,1, 2,3, 0,1,0,1, 4,4, 2,3 };
  static const int8_t X_SHIFT[] = { -3,3,-3,3, 0,0, -3,3,-3,3, 0,0, 0,0 };
  uint8_t beat = (t / 4) % sizeof(SEQ);
  buddyPrintSprite(P[SEQ[beat]], 5, 0, 0xFFE0, X_SHIFT[beat]);

  static const int8_t OX[] = { 0, 5, 7, 5, 0, -5, -7, -5 };
  static const int8_t OY[] = { -5, -3, 0, 3, 5, 3, 0, -3 };
  uint8_t p1 = t % 8;
  uint8_t p2 = (t + 4) % 8;
  buddySetColor(BUDDY_CYAN);
  buddySetCursor(BUDDY_X_CENTER + OX[p1] - 2, BUDDY_Y_OVERLAY + 6 + OY[p1]);
  buddyPrint("*");
  buddySetColor(BUDDY_YEL);
  buddySetCursor(BUDDY_X_CENTER + OX[p2] - 2, BUDDY_Y_OVERLAY + 6 + OY[p2]);
  buddyPrint("*");
}

// ─── HEART ───  ~10s cycle, dreamy float + heart stream
static void doHeart(uint32_t t) {
  static const char* const DREAMY[5] = { "    ,,     ", "  <(^ )__  ", "   (  ._>  ", "    `--'   ", "     ^^    " };
  static const char* const BLUSH[5]  = { "    ,,     ", "  <(^#)__  ", "   (  ._>  ", "    `--'   ", "     ^^    " };
  static const char* const EYES_C[5] = { "    ,,     ", "  <(<3)__  ", "   (  ._>  ", "    `--'   ", "     ^^    " };
  static const char* const TWIRL[5]  = { "    ,,     ", "  <(@ )__  ", "   (  ._>  ", "    `--'   ", "    ^  ^   " };
  static const char* const SIGH[5]   = { "    ,,     ", "  <(- )__  ", "   (  ^_>  ", "    `--'   ", "     ^^    " };

  const char* const* P[5] = { DREAMY, BLUSH, EYES_C, TWIRL, SIGH };
  static const uint8_t SEQ[] = {
    0,0,1,0, 2,2,0, 1,0,4, 0,0,3,3, 0,1,0,2, 1,0
  };
  static const int8_t Y_BOB[] = { 0,-1,0,-1, 0,-1,0, -1,0,0, -1,0,0,0, -1,0,-1,0, -1,0 };
  uint8_t beat = (t / 5) % sizeof(SEQ);
  buddyPrintSprite(P[SEQ[beat]], 5, Y_BOB[beat], 0xFFE0);

  buddySetColor(BUDDY_HEART);
  for (int i = 0; i < 5; i++) {
    int phase = (t + i * 4) % 16;
    int y = BUDDY_Y_OVERLAY + 16 - phase;
    if (y < -2 || y > BUDDY_Y_BASE) continue;
    int x = BUDDY_X_CENTER - 20 + i * 8 + ((phase / 3) & 1) * 2 - 1;
    buddySetCursor(x, y);
    buddyPrint("v");
  }
}

}  // namespace chick

extern const Species CHICK_SPECIES = {
  "chick",
  0xFFE0,
  { chick::doSleep, chick::doIdle, chick::doBusy, chick::doAttention,
    chick::doCelebrate, chick::doDizzy, chick::doHeart }
};
