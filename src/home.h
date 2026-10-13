#pragma once

// The pet's own screens: the egg it hatches from, home, the food tray, bed,
// and the cheer that ends every activity. Header-only with file-static
// state, like activities.h: include from main.cpp only.

// --- Needs ------------------------------------------------------------------
// Only tick while the unit is on: there is no clock while it's off, so a
// pet left in a drawer comes back exactly as it was, minus one pip each for
// "woke up hungry". A nap pauses the timers (see napCarry in power.h); a
// power-off starts them again.
const uint32_t HUNGER_DECAY_MS = 8UL * 60 * 1000;
const uint32_t HAPPY_DECAY_MS  = 12UL * 60 * 1000;
uint32_t lastHungerMs = 0, lastHappyMs = 0;

// A reaction that plays over the home screen's mood for a moment.
PetState oneShotState = P_IDLE;
uint32_t oneShotUntil = 0;

static void triggerOneShot(PetState s, uint32_t durMs) {
  oneShotState = s;
  oneShotUntil = millis() + durMs;
}
static bool oneShotActive() { return before(millis(), oneShotUntil); }

// --- Home screen ------------------------------------------------------------
enum { ICON_FEED, ICON_DANCE, ICON_BED, ICON_N };
uint8_t iconSel = 0;
const int BOTTOM_TOP = 166;      // buddy strip at 2× clears y < 164

// The apples row and, under it, the hearts row.
static void drawNeeds(bool hearts = true) {
  for (int i = 0; i < PET_MAX; i++) {
    int x = 34 + i * 22;
    drawApple(x, 177, 4, i < pet().hunger);
    if (hearts) drawHeart(x, 190, 1, i < pet().happy, i < pet().happy ? RED : DIM);
  }
}

static void drawBottom(bool showSel) {
  clearBelow(BOTTOM_TOP);
  drawNeeds();
  spr.drawFastHLine(0, 201, W, DIM);
  for (int i = 0; i < ICON_N; i++) {
    int cx = 22 + i * 45, cy = 222;
    if (i == ICON_FEED)      drawApple(cx, cy + 2, 9);
    else if (i == ICON_DANCE) drawParty(cx, cy);
    else                     drawMoon(cx, cy, 9);
    if (showSel && i == iconSel) drawFrame(cx - 20, 204, 40, 35, 6);
  }
  if (batMv > 0 && batPct() < 15) {            // low battery: tell a grown-up
    spr.drawRect(W - 18, 3, 14, 8, HOT);
    spr.fillRect(W - 4, 5, 2, 4, HOT);
    spr.fillRect(W - 16, 5, 3, 4, HOT);
  }
}

static void enterHome() {
  mode = M_HOME;
  clearScreen(false);
  applyBrightness();
  lastInteractMs = millis();
}

// --- Dress-up look ----------------------------------------------------------
// Index 0 keeps the animal's own colour.
static const uint16_t LOOK_TINT[] = { 0, AQUA, ROSE, YEL, GREEN, AMBER };
const uint8_t LOOK_N = sizeof(LOOK_TINT) / sizeof(LOOK_TINT[0]);

static void applyLook() { buddySetLook(LOOK_TINT[pet().color % LOOK_N], pet().hat); }

// --- Finishing an activity --------------------------------------------------
// Every activity ends here: a heart, and one time in three a sticker the
// kid doesn't have yet, shown off before going back to the pet.
const uint8_t STICKER_ODDS = 3;
uint8_t  rewardSticker = 0;
uint32_t rewardUntil = 0;

static void finishActivity(uint32_t now, uint8_t hearts = 1) {
  PetSave& s = pet();
  s.happy += hearts;
  if (s.happy > PET_MAX) s.happy = PET_MAX;
  lastHappyMs = now;

  uint16_t missing = ~s.stickers & STICKER_ALL;
  if (missing && esp_random() % STICKER_ODDS == 0) {
    uint8_t k = esp_random() % __builtin_popcount(missing);
    for (rewardSticker = 0; ; rewardSticker++)
      if ((missing & (1u << rewardSticker)) && k-- == 0) break;
    s.stickers |= 1u << rewardSticker;
    petSave();
    mode = M_REWARD;
    clearScreen(false);
    static const uint16_t PRIZE[] = { 1319, 1568, 2093, 2637 };
    jingle(PRIZE, 4, 90);
    rewardUntil = millis() + 3500;
    lastInteractMs = millis();
    return;
  }

  petSave();
  static const uint16_t TUNE[] = { 1319, 1568, 2093 };
  jingle(TUNE, 3, 90);
  enterHome();
  triggerOneShot(P_CELEBRATE, 3500);
}

static void tickReward(uint32_t now) {
  lastInteractMs = now;
  if (!before(now, rewardUntil)) {
    enterHome();
    // The last sticker gets a proper party.
    triggerOneShot(P_CELEBRATE, pet().stickers == STICKER_ALL ? 8000 : 2000);
    return;
  }
  buddyTick(P_CELEBRATE);
  clearBelow(BOTTOM_TOP);
  drawFrame(CX - 24, 178, 48, 48, 8);
  drawSticker(rewardSticker, CX, 202);
  bool tw = (now / 200) & 1;
  drawStar(20, 202, tw ? 6 : 4, YEL);
  drawStar(W - 20, 202, tw ? 4 : 6, YEL);
}

// --- Egg --------------------------------------------------------------------
const uint8_t EGG_TAPS = 7;
uint8_t  eggTaps = 0;
uint32_t eggWobbleUntil = 0;

static void enterEgg() {
  mode = M_EGG;
  eggTaps = 0;
  lastInteractMs = millis();
}

static void hatch() {
  PetSave& s = pet();
  // The parent screen can name the animal. Otherwise it is any of them,
  // except the one that was here before the egg.
  uint8_t n = buddySpeciesCount(), next = s.pick ? s.pick - 1 : esp_random() % n;
  if (!s.pick && n > 1 && next == s.species) next = (next + 1 + esp_random() % (n - 1)) % n;
  s.species = next;
  s.hatched = true;
  s.hunger = 2;
  s.happy = 2;
  lastHungerMs = lastHappyMs = millis();   // the egg's time is not the pet's
  petSave();
  buddySetSpeciesIdx(s.species);
  spr.fillSprite(INK);
  spr.pushSprite(0, 0);
  static const uint16_t TUNE[] = { 1047, 1319, 1568, 2093 };
  jingle(TUNE, 4, 90);
  enterHome();
  triggerOneShot(P_CELEBRATE, 4000);
}

static void tickEgg(uint32_t now, bool a, bool b, bool shook) {
  if (a || b || shook) {
    eggTaps++;
    eggWobbleUntil = now + 350;
    beep(500 + eggTaps * 150, 50);
    if (eggTaps >= EGG_TAPS) { eggTaps = 0; hatch(); return; }
  }

  static const char* const CRACK[4] = {
    "|       |", "|   /\\  |", "| /\\/\\  |", "|/\\/\\/\\ |"
  };
  uint8_t lvl = eggTaps / 2; if (lvl > 3) lvl = 3;
  const char* rows[6] = {
    "  .---.  ", " /     \\ ", "|       |", CRACK[lvl], " \\     / ", "  `---'  "
  };
  // Wobble hard after a tap, and give a small twitch every few seconds so
  // the egg looks alive while it waits.
  int xOff = 0;
  if (before(now, eggWobbleUntil)) xOff = ((now / 60) & 1) ? 4 : -4;
  else if ((now % 3000) < 240)             xOff = ((now / 80) & 1) ? 2 : -2;

  spr.fillSprite(BG);
  spr.setTextSize(2);
  spr.setTextColor(SHELL, BG);
  spr.setTextDatum(TC_DATUM);
  for (int i = 0; i < 6; i++) spr.drawString(rows[i], CX + xOff, 50 + i * 16);
  spr.setTextDatum(TL_DATUM);
  spr.setTextSize(1);

  // Pulsing blue dot = "press the blue button".
  spr.fillCircle(CX, 205, 10 + ((now / 400) & 1) * 3, BTN_BLUE);
}

// --- Feed -------------------------------------------------------------------
// The apple on the home row opens a tray of four foods. Every food fills one
// apple; the only difference is the reaction. One of them is today's
// favourite (picked fresh each wake-up) and gets the full celebration.
uint8_t  foodSel = 0, feedFood = 0, favFood = 0;
uint32_t feedStartMs = 0;
bool     feedCredited = false;
const uint32_t FEED_DROP_MS = 1200, FEED_HAPPY_MS = 2500;

static void startFood() {
  mode = M_FOOD;
  foodSel = pet().food < FOOD_N ? pet().food : 0;   // open on the last one picked
}

static void startFeed(uint32_t now, uint8_t food) {
  mode = M_FEED;
  feedFood = food;
  feedStartMs = now;
  feedCredited = false;
}

static void tickFood(uint32_t now, bool a, bool b) {
  if (b) { foodSel = (foodSel + 1) % FOOD_N; beep(1800, 25); }
  if (a) { beep(2400, 25); startFeed(now, foodSel); return; }

  buddyTick(P_IDLE);
  clearBelow(BOTTOM_TOP);
  drawNeeds(false);
  spr.drawFastHLine(0, 190, W, DIM);
  for (int i = 0; i < FOOD_N; i++) {
    int x = 21 + i * 31, y = 216;
    drawFood(i, x, y);
    if (i == foodSel) drawFrame(x - 15, y - 17, 30, 34, 6);
  }
}

static void tickFeed(uint32_t now) {
  uint32_t e = now - feedStartMs;
  lastInteractMs = now;
  bool fav = (feedFood == favFood);
  if (e < FEED_DROP_MS) {
    // The strip only clears on the 5 fps animation tick; force a redraw
    // every frame so the falling snack doesn't leave a trail.
    buddyInvalidate();
    buddyTick(P_IDLE);
    drawFood(feedFood, CX, 8 + (int)(e * 62 / FEED_DROP_MS));
  } else {
    if (!feedCredited) {
      feedCredited = true;
      if (pet().hunger < PET_MAX) pet().hunger++;
      pet().food = feedFood;
      lastHungerMs = now;
      petSave();
      static const uint16_t MUNCH[] = { 900, 700, 900 };
      static const uint16_t YUM[]   = { 900, 1175, 1568, 2093 };
      if (fav) jingle(YUM, 4, 70);
      else     jingle(MUNCH, 3, 50);
      buddyInvalidate();
    }
    buddyTick(fav ? P_CELEBRATE : P_HEART);
    if (e > FEED_DROP_MS + FEED_HAPPY_MS) { enterHome(); return; }
  }
  drawBottom(false);
}

// --- Bed --------------------------------------------------------------------
uint32_t bedStartMs = 0;

static void startBed(uint32_t now, bool lullaby) {
  mode = M_BED;
  bedStartMs = now;
  clearScreen(false);
  if (lullaby) {
    static const uint16_t TUNE[] = { 1047, 880, 698 };
    jingle(TUNE, 3, 140);
  }
  M5.Display.setBrightness(BED_BRIGHT);
}

static void tickBed(uint32_t now) {
  if (M5.BtnA.isPressed() || M5.BtnB.isPressed()) {
    swallowA = M5.BtnA.isPressed();
    swallowB = M5.BtnB.isPressed();
    enterHome();
    return;
  }
  buddyTick(P_SLEEP);
  if (!pet().stayOn && now - bedStartMs > BED_OFF_MS) napDown();
}

// --- Home -------------------------------------------------------------------
static PetState homeMood(uint32_t now) {
  const PetSave& s = pet();
  if (s.hunger == 0 || s.happy == 0) return P_ATTENTION;
  if (s.hunger == PET_MAX && s.happy == PET_MAX)
    return (now / 6000 % 3 == 0) ? P_HEART : P_IDLE;
  return P_IDLE;
}

static void tickHome(uint32_t now, bool a, bool b, bool shook) {
  if (b) { iconSel = (iconSel + 1) % ICON_N; beep(1800, 25); }
  if (a) {
    beep(2400, 25);
    if (iconSel == ICON_FEED)      { startFood(); return; }
    else if (iconSel == ICON_DANCE) { startDance(now); return; }
    else                           { startBed(now, true); return; }
  }
  if (shook && !oneShotActive()) {
    lastInteractMs = now;
    triggerOneShot(P_DIZZY, 2000);
  }

  // Needs drift down slowly; floor at zero, nothing worse ever happens.
  PetSave& s = pet();
  if (now - lastHungerMs >= HUNGER_DECAY_MS) { lastHungerMs = now; if (s.hunger) s.hunger--; }
  if (now - lastHappyMs  >= HAPPY_DECAY_MS)  { lastHappyMs  = now; if (s.happy)  s.happy--;  }

  buddyTick(oneShotActive() ? oneShotState : homeMood(now));
  drawBottom(true);

  // Face-down tucks it in early. Needs sustained face-down so IMU noise at
  // the threshold doesn't trigger it.
  static int8_t faceDownFrames = 0;
  if (isFaceDown()) { if (faceDownFrames < 20) faceDownFrames++; }
  else              { faceDownFrames = 0; }
  if (faceDownFrames >= 15) { faceDownFrames = 0; startBed(now, false); return; }

  if (!s.stayOn && idleFor(now, IDLE_BED_MS)) startBed(now, false);
}
