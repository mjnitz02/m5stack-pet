#include "../buddy_common.h"

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
  buddyPrintBody(P[SEQ[beat]]);

  buddyZs(t, 10, 4, 7, 18, 18, 10);
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
  buddyPrintBody(P[SEQ[beat]]);
}

// ─── PLAY ───  ~10s cycle, 6 poses (walks, pecks at a seed and gulps)
static void doPlay(uint32_t t) {
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
  buddyPrintBody(P[SEQ[beat]]);
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
  buddyPrintBody(P[pose], 0, xOff);

  buddyBangs(t, 4);
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
  buddyPrintBody(P[SEQ[beat]], Y_SHIFT[beat]);

  buddyConfetti(t, ".", INK, BTN_BLUE);
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
  buddyPrintBody(P[SEQ[beat]], 0, X_SHIFT[beat]);

  buddyStars(t);
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
  buddyPrintBody(P[SEQ[beat]], Y_BOB[beat]);

  buddyHearts(t);
}

}  // namespace chick

extern const Species CHICK_SPECIES = {
  "chick",
  0xFFE0,
  { chick::doSleep, chick::doIdle, chick::doPlay, chick::doAttention,
    chick::doCelebrate, chick::doDizzy, chick::doHeart }
};
