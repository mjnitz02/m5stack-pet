#include "../buddy_common.h"

namespace blob {

// ─── SLEEP ───  ~12s cycle, 6 poses
static void doSleep(uint32_t t) {
  static const char* const PUDDLE[5]  = { "            ", "            ", "   .----.   ", "  ( -- -- ) ", "  `~------~`" };
  static const char* const BREATH[5]  = { "            ", "   .----.   ", "  ( -- -- ) ", "  (        )", "   `------` " };
  static const char* const DEEP[5]    = { "            ", "  .------.  ", " ( -- -- ) ", " (         )", "  `~------~`" };
  static const char* const DRIP[5]    = { "            ", "            ", "   .----.   ", "  ( -- -- ) ", "  `--.----` " };
  static const char* const MELT[5]    = { "            ", "            ", "   .----.   ", "  ( __ __ ) ", " `~~~~~~~~~`" };
  static const char* const SNORE[5]   = { "            ", "  .------.  ", " ( __ __ )  ", " (    o    )", "  `~------~`" };

  const char* const* P[6] = { PUDDLE, BREATH, DEEP, DRIP, MELT, SNORE };
  static const uint8_t SEQ[] = {
    0,1,2,1,0,1,2,1,
    0,4,4,0,
    1,2,5,2,1,
    3,3,0,0,
    1,2,1,0
  };
  uint8_t beat = (t / 5) % sizeof(SEQ);
  buddyPrintBody(P[SEQ[beat]]);

  buddyZs(t, 10, 4, 7, 20, 18, 10);

  // slow droplet falling under the puddle
  int dphase = (t / 2) % 12;
  buddySetColor(buddyBodyColor());
  buddySetCursor(BUDDY_X_CENTER - 6, BUDDY_Y_BASE + 26 + dphase);
  buddyPrint(dphase < 8 ? "." : " ");
}

// ─── IDLE ───  ~14s cycle, 10 poses
static void doIdle(uint32_t t) {
  static const char* const SMALL[5]   = { "            ", "    .--.    ", "   (o  o)   ", "   (    )   ", "    `--`    " };
  static const char* const MED[5]     = { "            ", "   .----.   ", "  ( o  o )  ", "  (      )  ", "   `----`   " };
  static const char* const BIG[5]     = { "            ", "  .------.  ", " ( o    o ) ", " (        ) ", "  `------`  " };
  static const char* const LOOK_L[5]  = { "            ", "   .----.   ", "  (o   o )  ", "  (      )  ", "   `----`   " };
  static const char* const LOOK_R[5]  = { "            ", "   .----.   ", "  ( o   o)  ", "  (      )  ", "   `----`   " };
  static const char* const BLINK[5]   = { "            ", "   .----.   ", "  ( -  - )  ", "  (      )  ", "   `----`   " };
  static const char* const WIGGLE_L[5]= { "            ", "  .----.    ", " ( o  o )   ", " (      )   ", "  `----`    " };
  static const char* const WIGGLE_R[5]= { "            ", "    .----.  ", "   ( o  o ) ", "   (      ) ", "    `----`  " };
  static const char* const JIGGLE[5]  = { "            ", "  .~~~~~~.  ", " ( o    o ) ", " (        ) ", "  `~~~~~~`  " };
  static const char* const DRIP_S[5]  = { "            ", "   .----.   ", "  ( o  o )  ", "  (      )  ", "  `--.--.`  " };

  const char* const* P[10] = { SMALL, MED, BIG, LOOK_L, LOOK_R, BLINK, WIGGLE_L, WIGGLE_R, JIGGLE, DRIP_S };
  static const uint8_t SEQ[] = {
    1,2,1,0,1,2, 1,3,1,4,1,5,
    2,2,8,8,2,
    6,7,6,7, 1,1,
    2,9,9,1,
    1,3,4,3,4,1,5,1,
    0,0,2,2
  };
  uint8_t beat = (t / 5) % sizeof(SEQ);
  buddyPrintBody(P[SEQ[beat]]);
}

// ─── PLAY ───  1.6s a bounce, 3 poses (bounces a ball on its head)
static void doPlay(uint32_t t) {
  static const char* const WATCH[5] = { "            ", "   .----.   ", "  ( ^  ^ )  ", "  (  ww  )  ", "   `----`   " };
  static const char* const READY[5] = { "            ", "   .----.   ", "  ( o  o )  ", "  (  uu  )  ", "   `----`   " };
  static const char* const BONK[5]  = { "            ", "  .------.  ", " ( >    < ) ", " (   ww   ) ", "  `------`  " };

  // Where the ball is, from on the head to the top of its bounce and back.
  static const int8_t UP[8] = { 0, 6, 12, 16, 18, 16, 12, 6 };
  uint8_t p = t % 8;
  buddyPrintBody(p == 0 ? BONK : (p == 1 || p == 7) ? READY : WATCH);

  buddySetColor(YEL);
  int y = BUDDY_Y_OVERLAY + 21 - buddyHatHeight() - UP[p];
  buddySetCursor(BUDDY_X_CENTER - 3, y < 1 ? 1 : y);   // a tall hat leaves less room under the top edge
  buddyPrint("o");
}

// ─── ATTENTION ───  ~8s cycle, 6 poses + ! pulse
static void doAttention(uint32_t t) {
  static const char* const TALL[5]    = { "    .--.    ", "   (    )   ", "  ( O  O )  ", "  (   !   ) ", "  `------`  " };
  static const char* const PEEK_L[5]  = { "    .--.    ", "   (    )   ", " ( O  O  )  ", " (   !    ) ", " `------`   " };
  static const char* const PEEK_R[5]  = { "    .--.    ", "   (    )   ", "  ( O  O )  ", "   (   !  ) ", "   `------` " };
  static const char* const STRETCH[5] = { "     ||     ", "    /  \\    ", "  ( O  O )  ", "  (   !   ) ", "  `------`  " };
  static const char* const TENSE[5]   = { "    .--.    ", "  /(    )\\  ", " /( O  O )\\ ", " (   !!   ) ", " /`------`\\ " };
  static const char* const SHRINK[5]  = { "            ", "    .--.    ", "   (O  O)   ", "   (  !  )  ", "    `--`    " };

  const char* const* P[7] = { TALL, PEEK_L, TALL, PEEK_R, STRETCH, TENSE, SHRINK };
  static const uint8_t SEQ[] = {
    0,4,0,1,0,2,0,3, 4,4,0,1,2,0, 5,3, 0,0,6,0
  };
  uint8_t beat = (t / 5) % sizeof(SEQ);
  uint8_t pose = SEQ[beat];
  int xOff = (pose == 4) ? ((t & 1) ? 1 : -1) : 0;
  buddyPrintBody(P[pose], 0, xOff);

  if ((t / 2) & 1) {
    buddySetColor(YEL);
    buddySetCursor(BUDDY_X_CENTER - 8, BUDDY_Y_OVERLAY - 4);
    buddyPrint("!");
  }
  if ((t / 3) & 1) {
    buddySetColor(RED);
    buddySetCursor(BUDDY_X_CENTER + 8, BUDDY_Y_OVERLAY);
    buddyPrint("!");
  }
  if ((t / 4) & 1) {
    buddySetColor(YEL);
    buddySetCursor(BUDDY_X_CENTER, BUDDY_Y_OVERLAY - 8);
    buddyPrint("!");
  }
}

// ─── CELEBRATE ───  ~5.6s cycle, 6 poses + droplet/confetti rain
static void doCelebrate(uint32_t t) {
  static const char* const SQUASH[5]  = { "            ", "            ", "  .--------.", " ( ^      ^)", " `~~------~`" };
  static const char* const LAUNCH[5]  = { "            ", "   .----.   ", "  ( ^  ^ )  ", " /(  ww  )\\ ", "  `------`  " };
  static const char* const AIRBORNE[5]= { "    .--.    ", "   ( ^^ )   ", "   (  WW)   ", "    `--`    ", "    : :     " };
  static const char* const SPLAT_L[5] = { "            ", "            ", " .---------.", "( ^     ^ )", " `~~~------`" };
  static const char* const SPLAT_R[5] = { "            ", "            ", ".---------. ", "( ^     ^ )", "`------~~~` " };
  static const char* const POSE[5]    = { "    \\__/    ", "   .----.   ", "  ( *  * )  ", " /(  WW  )\\ ", "  `------`  " };

  const char* const* P[6] = { SQUASH, LAUNCH, AIRBORNE, SPLAT_L, SPLAT_R, POSE };
  static const uint8_t SEQ[] = { 0,1,2,1,0, 3,4,3,4, 0,1,2,1,0, 5,5 };
  static const int8_t Y_SHIFT[] = { 0,-2,-7,-2,0, 0,0,0,0, 0,-2,-7,-2,0, 0,0 };
  uint8_t beat = (t / 3) % sizeof(SEQ);
  buddyPrintBody(P[SEQ[beat]], Y_SHIFT[beat]);

  buddyConfetti(t, "o", buddyBodyColor());
}

// ─── DIZZY ───  ~5.6s cycle, 5 poses + orbiting stars
static void doDizzy(uint32_t t) {
  static const char* const LEAN_L[5]  = { "            ", "  .----.    ", " ( @  @ )   ", " (  ~~  )   ", "  `----`    " };
  static const char* const LEAN_R[5]  = { "            ", "    .----.  ", "   ( @  @ ) ", "   (  ~~  ) ", "    `----`  " };
  static const char* const WOBBLE[5]  = { "            ", "  .~----~.  ", " ( x    @ ) ", " (   vv   )", "  `~----~`  " };
  static const char* const WOBBLE2[5] = { "            ", "  .~----~.  ", " ( @    x ) ", " (   vv   )", "  `~----~`  " };
  static const char* const SPLAT[5]   = { "            ", "            ", " .---------.", "( @     @ )", " `--._.--._`" };

  const char* const* P[5] = { LEAN_L, LEAN_R, WOBBLE, WOBBLE2, SPLAT };
  static const uint8_t SEQ[] = { 0,1,0,1, 2,3, 0,1,0,1, 4,4, 2,3 };
  static const int8_t X_SHIFT[] = { -3,3,-3,3, 0,0, -3,3,-3,3, 0,0, 0,0 };
  uint8_t beat = (t / 4) % sizeof(SEQ);
  buddyPrintBody(P[SEQ[beat]], 0, X_SHIFT[beat]);

  buddyStars(t);
  buddyOrbit(t + 2, "o", INK);
}

// ─── HEART ───  ~10s cycle, 5 poses + rising heart stream
static void doHeart(uint32_t t) {
  static const char* const DREAMY[5]  = { "            ", "   .----.   ", "  ( ^  ^ )  ", "  (   ww  ) ", "   `----`   " };
  static const char* const BLUSH[5]   = { "            ", "   .----.   ", "  (#^  ^#)  ", "  (   ww  ) ", "   `----`   " };
  static const char* const HEART_E[5] = { "            ", "  .------.  ", " ( <3  <3 ) ", " (    v   ) ", "  `------`  " };
  static const char* const MELT_H[5]  = { "            ", "  .~~~~~~.  ", " ( @    @ ) ", " (   ww   )", "  `~------`" };
  static const char* const SIGH[5]    = { "            ", "   .----.   ", "  ( -  - )  ", "  (   ^^  ) ", "   `----`   " };

  const char* const* P[5] = { DREAMY, BLUSH, HEART_E, MELT_H, SIGH };
  static const uint8_t SEQ[] = {
    0,0,1,0, 2,2,0, 1,0,4, 0,0,3,3, 0,1,0,2, 1,0
  };
  static const int8_t Y_BOB[] = { 0,-1,0,-1, 0,-1,0, -1,0,0, -1,0,0,0, -1,0,-1,0, -1,0 };
  uint8_t beat = (t / 5) % sizeof(SEQ);
  buddyPrintBody(P[SEQ[beat]], Y_BOB[beat]);

  buddyHearts(t);
}

}  // namespace blob

extern const Species BLOB_SPECIES = {
  "blob",
  0x07F0,
  { blob::doSleep, blob::doIdle, blob::doPlay, blob::doAttention,
    blob::doCelebrate, blob::doDizzy, blob::doHeart }
};
