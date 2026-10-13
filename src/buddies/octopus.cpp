#include "../buddy_common.h"

namespace octopus {

// ─── SLEEP ───  ~12s cycle, 6 poses
static void doSleep(uint32_t t) {
  static const char* const CURL[5]    = { "            ", "            ", "   .----.   ", "  ( -- -- ) ", "  (~~zz~~)  " };
  static const char* const BREATHE[5] = { "            ", "   .----.   ", "  ( -- -- ) ", "  (______)  ", "  ~~~~~~~~  " };
  static const char* const DEEP[5]    = { "            ", "   .----.   ", "  ( __ __ ) ", "  (______)  ", "  ~~~~~~~~  " };
  static const char* const DRIFT_L[5] = { "            ", "  .----.    ", " ( -- -- )  ", " (______)   ", " ~/~/~/~/   " };
  static const char* const DRIFT_R[5] = { "            ", "    .----.  ", "   ( -- -- )", "   (______) ", "   /~/~/~/~ " };
  static const char* const SNORE[5]   = { "            ", "   .----.   ", "  ( oO oO ) ", "  (__o___)  ", "  ~~~~~~~~  " };

  const char* const* P[6] = { CURL, BREATHE, DEEP, DRIFT_L, DRIFT_R, SNORE };
  static const uint8_t SEQ[] = {
    0,0,1,1,0,0,2,2,
    1,1,0,0,
    3,3,4,4,3,4,
    1,1,2,2,
    0,5,1,1
  };
  uint8_t beat = (t / 5) % sizeof(SEQ);
  buddyPrintBody(P[SEQ[beat]]);

  buddyZs(t, 10, 4, 7, 18, 16, 8);
}

// ─── IDLE ───  ~14s cycle, 10 poses
static void doIdle(uint32_t t) {
  static const char* const REST[5]    = { "            ", "   .----.   ", "  ( o  o )  ", "  (______)  ", "  /\\/\\/\\/\\  " };
  static const char* const WAVE_A[5]  = { "            ", "   .----.   ", "  ( o  o )  ", "  (______)  ", "  \\/\\/\\/\\/  " };
  static const char* const LOOK_L[5]  = { "            ", "   .----.   ", "  (o   o )  ", "  (______)  ", "  /\\/\\/\\/\\  " };
  static const char* const LOOK_R[5]  = { "            ", "   .----.   ", "  ( o   o)  ", "  (______)  ", "  \\/\\/\\/\\/  " };
  static const char* const BLINK[5]   = { "            ", "   .----.   ", "  ( -  - )  ", "  (______)  ", "  /\\/\\/\\/\\  " };
  static const char* const CURL_T[5]  = { "            ", "   .----.   ", "  ( o  o )  ", "  (______)  ", "  /)/\\/\\(\\  " };
  static const char* const WIGGLE[5]  = { "            ", "   .----.   ", "  ( ^  ^ )  ", "  (______)  ", "  )(/\\/\\)(  " };
  static const char* const FLOAT_U[5] = { "   .----.   ", "  ( o  o )  ", "  (______)  ", "  /\\/\\/\\/\\  ", "            " };
  static const char* const GRIN[5]    = { "            ", "   .----.   ", "  ( ^  ^ )  ", "  (\\__/\\)  ", "  /\\/\\/\\/\\  " };
  static const char* const STRETCH[5] = { "            ", "  /.----.\\  ", " /( o  o )\\ ", " \\(______)/ ", " //\\/\\/\\/\\\\" };

  const char* const* P[10] = { REST, WAVE_A, LOOK_L, LOOK_R, BLINK, CURL_T, WIGGLE, FLOAT_U, GRIN, STRETCH };
  static const uint8_t SEQ[] = {
    0,1,0,1,0,2,1,3,
    0,1,4,0,
    5,6,5,6,
    0,1,0,4,1,0,
    7,7,0,1,
    8,8,0,1,
    9,9,0,1
  };
  uint8_t beat = (t / 5) % sizeof(SEQ);
  buddyPrintBody(P[SEQ[beat]]);
}

// ─── PLAY ───  1.6s a throw, 4 poses (juggles two balls)
static void doPlay(uint32_t t) {
  static const char* const EYE_L[5]  = { "            ", "   .----.   ", " \\(o   o )/ ", "  (__ww__)  ", "  /\\/\\/\\/\\  " };
  static const char* const EYE_R[5]  = { "            ", "   .----.   ", " \\( o   o)/ ", "  (__ww__)  ", "  \\/\\/\\/\\/  " };
  static const char* const UP_A[5]   = { "            ", "   .----.   ", " \\( ^  ^ )/ ", "  (__ww__)  ", "  /\\/\\/\\/\\  " };
  static const char* const UP_B[5]   = { "            ", "   .----.   ", " \\( ^  ^ )/ ", "  (__ww__)  ", "  \\/\\/\\/\\/  " };

  // Each ball goes up from its arm and comes back; the eyes follow whichever
  // is about to land.
  static const int8_t UP[8] = { 0, 7, 13, 17, 19, 17, 13, 7 };
  uint8_t p = t % 8;
  buddyPrintBody(p == 0 ? EYE_L : p == 4 ? EYE_R : (p & 1) ? UP_A : UP_B);

  buddySetColor(YEL);
  buddySetCursor(BUDDY_X_CENTER - 30, BUDDY_Y_OVERLAY + 26 - UP[p]);
  buddyPrint("o");
  buddySetColor(AQUA);
  buddySetCursor(BUDDY_X_CENTER + 24, BUDDY_Y_OVERLAY + 26 - UP[(p + 4) % 8]);
  buddyPrint("o");
}

// ─── ATTENTION ───  ~8s cycle, 6 poses + ! pulse
static void doAttention(uint32_t t) {
  static const char* const ALERT[5]   = { "    ^  ^    ", "   .----.   ", "  ( O  O )  ", "  (__O___)  ", "  /\\/\\/\\/\\  " };
  static const char* const SCAN_L[5]  = { "    ^  ^    ", "   .----.   ", "  (O   O )  ", "  (__O___)  ", "  /\\/\\/\\/\\  " };
  static const char* const SCAN_R[5]  = { "    ^  ^    ", "   .----.   ", "  ( O   O)  ", "  (__O___)  ", "  \\/\\/\\/\\/  " };
  static const char* const SCAN_U[5]  = { "    ^  ^    ", "   .----.   ", "  ( ^  ^ )  ", "  (__O___)  ", "  /\\/\\/\\/\\  " };
  static const char* const TENSE[5]   = { "    ^  ^    ", "  /.----.\\  ", " ( O    O ) ", "  (__O___)  ", " //\\/\\/\\\\\\ " };
  static const char* const HUSH[5]    = { "    ^  ^    ", "   .----.   ", "  ( o  o )  ", "  (__.___)  ", "  /\\/\\/\\/\\  " };

  const char* const* P[6] = { ALERT, SCAN_L, SCAN_R, SCAN_U, TENSE, HUSH };
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
  static const char* const CROUCH[5]  = { "            ", "   .----.   ", "  ( ^  ^ )  ", "  (__ww__)  ", " //\\/\\/\\\\\\ " };
  static const char* const JUMP[5]    = { "  \\/    \\/  ", "   .----.   ", "  ( ^  ^ )  ", "  (__ww__)  ", "  )(\\/\\/)( " };
  static const char* const PEAK[5]    = { "  \\^    ^/  ", "   .----.   ", "  ( ^  ^ )  ", "  (__WW__)  ", "  ((    ))  " };
  static const char* const SPIN_L[5]  = { "            ", "   .----.   ", " (( <  < )) ", "  (__ww__)  ", " /)/\\/\\(\\  " };
  static const char* const SPIN_R[5]  = { "            ", "   .----.   ", " (( >  > )) ", "  (__ww__)  ", " (\\/\\/\\/)/ " };
  static const char* const POSE[5]    = { "    \\__/    ", "   .----.   ", " /( ^  ^ )\\ ", " \\(__WW__)/ ", " //\\/\\/\\\\\\ " };

  const char* const* P[6] = { CROUCH, JUMP, PEAK, SPIN_L, SPIN_R, POSE };
  static const uint8_t SEQ[] = { 0,1,2,1,0, 3,4,3,4, 0,1,2,1,0, 5,5 };
  static const int8_t Y_SHIFT[] = { 0,-3,-6,-3,0, 0,0,0,0, 0,-3,-6,-3,0, 0,0 };
  uint8_t beat = (t / 3) % sizeof(SEQ);
  buddyPrintBody(P[SEQ[beat]], Y_SHIFT[beat]);

  buddyConfetti(t, "o");
}

// ─── DIZZY ───  ~5.6s cycle, 5 poses + orbiting stars + ink cloud
static void doDizzy(uint32_t t) {
  static const char* const TILT_L[5]  = { "            ", "  .----.    ", " ( @  @ )   ", " (__~~__)   ", " /\\/\\/\\/\\   " };
  static const char* const TILT_R[5]  = { "            ", "    .----.  ", "   ( @  @ ) ", "   (__~~__) ", "   \\/\\/\\/\\/ " };
  static const char* const WOOZY[5]   = { "            ", "   .----.   ", "  ( x  @ )  ", "  (__~v__)  ", "  /\\)/\\(\\  " };
  static const char* const WOOZY2[5]  = { "            ", "   .----.   ", "  ( @  x )  ", "  (__v~__)  ", "  (\\/\\(/\\/  " };
  static const char* const STUMBLE[5] = { "            ", "   .----.   ", "  ( @  @ )  ", "  (__--__)  ", " /)\\_/\\_(\\ " };

  const char* const* P[5] = { TILT_L, TILT_R, WOOZY, WOOZY2, STUMBLE };
  static const uint8_t SEQ[] = { 0,1,0,1, 2,3, 0,1,0,1, 4,4, 2,3 };
  static const int8_t X_SHIFT[] = { -3,3,-3,3, 0,0, -3,3,-3,3, 0,0, 0,0 };
  uint8_t beat = (t / 4) % sizeof(SEQ);
  buddyPrintBody(P[SEQ[beat]], 0, X_SHIFT[beat]);

  buddyStars(t, 4, PLUM);

  // Ink cloud puff drifts down occasionally
  if ((t / 8) & 1) {
    int puff = (t % 8);
    buddySetColor(DIM);
    buddySetCursor(BUDDY_X_CENTER - 24 - puff, BUDDY_Y_OVERLAY + 10 + puff);
    buddyPrint("o");
    buddySetCursor(BUDDY_X_CENTER + 24 + puff, BUDDY_Y_OVERLAY + 10 + puff);
    buddyPrint("o");
  }
}

// ─── HEART ───  ~10s cycle, 6 poses + rising heart stream
static void doHeart(uint32_t t) {
  static const char* const DREAMY[5]  = { "            ", "   .----.   ", "  ( ^  ^ )  ", "  (__ww__)  ", "  /\\/\\/\\/\\  " };
  static const char* const BLUSH[5]   = { "            ", "   .----.   ", "  (#^  ^#)  ", "  (__ww__)  ", "  /\\/\\/\\/\\  " };
  static const char* const EYES_C[5]  = { "            ", "   .----.   ", "  (<3  <3)  ", "  (__ww__)  ", "  \\/\\/\\/\\/  " };
  static const char* const TWIRL[5]   = { "            ", "   .----.   ", "  ( @  @ )  ", "  (__ww__)  ", " //\\/\\/\\\\\\ " };
  static const char* const SIGH[5]    = { "    ~~~     ", "   .----.   ", "  ( -  - )  ", "  (__^^__)  ", "  /\\/\\/\\/\\  " };
  static const char* const HUG[5]     = { "            ", "  /.----.\\  ", " /(#^  ^#)\\ ", " \\(__ww__)/ ", "  )(\\/\\/)(  " };

  const char* const* P[6] = { DREAMY, BLUSH, EYES_C, TWIRL, SIGH, HUG };
  static const uint8_t SEQ[] = {
    0,0,1,0, 2,2,0, 1,0,4, 0,0,3,3, 0,1,0,2, 5,5,1,0
  };
  static const int8_t Y_BOB[] = { 0,-1,0,-1, 0,-1,0, -1,0,0, -1,0,0,0, -1,0,-1,0, 0,-1,0,-1 };
  uint8_t beat = (t / 5) % sizeof(SEQ);
  buddyPrintBody(P[SEQ[beat]], Y_BOB[beat]);

  buddyHearts(t);
}

}  // namespace octopus

extern const Species OCTOPUS_SPECIES = {
  "octopus",
  0xA01F,
  { octopus::doSleep, octopus::doIdle, octopus::doPlay, octopus::doAttention,
    octopus::doCelebrate, octopus::doDizzy, octopus::doHeart }
};
