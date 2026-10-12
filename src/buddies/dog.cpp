#include "../buddy_common.h"

namespace dog {

// ─── SLEEP ───  ~12s cycle, 6 poses (flat out on the floor, breathing, tail twitch)
static void doSleep(uint32_t t) {
  static const char* const LOAF[5]     = { "            ", "            ", "  .-----.  ", "U( -   - )U", " (_______) " };
  static const char* const BREATHE[5]  = { "            ", "            ", "  .-----.  ", "U( -   - )U", " (_______)_" };
  static const char* const CURL[5]     = { "            ", "            ", "  .-----.  ", "U( _   _ )U", " (_______) " };
  static const char* const CURL_TW[5]  = { "            ", "            ", "  .-----.  ", "U( _   _ )U", " (_______)/" };
  static const char* const PURR[5]     = { "            ", "            ", "  .-----.  ", "U( u   u )U", " (_______)_" };

  const char* const* P[6] = { LOAF, BREATHE, LOAF, PURR, CURL, CURL_TW };
  static const uint8_t SEQ[] = {
    0,1,0,1,0,1,
    3,3,0,1,
    4,5,4,5,4,5,
    2,2,
    0,1,0,1,
    5,5,4,4
  };
  uint8_t beat = (t / 5) % sizeof(SEQ);
  buddyPrintBody(P[SEQ[beat]]);

  buddyZs(t, 12, 5, 9, 18, 18, 8);
}

// ─── IDLE ───  ~16s cycle, 10 poses (ear flicks, tail wags, panting)
static void doIdle(uint32_t t) {
  static const char* const REST[5]    = { "            ", "  .-----.  ", "U( o   o )U", " (   Y   ) ", "  (\")_(\")  " };
  static const char* const LOOK_L[5]  = { "            ", "  .-----.  ", "U(o    o )U", " (   Y   ) ", "  (\")_(\")  " };
  static const char* const LOOK_R[5]  = { "            ", "  .-----.  ", "U( o    o)U", " (   Y   ) ", "  (\")_(\")  " };
  static const char* const BLINK[5]   = { "            ", "  .-----.  ", "U( -   - )U", " (   Y   ) ", "  (\")_(\")  " };
  static const char* const SLOW_BL[5] = { "            ", "  .-----.  ", "U( _   _ )U", " (   Y   ) ", "  (\")_(\")  " };
  static const char* const EAR_L[5]   = { "            ", "  .-----.  ", "/( o   o )U", " (   Y   ) ", "  (\")_(\")  " };
  static const char* const EAR_R[5]   = { "            ", "  .-----.  ", "U( o   o )\\", " (   Y   ) ", "  (\")_(\")  " };
  static const char* const TAIL_L[5]  = { "            ", "  .-----.  ", "U( o   o )U", " (   Y   ) ", "  (\")_(\")/ " };
  static const char* const TAIL_R[5]  = { "            ", "  .-----.  ", "U( o   o )U", " (   Y   ) ", "  (\")_(\")_ " };
  static const char* const GROOM[5]   = { "            ", "  .-----.  ", "U( ^   ^ )U", " (   Yu  ) ", "  (\")_(\")  " };

  const char* const* P[10] = { REST, LOOK_L, LOOK_R, BLINK, SLOW_BL, EAR_L, EAR_R, TAIL_L, TAIL_R, GROOM };
  static const uint8_t SEQ[] = {
    0,0,0,3,0,1,0,2,0,
    7,8,7,8,7,
    0,5,0,6,0,
    4,4,0,
    9,9,9,0,
    0,3,0,
    8,7,8,7,
    0,0,4,0
  };
  uint8_t beat = (t / 5) % sizeof(SEQ);
  buddyPrintBody(P[SEQ[beat]]);
}

// ─── PLAY ───  ~10s cycle, 6 poses + dot ticker
// Not played anywhere yet: see "A playing animation" in docs/toy-box.md.
// Noses a ball along. Usable as it is.
static void doPlay(uint32_t t) {
  static const char* const PAW_UP[5]  = { "      .     ", "  .-----.  ", "U( o   o )U", " (   Y   )/", "  (\")_(\")  " };
  static const char* const PAW_TAP[5] = { "    .       ", "  .-----.  ", "U( o   o )U", " (   Y   )_", "  (\")_(\")  " };
  static const char* const STARE[5]   = { "            ", "  .-----.  ", "U( O   O )U", " (   Y   ) ", "  (\")_(\")  " };
  static const char* const NUDGE[5]   = { "    o       ", "  .-----.  ", "U( o   o )U", " (  -Y   ) ", "  (\")_(\")  " };
  static const char* const SHOVE[5]   = { "  o         ", "  .-----.  ", "U( o   o )U", " ( -Y    ) ", "  (\")_(\")  " };
  static const char* const SMUG[5]    = { "            ", "  .-----.  ", "U( -   - )U", " (   Y   ) ", "  (\")_(\")  " };

  const char* const* P[6] = { PAW_UP, PAW_TAP, STARE, NUDGE, SHOVE, SMUG };
  static const uint8_t SEQ[] = {
    2,2,2, 0,1,0,1, 3,4,3,4, 5,5, 2,2, 0,1,0,1, 5,2
  };
  uint8_t beat = (t / 5) % sizeof(SEQ);
  buddyPrintBody(P[SEQ[beat]]);

  buddyDots(t);
}

// ─── ATTENTION ───  ~8s cycle, 6 poses + ! pulse (ears up, a bark)
static void doAttention(uint32_t t) {
  static const char* const ALERT[5]   = { "            ", "  .-----.  ", "/( O   O )\\", " (   Y   ) ", "  (\")_(\")  " };
  static const char* const SCAN_L[5]  = { "            ", "  .-----.  ", "/(O    O )\\", " (   Y   ) ", "  (\")_(\")  " };
  static const char* const SCAN_R[5]  = { "            ", "  .-----.  ", "/( O    O)\\", " (   Y   ) ", "  (\")_(\")  " };
  static const char* const SCAN_U[5]  = { "            ", "  .-----.  ", "/( ^   ^ )\\", " (   Y   ) ", "  (\")_(\")  " };
  static const char* const CROUCH[5]  = { "            ", "  .-----.  ", "/( O   O )\\", " (   Y   ) ", " /(\")_(\")\\ " };
  static const char* const HISS[5]    = { "            ", "  .-----.  ", "/( O   O )\\", " (   O   ) ", "  (\")_(\")  " };

  const char* const* P[6] = { ALERT, SCAN_L, SCAN_R, SCAN_U, CROUCH, HISS };
  static const uint8_t SEQ[] = {
    0,4,0,1,0,2,0,3, 4,4,0,1,2,0, 5,0
  };
  uint8_t beat = (t / 5) % sizeof(SEQ);
  uint8_t pose = SEQ[beat];
  int xOff = (pose == 4) ? ((t & 1) ? 1 : -1) : 0;
  buddyPrintBody(P[pose], 0, xOff);

  buddyBangs(t, 4);
}

// ─── CELEBRATE ───  ~5s cycle, 6 poses + confetti rain (zoomies)
static void doCelebrate(uint32_t t) {
  static const char* const CROUCH[5]  = { "            ", "  .-----.  ", "U( ^   ^ )U", " (   Yu  ) ", " /(\")_(\")\\ " };
  static const char* const JUMP[5]    = { "  \\^   ^/  ", "  .-----.  ", "U( ^   ^ )U", " (   Yu  ) ", "  (\")_(\")  " };
  static const char* const PEAK[5]    = { "  \\^   ^/  ", "  .-----.  ", "U( * * * )U", " (   Yu  ) ", "  (\")_(\")/ " };
  static const char* const SPIN_L[5]  = { "            ", "  .-----.  ", "U( <   < )U", " (   Yu  )/", "  (\")_(\")  " };
  static const char* const SPIN_R[5]  = { "            ", "  .-----.  ", "U( >   > )U", "\\(   Yu  ) ", "  (\")_(\")/ " };
  static const char* const POSE[5]    = { "    \\o/    ", "  .-----.  ", "U( ^   ^ )U", "/(   Yu  )\\", "  (\")_(\")  " };

  const char* const* P[6] = { CROUCH, JUMP, PEAK, SPIN_L, SPIN_R, POSE };
  static const uint8_t SEQ[] = { 0,1,2,1,0, 3,4,3,4, 0,1,2,1,0, 5,5 };
  static const int8_t Y_SHIFT[] = { 0,-3,-6,-3,0, 0,0,0,0, 0,-3,-6,-3,0, 0,0 };
  uint8_t beat = (t / 3) % sizeof(SEQ);
  buddyPrintBody(P[SEQ[beat]], Y_SHIFT[beat]);

  buddyConfetti(t, ".");
}

// ─── DIZZY ───  ~6s cycle, 5 poses + orbiting stars (chased its tail too long)
static void doDizzy(uint32_t t) {
  static const char* const TILT_L[5]  = { "            ", " .-----.   ", "U( @   @ )U", " (  ~~~  ) ", " (\")_(\")   " };
  static const char* const TILT_R[5]  = { "            ", "   .-----. ", "U( @   @ )U", " (  ~~~  ) ", "   (\")_(\") " };
  static const char* const WOOZY[5]   = { "            ", "  .-----.  ", "U( x   @ )U", " (   Y   ) ", "  (\")_(\")/ " };
  static const char* const WOOZY2[5]  = { "            ", "  .-----.  ", "U( @   x )U", " (   Y   ) ", "  (\")_(\")  " };
  static const char* const SPLAT[5]   = { "            ", "  .-----.  ", "U( @   @ )U", " (   -   ) ", " /(\")_(\")\\ " };

  const char* const* P[5] = { TILT_L, TILT_R, WOOZY, WOOZY2, SPLAT };
  static const uint8_t SEQ[] = { 0,1,0,1, 2,3, 0,1,0,1, 4,4, 2,3 };
  static const int8_t X_SHIFT[] = { -3,3,-3,3, 0,0, -3,3,-3,3, 0,0, 0,0 };
  uint8_t beat = (t / 4) % sizeof(SEQ);
  buddyPrintBody(P[SEQ[beat]], 0, X_SHIFT[beat]);

  buddyStars(t);
}

// ─── HEART ───  ~10s cycle, 5 poses + rising hearts (licks and wags)
static void doHeart(uint32_t t) {
  static const char* const DREAMY[5]  = { "            ", "  .-----.  ", "U( ^   ^ )U", " (   Yu  ) ", "  (\")_(\")/ " };
  static const char* const BLUSH[5]   = { "            ", "  .-----.  ", "U(#^   ^#)U", " (   Yu  ) ", "  (\")_(\")  " };
  static const char* const HEART_E[5] = { "            ", "  .-----.  ", "U( <3 <3 )U", " (   Yu  ) ", "  (\")_(\")/ " };
  static const char* const PURR[5]    = { "            ", "  .-----.  ", "U( ~   ~ )U", " (   Yu  ) ", "  (\")_(\")/ " };
  static const char* const HEAD_T[5]  = { "            ", "  .-----.  ", "U( ^   - )U", " (   Yu  ) ", "  (\")_(\")  " };

  const char* const* P[5] = { DREAMY, BLUSH, HEART_E, PURR, HEAD_T };
  static const uint8_t SEQ[] = {
    0,0,1,0, 2,2,0, 1,0,4, 0,0,3,3, 0,1,0,2, 1,0
  };
  static const int8_t Y_BOB[] = { 0,-1,0,-1, 0,-1,0, -1,0,0, -1,0,0,0, -1,0,-1,0, -1,0 };
  uint8_t beat = (t / 5) % sizeof(SEQ);
  buddyPrintBody(P[SEQ[beat]], Y_BOB[beat]);

  buddyHearts(t);
}

}  // namespace dog

extern const Species DOG_SPECIES = {
  "dog",
  0xD54B,
  { dog::doSleep, dog::doIdle, dog::doPlay, dog::doAttention,
    dog::doCelebrate, dog::doDizzy, dog::doHeart }
};
