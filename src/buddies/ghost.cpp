#include "../buddy_common.h"

namespace ghost {

// ─── SLEEP ───  ~12s cycle, drifting + fading
static void doSleep(uint32_t t) {
  static const char* const DRIFT_A[5] = { "            ", "  .------.  ", " ( -    - ) ", " |        | ", " '~^~^^~^~' " };
  static const char* const DRIFT_B[5] = { "            ", "  .------.  ", " ( -    - ) ", " |        | ", " '^~^~~^~^' " };
  static const char* const FADE[5]    = { "            ", "  . ---- .  ", " ( -    - ) ", " .        . ", " . . .. . . " };
  static const char* const GONE[5]    = { "            ", "            ", "   .    .   ", "            ", "  . .  . .  " };
  static const char* const RETURN[5]  = { "            ", "   .    .   ", " ( -    - ) ", " .        . ", " .  ~^^~  . " };
  static const char* const SNORE[5]   = { "            ", "  .------.  ", " ( z    z ) ", " |   o    | ", " '~~^~~^~~' " };

  const char* const* P[6] = { DRIFT_A, DRIFT_B, FADE, GONE, RETURN, SNORE };
  static const uint8_t SEQ[] = {
    0,1,0,1,0,1, 5,5, 0,1,0,1,
    2,2, 3,3, 4,4, 0,1,
    0,1,5,5, 2,3,4,0,1
  };
  static const int8_t Y_BOB[] = {
    0,-1,0,-1,0,-1, 0,-1, -1,-2,-1,-2,
    -2,-3, -3,-3, -2,-1, -1,0,
    0,-1,0,-1, -1,-2,-1,0,-1
  };
  uint8_t beat = (t / 5) % sizeof(SEQ);
  buddyPrintBody(P[SEQ[beat]], Y_BOB[beat]);

  buddyZs(t, 12, 4, 8, 18, 16, 8);
}

// ─── IDLE ───  ~14s cycle, 10 micro-poses
static void doIdle(uint32_t t) {
  static const char* const HOVER[5]   = { "            ", "  .------.  ", " ( o    o ) ", " |   __   | ", " '~^~^^~^~' " };
  static const char* const LOOK_L[5]  = { "            ", "  .------.  ", " (o    o  ) ", " |   __   | ", " '~^~^^~^~' " };
  static const char* const LOOK_R[5]  = { "            ", "  .------.  ", " (  o    o) ", " |   __   | ", " '~^~^^~^~' " };
  static const char* const LOOK_U[5]  = { "            ", "  .------.  ", " ( ^    ^ ) ", " |   __   | ", " '~^~^^~^~' " };
  static const char* const BLINK[5]   = { "            ", "  .------.  ", " ( -    - ) ", " |   __   | ", " '~^~^^~^~' " };
  static const char* const SMILE[5]   = { "            ", "  .------.  ", " ( o    o ) ", " |   uu   | ", " '~^~^^~^~' " };
  static const char* const WIGGLE[5]  = { "            ", "  .------.  ", " ( o    o ) ", " |   __   | ", " '^~^~~^~^' " };
  static const char* const WIGGLE2[5] = { "            ", "  .------.  ", " ( o    o ) ", " |   __   | ", " '~~^~~^~~' " };
  static const char* const TRANS[5]   = { "            ", "  . ---- .  ", " . o    o . ", " .   __   . ", " . ~^~^^~ . " };
  static const char* const PHASE[5]   = { "            ", "  .------.  ", " ( O    O ) ", " |   oo   | ", " '~^~^^~^~' " };

  const char* const* P[10] = { HOVER, LOOK_L, LOOK_R, LOOK_U, BLINK, SMILE, WIGGLE, WIGGLE2, TRANS, PHASE };
  static const uint8_t SEQ[] = {
    0,0,6,7,0,1,0,2,
    0,4,0,6,
    5,5,0,0,
    7,6,0,3,0,4,
    8,8,0,0,
    9,9,0,6,7,0
  };
  static const int8_t Y_BOB[] = {
    0,-1,-1,0,-1,0,-1,0,
    -1,0,-1,-1,
    -1,0,-1,0,
    0,-1,0,-1,0,-1,
    -2,-1,-1,0,
    -1,0,-1,0,-1,0
  };
  uint8_t beat = (t / 5) % sizeof(SEQ);
  buddyPrintBody(P[SEQ[beat]], Y_BOB[beat]);
}

// ─── PLAY ───  ~10s cycle, 6 poses + dot ticker
// Not played anywhere yet: see "A playing animation" in docs/toy-box.md.
// Desk work from the original project. Only the sparkle pose (CONJURE) is worth keeping. Redraw.
static void doPlay(uint32_t t) {
  static const char* const WORK_A[5]  = { "            ", "  .------.  ", " ( v    v ) ", " |   --   | ", " /~^~^^~^~\\ " };
  static const char* const WORK_B[5]  = { "            ", "  .------.  ", " ( v    v ) ", " |   __   | ", " \\~^~^^~^~/ " };
  static const char* const THINK[5]   = { "      ?     ", "  .------.  ", " ( ^    ^ ) ", " |   ..   | ", " '~^~^^~^~' " };
  static const char* const CONJURE[5] = { "    * . *   ", "  .------.  ", " ( O    O ) ", " |   oo   | ", " '~^~^^~^~' " };
  static const char* const EUREKA[5]  = { "      !     ", "  .------.  ", " ( O    O ) ", " |   ^^   | ", " /~^~^^~^~\\ " };
  static const char* const RELIEF[5]  = { "    ~ ~ ~   ", "  .------.  ", " ( -    - ) ", " |   __   | ", " '^~^~~^~^' " };

  const char* const* P[6] = { WORK_A, WORK_B, THINK, CONJURE, EUREKA, RELIEF };
  static const uint8_t SEQ[] = {
    0,1,0,1,0,1, 2,2, 0,1,0,1, 3,3, 2,4, 0,1,0,1,5
  };
  uint8_t beat = (t / 5) % sizeof(SEQ);
  buddyPrintBody(P[SEQ[beat]]);

  static const char* const DOTS[] = { "o  ", "oO ", "oOo", " Oo", "  o", "   " };
  buddySetColor(AQUA);
  buddySetCursor(BUDDY_X_CENTER + 22, BUDDY_Y_OVERLAY + 14);
  buddyPrint(DOTS[t % 6]);
}

// ─── ATTENTION ───  ~8s cycle, 6 poses + BOO! pulse
static void doAttention(uint32_t t) {
  static const char* const SPOOK[5]   = { "    ! !!    ", "  .------.  ", " ( O    O ) ", " |   OO   | ", " /~^~^^~^~\\ " };
  static const char* const SCAN_L[5]  = { "    ! !!    ", "  .------.  ", " (O      O) ", " |   O    | ", " '~^~^^~^~' " };
  static const char* const SCAN_R[5]  = { "    ! !!    ", "  .------.  ", " ( O    O ) ", " |    O   | ", " '~^~^^~^~' " };
  static const char* const SCAN_U[5]  = { "    ! !!    ", "  .------.  ", " ( ^    ^ ) ", " |   O    | ", " '~^~^^~^~' " };
  static const char* const TENSE[5]   = { "    ! !!    ", " /.------.\\ ", " ( O    O ) ", " |   O    | ", " /~^~^^~^~\\ " };
  static const char* const HUSH[5]    = { "    .  .    ", "  .------.  ", " ( o    o ) ", " |   .    | ", " '~^~^^~^~' " };

  const char* const* P[6] = { SPOOK, SCAN_L, SCAN_R, SCAN_U, TENSE, HUSH };
  static const uint8_t SEQ[] = {
    0,4,0,1,0,2,0,3, 4,4,0,1,2,0, 5,0
  };
  uint8_t beat = (t / 5) % sizeof(SEQ);
  uint8_t pose = SEQ[beat];
  int xOff = (pose == 4) ? ((t & 1) ? 1 : -1) : 0;
  buddyPrintBody(P[pose], 0, xOff);

  buddyBangs(t, 6);
}

// ─── CELEBRATE ───  ~5.6s cycle, 6 poses + confetti rain
static void doCelebrate(uint32_t t) {
  static const char* const CROUCH[5]  = { "            ", "  .------.  ", " ( ^    ^ ) ", " |   ww   | ", " /~^~^^~^~\\ " };
  static const char* const JUMP[5]    = { "  \\(    )/  ", "  .------.  ", " ( ^    ^ ) ", " |   ww   | ", " '~^~^^~^~' " };
  static const char* const PEAK[5]    = { "  \\^    ^/  ", "  .------.  ", " ( ^    ^ ) ", " |   WW   | ", " '~^~^^~^~' " };
  static const char* const SPIN_L[5]  = { "            ", "  .------.  ", " ( <    < ) ", " |   ww   | ", " '~^~^^~^~/ " };
  static const char* const SPIN_R[5]  = { "            ", "  .------.  ", " ( >    > ) ", " |   ww   | ", " \\~^~^^~^~' " };
  static const char* const POSE[5]    = { "    \\__/    ", "  .------.  ", " ( ^    ^ ) ", " |   WW   | ", " /~^~^^~^~\\ " };

  const char* const* P[6] = { CROUCH, JUMP, PEAK, SPIN_L, SPIN_R, POSE };
  static const uint8_t SEQ[] = { 0,1,2,1,0, 3,4,3,4, 0,1,2,1,0, 5,5 };
  static const int8_t Y_SHIFT[] = { 0,-3,-6,-3,0, 0,0,0,0, 0,-3,-6,-3,0, 0,0 };
  uint8_t beat = (t / 3) % sizeof(SEQ);
  buddyPrintBody(P[SEQ[beat]], Y_SHIFT[beat]);

  buddyConfetti(t, "o");
}

// ─── DIZZY ───  ~5.6s cycle, 5 poses + orbiting stars
static void doDizzy(uint32_t t) {
  static const char* const TILT_L[5]  = { "            ", " .------.   ", "( @    @ )  ", "|   ~~   |  ", "'~^~^^~^~'  " };
  static const char* const TILT_R[5]  = { "            ", "   .------. ", " ( @    @ ) ", "  |   ~~   |", "  '~^~^^~^~'" };
  static const char* const WOOZY[5]   = { "            ", "  .------.  ", " ( x    @ ) ", " |   ~v   | ", " '~^~^^~^~' " };
  static const char* const WOOZY2[5]  = { "            ", "  .------.  ", " ( @    x ) ", " |   v~   | ", " '~^~^^~^~' " };
  static const char* const WOBBLE[5]  = { "            ", "  .------.  ", " ( @    @ ) ", " |   --   | ", " /~_~^^~_~\\ " };

  const char* const* P[5] = { TILT_L, TILT_R, WOOZY, WOOZY2, WOBBLE };
  static const uint8_t SEQ[] = { 0,1,0,1, 2,3, 0,1,0,1, 4,4, 2,3 };
  static const int8_t X_SHIFT[] = { -3,3,-3,3, 0,0, -3,3,-3,3, 0,0, 0,0 };
  uint8_t beat = (t / 4) % sizeof(SEQ);
  buddyPrintBody(P[SEQ[beat]], 0, X_SHIFT[beat]);

  buddyStars(t);
}

// ─── HEART ───  ~10s cycle, 5 poses + rising heart stream
static void doHeart(uint32_t t) {
  static const char* const DREAMY[5]  = { "            ", "  .------.  ", " ( ^    ^ ) ", " |   ww   | ", " '~^~^^~^~' " };
  static const char* const BLUSH[5]   = { "            ", "  .------.  ", " (#^    ^#) ", " |   ww   | ", " '~^~^^~^~' " };
  static const char* const EYES_C[5]  = { "            ", "  .------.  ", " ( <3  <3 ) ", " |   ww   | ", " '~^~^^~^~' " };
  static const char* const TWIRL[5]   = { "            ", "  .------.  ", " ( @    @ ) ", " |   ww   | ", " /~^~^^~^~\\ " };
  static const char* const SIGH[5]    = { "            ", "  .------.  ", " ( -    - ) ", " |   ^^   | ", " '^~^~~^~^' " };

  const char* const* P[5] = { DREAMY, BLUSH, EYES_C, TWIRL, SIGH };
  static const uint8_t SEQ[] = {
    0,0,1,0, 2,2,0, 1,0,4, 0,0,3,3, 0,1,0,2, 1,0
  };
  static const int8_t Y_BOB[] = { -1,-2,-1,-2, -1,-2,-1, -2,-1,-1, -2,-1,-1,-1, -2,-1,-2,-1, -2,-1 };
  uint8_t beat = (t / 5) % sizeof(SEQ);
  buddyPrintBody(P[SEQ[beat]], Y_BOB[beat]);

  buddyHearts(t);
}

}  // namespace ghost

extern const Species GHOST_SPECIES = { "ghost", 0xFFFF, { ghost::doSleep, ghost::doIdle, ghost::doPlay, ghost::doAttention, ghost::doCelebrate, ghost::doDizzy, ghost::doHeart } };
