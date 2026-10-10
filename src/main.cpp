#include "compat.h"
#include "buddy.h"
#include "pet.h"
#if defined(BOARD_STICKS3)
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#endif

// Offline pocket pet. No radio, no host: an egg hatches into one of the
// ASCII species, and the kid feeds it, plays a tilt game, and puts it to
// bed. Nothing can die — needs bottom out at "asking for it" and stop.
//
//   BtnB (side)        next icon
//   BtnA (front, blue) select
//   hold side 1.5s     sound on/off
//   hold both 1.5s     parent screen (text; brightness, battery test, reset)
//   Power (side)       turns the unit back on after it has put itself to bed

TFT_eSprite spr = TFT_eSprite(&M5.Lcd);

const int W = 135, H = 240;
const int CX = W / 2;

const uint16_t BG    = 0x0000;
const uint16_t INK   = 0xFFFF;
const uint16_t DIM   = 0x8410;
const uint16_t HOT   = 0xFA20;   // red-orange: warnings
const uint16_t BTN_BLUE = 0x041F;   // matches the front button
const uint16_t YEL   = 0xFFE0;
const uint16_t AQUA  = 0x07FF;
const uint16_t SHELL = 0xFFF4;   // egg

enum PetState { P_SLEEP, P_IDLE, P_BUSY, P_ATTENTION, P_CELEBRATE, P_DIZZY, P_HEART };
enum Mode     { M_EGG, M_HOME, M_FEED, M_PLAY, M_BED, M_PARENT };

Mode     mode = M_EGG;
uint32_t lastInteractMs = 0;
bool     swallowA = false, swallowB = false;

// --- Battery budget ---------------------------------------------------------
// The backlight and the CPU are the only real loads once the radio is gone.
// Idle → the pet falls asleep on a dimmed screen → the unit powers itself
// off. Waking is the side power button.
const uint32_t IDLE_BED_MS = 60000;   // no input this long → bedtime
const uint32_t BED_OFF_MS  = 15000;   // asleep this long → power off
const uint8_t  BED_BRIGHT  = 12;

// --- Needs ------------------------------------------------------------------
// Only tick while the unit is on: there is no clock while it's off, so a
// pet left in a drawer comes back exactly as it was, minus one pip each for
// "woke up hungry".
const uint32_t HUNGER_DECAY_MS = 8UL * 60 * 1000;
const uint32_t HAPPY_DECAY_MS  = 12UL * 60 * 1000;
uint32_t lastHungerMs = 0, lastHappyMs = 0;

PetState oneShotState = P_IDLE;
uint32_t oneShotUntil = 0;

static void triggerOneShot(PetState s, uint32_t durMs) {
  oneShotState = s;
  oneShotUntil = millis() + durMs;
}
static bool oneShotActive() { return (int32_t)(millis() - oneShotUntil) < 0; }

static void applyBrightness() { M5.Display.setBrightness((pet().bright + 1) * 51); }

static void beep(uint16_t freq, uint16_t dur) {
  if (pet().sound) M5.Speaker.tone(freq, dur);
}

// Short blocking tune for one-off moments (hatch, win, bedtime).
static void jingle(const uint16_t* notes, uint8_t n, uint16_t each) {
  if (!pet().sound) return;
  for (uint8_t i = 0; i < n; i++) { M5.Speaker.tone(notes[i], each); delay(each + 15); }
}

// --- IMU --------------------------------------------------------------------
float accelBaseline = 1.0f;
uint32_t lastShakeCheck = 0;

static bool checkShake() {
  float ax, ay, az;
  M5.Imu.getAccel(&ax, &ay, &az);
  float mag = sqrtf(ax*ax + ay*ay + az*az);
  float delta = fabsf(mag - accelBaseline);
  accelBaseline = accelBaseline * 0.95f + mag * 0.05f;
  return delta > 0.8f;
}

// Face-down = Z-axis dominant and negative. Debounced so a toss doesn't count.
static bool isFaceDown() {
  float ax, ay, az;
  M5.Imu.getAccel(&ax, &ay, &az);
  return az < -0.7f && fabsf(ax) < 0.4f && fabsf(ay) < 0.4f;
}

// --- Power ------------------------------------------------------------------
// Awake minutes persist every 5 min so a run-to-empty battery test still
// leaves a usable number behind (worst case it under-reports by 5 min).
uint32_t awakeMarkMs = 0;
uint8_t  awakeUnsaved = 0;
int      batMv = 0;
uint32_t lastBatMs = 0;

static int batPct() {
  int pct = (batMv - 3200) / 10;
  return pct < 0 ? 0 : pct > 100 ? 100 : pct;
}

static void powerTick(uint32_t now) {
  if (now - awakeMarkMs >= 60000) {
    awakeMarkMs += 60000;
    pet().awakeMin++;
    if (++awakeUnsaved >= 5) { petSave(); awakeUnsaved = 0; }
  }
  if (lastBatMs == 0 || now - lastBatMs >= 10000) {
    lastBatMs = now;
    batMv = M5.Power.getBatteryVoltage();
    Serial.printf("bat=%dmV awake=%lum\n", batMv, (unsigned long)pet().awakeMin);
  }
}

static void powerDown() {
  petSave();
  M5.Display.sleep();
#if defined(BOARD_STICKS3)
  // On USB the PMIC can't cut the rail, so powerOff() lands in deep sleep
  // instead. Arm the front button (GPIO11) so that case is still wakeable.
  rtc_gpio_pullup_en(GPIO_NUM_11);
  rtc_gpio_pulldown_dis(GPIO_NUM_11);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_11, 0);
#endif
  M5.Power.powerOff();
}

// --- Icons ------------------------------------------------------------------
static void drawApple(int x, int y, int r, bool filled = true) {
  if (filled) {
    spr.fillCircle(x, y, r, RED);
    spr.drawLine(x, y - r, x + 1, y - r - r / 2 - 1, DIM);
    if (r >= 6) spr.fillTriangle(x + 1, y - r - 1, x + r - 1, y - r - 3, x + r / 2, y - r + 1, GREEN);
  } else {
    spr.drawCircle(x, y, r, DIM);
  }
}

static void tinyHeart(int x, int y, bool filled, uint16_t col) {
  if (filled) {
    spr.fillCircle(x - 2, y, 2, col);
    spr.fillCircle(x + 2, y, 2, col);
    spr.fillTriangle(x - 4, y + 1, x + 4, y + 1, x, y + 5, col);
  } else {
    spr.drawCircle(x - 2, y, 2, col);
    spr.drawCircle(x + 2, y, 2, col);
    spr.drawLine(x - 4, y + 1, x, y + 5, col);
    spr.drawLine(x + 4, y + 1, x, y + 5, col);
  }
}

static void drawBall(int x, int y, int r) {
  spr.fillCircle(x, y, r, AQUA);
  spr.drawLine(x - r, y, x + r, y, INK);
  spr.drawLine(x, y - r, x, y + r, INK);
  spr.drawCircle(x, y, r, INK);
}

static void drawMoon(int x, int y, int r) {
  spr.fillCircle(x - 2, y, r, YEL);
  spr.fillCircle(x + 2, y - 3, r - 1, BG);
  spr.setTextSize(1);
  spr.setTextColor(INK, BG);
  spr.setCursor(x + 5, y - 2);
  spr.print("z");
}

// Speaker glyph shown for a moment after the hold-side mute toggle.
uint32_t soundFlashUntil = 0;
static void drawSoundFlash() {
  uint16_t c = pet().sound ? INK : HOT;
  spr.fillRect(2, 2, 22, 14, BG);
  spr.fillRect(4, 7, 4, 5, c);
  spr.fillTriangle(8, 7, 8, 11, 13, 4, c);
  spr.fillTriangle(8, 11, 13, 4, 13, 14, c);
  if (pet().sound) { spr.drawLine(16, 6, 16, 12, c); spr.drawLine(19, 4, 19, 14, c); }
  else             { spr.drawLine(16, 5, 22, 13, c); spr.drawLine(22, 5, 16, 13, c); }
}

// --- Home screen ------------------------------------------------------------
enum { ICON_FEED, ICON_PLAY, ICON_BED, ICON_N };
uint8_t iconSel = 0;
const int BOTTOM_TOP = 166;      // buddy strip at 2× clears y < 164

static void drawNeeds() {
  for (int i = 0; i < PET_MAX; i++) {
    int x = 34 + i * 22;
    drawApple(x, 177, 4, i < pet().hunger);
    tinyHeart(x, 190, i < pet().happy, i < pet().happy ? RED : DIM);
  }
}

static void drawBottom(bool showSel) {
  spr.fillRect(0, BOTTOM_TOP, W, H - BOTTOM_TOP, BG);
  drawNeeds();
  spr.drawFastHLine(0, 201, W, DIM);
  for (int i = 0; i < ICON_N; i++) {
    int cx = 22 + i * 45, cy = 222;
    if (i == ICON_FEED)      drawApple(cx, cy + 2, 9);
    else if (i == ICON_PLAY) drawBall(cx, cy, 9);
    else                     drawMoon(cx, cy, 9);
    if (showSel && i == iconSel) {
      spr.drawRoundRect(cx - 20, 204, 40, 35, 6, INK);
      spr.drawRoundRect(cx - 19, 205, 38, 33, 5, INK);
    }
  }
  if (batMv > 0 && batPct() < 15) {            // low battery: tell a grown-up
    spr.drawRect(W - 18, 3, 14, 8, HOT);
    spr.fillRect(W - 4, 5, 2, 4, HOT);
    spr.fillRect(W - 16, 5, 3, 4, HOT);
  }
}

static void enterHome() {
  mode = M_HOME;
  buddySetPeek(false);
  spr.fillSprite(BG);
  buddyInvalidate();
  applyBrightness();
  lastInteractMs = millis();
}

// --- Egg --------------------------------------------------------------------
const uint8_t EGG_TAPS = 7;
uint8_t  eggTaps = 0;
uint32_t eggWobbleUntil = 0;

static void hatch() {
  PetSave& s = pet();
  // Every egg is an octopus while testing. The parent screen can still
  // switch species; picking one at hatch comes later.
  buddySetSpecies("octopus");
  s.species = buddySpeciesIdx();
  s.hatched = true;
  s.hunger = 2;
  s.happy = 2;
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
  if ((int32_t)(now - eggWobbleUntil) < 0) xOff = ((now / 60) & 1) ? 4 : -4;
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
uint32_t feedStartMs = 0;
bool     feedCredited = false;
const uint32_t FEED_DROP_MS = 1200, FEED_HAPPY_MS = 2500;

static void startFeed(uint32_t now) {
  mode = M_FEED;
  feedStartMs = now;
  feedCredited = false;
}

static void tickFeed(uint32_t now) {
  uint32_t e = now - feedStartMs;
  lastInteractMs = now;
  if (e < FEED_DROP_MS) {
    // The strip only clears on the 5 fps animation tick; force a redraw
    // every frame so the falling snack doesn't leave a trail.
    buddyInvalidate();
    buddyTick(P_IDLE);
    drawApple(CX, 8 + (int)(e * 62 / FEED_DROP_MS), 7);
  } else {
    if (!feedCredited) {
      feedCredited = true;
      if (pet().hunger < PET_MAX) pet().hunger++;
      lastHungerMs = now;
      petSave();
      static const uint16_t MUNCH[] = { 900, 700, 900 };
      jingle(MUNCH, 3, 50);
      buddyInvalidate();
    }
    buddyTick(P_HEART);
    if (e > FEED_DROP_MS + FEED_HAPPY_MS) { enterHome(); return; }
  }
  drawBottom(false);
}

// --- Play: tilt to catch ----------------------------------------------------
// Snacks fall one at a time; tilt the stick left/right to slide the basket
// under them. No way to lose — every round ends in a celebration.
const float   TILT_SIGN   = 1.0f;     // flip to -1 if the basket moves backwards
const uint8_t PLAY_ROUNDS = 5;
const int     FIELD_TOP   = 86, BASKET_Y = 214, CATCH_HALF = 30;
const float   FALL_PX_MS  = 0.055f;
uint8_t  playRound = 0, playCaught = 0, playHits = 0;
float    basketX = CX;
int      dropX = CX;
uint32_t dropStartMs = 0, playCheerUntil = 0;

static void spawnDrop(uint32_t now) {
  dropX = 16 + esp_random() % (W - 32);
  dropStartMs = now;
}

static void startPlay(uint32_t now) {
  mode = M_PLAY;
  playRound = playCaught = playHits = 0;
  basketX = CX;
  buddySetPeek(true);
  spr.fillSprite(BG);
  spawnDrop(now);
}

static void endPlay(uint32_t now) {
  PetSave& s = pet();
  s.happy += (playCaught >= 3) ? 2 : 1;
  if (s.happy > PET_MAX) s.happy = PET_MAX;
  lastHappyMs = now;
  petSave();
  static const uint16_t TUNE[] = { 1319, 1568, 2093 };
  jingle(TUNE, 3, 90);
  enterHome();
  triggerOneShot(P_CELEBRATE, 3500);
}

static void tickPlay(uint32_t now) {
  lastInteractMs = now;

  float ax, ay, az;
  M5.Imu.getAccel(&ax, &ay, &az);
  float target = CX + TILT_SIGN * ax * 110.0f;
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

  buddyTick((int32_t)(now - playCheerUntil) < 0 ? P_HEART : P_IDLE);

  spr.fillRect(0, FIELD_TOP, W, H - FIELD_TOP, BG);
  drawApple(dropX, dropY, 7);
  spr.setTextSize(2);
  spr.setTextColor(INK, BG);
  spr.setTextDatum(TC_DATUM);
  spr.drawString("\\___/", (int)basketX, BASKET_Y);
  spr.setTextDatum(TL_DATUM);
  spr.setTextSize(1);
  for (int i = 0; i < PLAY_ROUNDS; i++) {
    int x = CX - (PLAY_ROUNDS - 1) * 8 + i * 16;
    if (i < playRound) drawApple(x, 234, 3, playHits & (1 << i));
    else               spr.drawPixel(x, 234, DIM);
  }
}

// --- Bed --------------------------------------------------------------------
uint32_t bedStartMs = 0;

static void startBed(uint32_t now, bool lullaby) {
  mode = M_BED;
  bedStartMs = now;
  spr.fillSprite(BG);
  buddyInvalidate();
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
  if (!pet().stayOn && now - bedStartMs > BED_OFF_MS) powerDown();
}

// --- Parent screen ----------------------------------------------------------
enum { PA_BRIGHT, PA_SOUND, PA_STAYON, PA_PET, PA_ZERO, PA_EGG, PA_BACK, PA_N };
const char* const PARENT_ITEMS[PA_N] = {
  "bright", "sound", "stay on", "pet", "zero timer", "new egg", "back"
};
uint8_t  parentSel = 0;
uint32_t eggConfirmUntil = 0;

static void leaveParent() {
  if (pet().hatched) enterHome();
  else { mode = M_EGG; eggTaps = 0; lastInteractMs = millis(); }
}

static void applyParent(uint8_t idx, uint32_t now) {
  PetSave& s = pet();
  switch (idx) {
    case PA_BRIGHT: s.bright = (s.bright + 1) % 5; applyBrightness(); break;
    case PA_SOUND:  s.sound = !s.sound; break;
    case PA_STAYON: s.stayOn = !s.stayOn; break;
    case PA_PET:
      buddyNextSpecies();
      s.species = buddySpeciesIdx();
      break;
    case PA_ZERO:   s.awakeMin = 0; awakeMarkMs = now; break;
    case PA_EGG:
      // Tap-twice confirm: first tap arms (label flips to "really?").
      if ((int32_t)(now - eggConfirmUntil) >= 0) { eggConfirmUntil = now + 3000; return; }
      eggConfirmUntil = 0;
      petReset();
      leaveParent();
      return;
    case PA_BACK:   petSave(); leaveParent(); return;
  }
  petSave();
}

static void tickParent(uint32_t now, bool a, bool b) {
  lastInteractMs = now;
  if (b) { parentSel = (parentSel + 1) % PA_N; eggConfirmUntil = 0; beep(1800, 25); }
  if (a) { beep(2400, 25); applyParent(parentSel, now); if (mode != M_PARENT) return; }

  PetSave& s = pet();
  spr.fillSprite(BG);
  spr.setTextSize(1);
  spr.setTextColor(INK, BG);
  spr.setCursor(6, 8);  spr.print("PARENT");
  spr.setTextColor(DIM, BG);
  spr.setCursor(6, 24); spr.printf("battery %d.%02dV %d%%", batMv / 1000, (batMv % 1000) / 10, batPct());
  spr.setCursor(6, 36); spr.printf("awake   %luh %02lum",
                                   (unsigned long)(s.awakeMin / 60), (unsigned long)(s.awakeMin % 60));
  for (int i = 0; i < PA_N; i++) {
    int y = 60 + i * 16;
    bool sel = (i == parentSel);
    bool armed = (i == PA_EGG) && (int32_t)(now - eggConfirmUntil) < 0;
    spr.setTextColor(armed ? HOT : sel ? INK : DIM, BG);
    spr.setCursor(6, y);
    spr.print(sel ? "> " : "  ");
    spr.print(armed ? "really?" : PARENT_ITEMS[i]);
    spr.setTextColor(DIM, BG);
    spr.setCursor(84, y);
    if (i == PA_BRIGHT)      spr.printf("%u/4", s.bright);
    else if (i == PA_SOUND)  spr.print(s.sound ? "on" : "off");
    else if (i == PA_STAYON) spr.print(s.stayOn ? "on" : "off");
    else if (i == PA_PET)    spr.print(buddySpeciesName());
  }
  spr.setTextColor(DIM, BG);
  spr.setCursor(6, H - 24); spr.print("side: next");
  spr.setCursor(6, H - 12); spr.print("blue: change");
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
    if (iconSel == ICON_FEED)      { startFeed(now); return; }
    else if (iconSel == ICON_PLAY) { startPlay(now); return; }
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

  if (!s.stayOn && now - lastInteractMs > IDLE_BED_MS) startBed(now, false);
}

void setup() {
  // USB-CDC (native USB on ESP32-S3) blocks on write when no serial monitor
  // is attached — the TX buffer fills and Serial.print() waits forever for a
  // host, hanging setup()/loop(). Make it non-blocking so the device runs
  // standalone. The StickC Plus has a real UART and no such call.
#if defined(BOARD_STICKS3)
  Serial.setTxTimeoutMs(0);
#endif
  M5.begin();
  // No radio and 5 fps animation: 80 MHz is plenty and roughly halves the
  // CPU's share of the battery. (Lowest speed that keeps USB alive.)
  setCpuFrequencyMhz(80);
  M5.Lcd.setRotation(0);
  M5.Speaker.setVolume(90);      // IMU auto-inits in M5.begin()
  petLoad();
  applyBrightness();
  spr.createSprite(W, H);
  buddyInit();
  buddySetSpeciesIdx(pet().species);

  uint32_t now = millis();
  awakeMarkMs = lastHungerMs = lastHappyMs = lastInteractMs = now;
  if (pet().hatched) {
    // Woke up a little hungry and a little bored: always something to do.
    if (pet().hunger) pet().hunger--;
    if (pet().happy)  pet().happy--;
    enterHome();
    triggerOneShot(P_SLEEP, 2500);
  } else {
    mode = M_EGG;
  }
}

void loop() {
  M5.update();
  uint32_t now = millis();
  powerTick(now);

  if (M5.BtnA.isPressed() || M5.BtnB.isPressed()) lastInteractMs = now;

  // Actions fire on release so the both-buttons hold can be told apart from
  // two ordinary presses.
  bool a = M5.BtnA.wasReleased(), b = M5.BtnB.wasReleased();
  if (a && swallowA) { a = false; swallowA = false; }
  if (b && swallowB) { b = false; swallowB = false; }

  if (mode != M_PARENT && M5.BtnA.pressedFor(1500) && M5.BtnB.pressedFor(1500)) {
    swallowA = swallowB = true;
    a = b = false;
    buddySetPeek(false);
    applyBrightness();
    parentSel = 0;
    mode = M_PARENT;
    beep(800, 80);
  }

  // Hold the side button alone: mute/unmute. The chirp plays only when
  // sound has just come back on; the glyph covers the silent direction.
  if (mode != M_PARENT && !swallowB && M5.BtnB.pressedFor(1500) && !M5.BtnA.isPressed()) {
    swallowB = true;
    b = false;
    pet().sound = !pet().sound;
    petSave();
    beep(1800, 80);
    soundFlashUntil = now + 1500;
  }

  bool shook = false;
  if ((mode == M_HOME || mode == M_EGG) && now - lastShakeCheck > 50) {
    lastShakeCheck = now;
    shook = checkShake();
  }

  switch (mode) {
    case M_EGG:
      tickEgg(now, a, b, shook);
      if (mode == M_EGG && !pet().stayOn && now - lastInteractMs > IDLE_BED_MS + BED_OFF_MS) powerDown();
      break;
    case M_HOME:   tickHome(now, a, b, shook); break;
    case M_FEED:   tickFeed(now); break;
    case M_PLAY:   tickPlay(now); break;
    case M_BED:    tickBed(now); break;
    case M_PARENT: tickParent(now, a, b); break;
  }

  if (mode != M_PARENT && (int32_t)(now - soundFlashUntil) < 0) drawSoundFlash();
  spr.pushSprite(0, 0);
  delay(mode == M_BED ? 100 : 33);
}
