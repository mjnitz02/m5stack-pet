#include "compat.h"
#include "buddy.h"
#include "pet.h"
#if defined(BOARD_STICKS3)
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#endif

// Offline pocket pet. No radio, no host: an egg hatches into one of the
// ASCII species, and the kid feeds it, plays with it, and puts it to bed.
// Nothing can die — needs bottom out at "asking for it" and stop.
//
//   BtnB (side)        next icon
//   BtnA (front, blue) select
//   hold blue 0.7s     toy box from the pet screen; back to the pet from
//                      anywhere else
//   hold side 1.5s     sound on/off
//   hold both 1.5s     parent screen (text; brightness, battery test, reset)
// A new or reset stick opens on a short lesson in those buttons, then the egg.
//   blue button        wakes it from a nap (the first minutes after bedtime)
//   Power (side)       turns the unit back on once it has powered off
//
// main.cpp holds the core loop and screens; icons.h has every drawing and
// activities.h has the toy box activities.

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
const uint16_t AMBER = 0xFD20;
const uint16_t ROSE  = 0xFB76;
const uint16_t TAN   = 0xE58E;
const uint16_t GRID  = 0x39E7;   // unlit outlines

enum PetState { P_SLEEP, P_IDLE, P_BUSY, P_ATTENTION, P_CELEBRATE, P_DIZZY, P_HEART };
// Everything from M_WALK on is a toy box activity (see activities.h).
enum Mode     { M_EGG, M_HOME, M_FOOD, M_FEED, M_PLAY, M_BED, M_PARENT, M_MENU, M_REWARD, M_CAL, M_LEARN,
                M_WALK, M_BATH, M_ROCK, M_FREEZE, M_DRESS, M_STICKERS,
                M_DRAW, M_MARBLE, M_COPY, M_HIDE, M_DANCE };
#if defined(PET_LOG)
const char* const MODE_NAME[] = {
  "egg", "home", "food", "feed", "play", "bed", "parent", "menu", "reward", "cal", "learn",
  "walk", "bath", "rock", "freeze", "dress", "stickers",
  "draw", "marble", "copy", "hide", "dance" };
static_assert(sizeof(MODE_NAME) / sizeof(MODE_NAME[0]) == M_DANCE + 1, "MODE_NAME is out of step with Mode");
#endif

// Toy box tiles in reading order. The first page is care and moving, the
// second is the sit-still games. The egg stays last so it can be hidden.
enum { T_WALK, T_BATH, T_ROCK, T_FREEZE, T_DRESS, T_STICKERS,
       T_CATCH, T_DRAW, T_MARBLE, T_COPY, T_HIDE, T_EGG, T_N };

// Mostly healthy, one treat. Each fills one apple on the hunger row.
enum { FOOD_APPLE, FOOD_CARROT, FOOD_STRAWBERRY, FOOD_ICECREAM, FOOD_N };

Mode     mode = M_EGG;
uint32_t lastInteractMs = 0;
bool     swallowA = false, swallowB = false;

// --- Battery budget ---------------------------------------------------------
// The backlight and the CPU are the only real loads once the radio is gone.
// Idle → the pet falls asleep on a dimmed screen → the screen goes off and
// the unit naps in deep sleep, where the blue button brings it straight
// back → the unit powers itself off. After that, waking is the side power
// button.
const uint32_t IDLE_BED_MS = 60000;   // no input this long → bedtime
const uint32_t BED_OFF_MS  = 15000;   // asleep this long → screen off, nap
#if defined(PET_LOG)
// Debug build: a nap long enough for its drain to show in the log overnight.
const uint32_t NAP_MS      = 12UL * 60 * 60 * 1000;
#else
const uint32_t NAP_MS      = 60UL * 60 * 1000;   // napping this long → power off
#endif
const uint8_t  BED_BRIGHT  = 12;
const uint32_t MENU_IDLE_MS = 15000;  // toy box / food tray left alone → back to the pet
const uint32_t HOLD_BLUE_MS = 700;    // blue held this long → toy box, or back to the pet

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
// One motion sample every 50 ms, shared by everything that cares about
// movement: how far the pull on the stick is from resting 1 g.
float    accelBaseline = 1.0f;
uint32_t lastImuMs = 0;
float    imuDelta = 0;
float    imuTurn = 0;           // how far the pull has swung round, see below
float    turnX = 0, turnY = 0, turnZ = 0;
bool     imuFresh = false;      // true on the frame a sample was taken
const float SHAKE_DELTA = 0.8f;

static void sampleMotion(uint32_t now) {
  imuFresh = false;
  if (now - lastImuMs < 50) return;
  lastImuMs = now;
  float ax, ay, az;
  M5.Imu.getAccel(&ax, &ay, &az);
  float mag = sqrtf(ax*ax + ay*ay + az*az);
  imuDelta = fabsf(mag - accelBaseline);
  accelBaseline = accelBaseline * 0.95f + mag * 0.05f;
  // The same sample as a direction. Tipping or swaying the stick barely
  // changes how hard gravity pulls on it, but it does change which way, so
  // this is the one that notices gentle movement.
  float dx = ax - turnX, dy = ay - turnY, dz = az - turnZ;
  imuTurn = sqrtf(dx*dx + dy*dy + dz*dz);
  turnX += dx * 0.2f; turnY += dy * 0.2f; turnZ += dz * 0.2f;
  imuFresh = true;
}

// Tilt steering. Nobody holds the stick level, so "level" is whatever way
// gravity points when a tilt activity starts: tiltBegin() spends the first
// half second averaging it. tiltX and tiltY are then how far gravity has
// leaned from there, along the screen's own left/right and up/down, at full
// strength however the stick was held.
//
// TILT_DEFAULT is what a real calibration on a StickS3 read, in an adult
// hand, and is used until a grown-up runs their own from the parent screen:
//   held to play   -0.55  0.00  0.84   top tipped up about 35 degrees
//   tilted left    -0.54  0.63  0.55   so screen-right is the sensor's -y
//   tilted right   -0.56 -0.57  0.61
//   tilted up       0.05  0.01  1.00   flat on its back
//   tilted down    -0.98  0.01  0.14   so screen-down is the sensor's -x
// A deliberate tilt came out at 0.6 to 0.7 g each way (roughly 40 degrees),
// about twice what the games count as a full tilt, hence the gain.
const TiltCal  TILT_DEFAULT = { false, { 0, -1, 0 }, { -1, 0, 0 }, 0.55f, 0.55f };
const uint32_t TILT_ZERO_MS = 500;
float    tiltX = 0, tiltY = 0, tiltSeenX = 0, tiltSeenY = 0;
float    tiltSum[3] = { 0, 0, 0 };
float    tiltU[3] = { 0, -1, 0 }, tiltV[3] = { -1, 0, 0 };   // screen x and y, laid level
float    tiltGainX = 1, tiltGainY = 1;
uint16_t tiltN = 0;
uint32_t tiltZeroUntil = 0;

static void tiltBegin(uint32_t now) {
  tiltSum[0] = tiltSum[1] = tiltSum[2] = 0;
  tiltN = 0;
  tiltX = tiltY = tiltSeenX = tiltSeenY = 0;
  tiltZeroUntil = now + TILT_ZERO_MS;
}

// The part of d that lies level (square to the unit pull g), at unit length.
// False if d points too nearly at the floor to say.
static bool tiltFlatten(const float* d, const float* g, float* out) {
  float k = d[0]*g[0] + d[1]*g[1] + d[2]*g[2];
  for (int i = 0; i < 3; i++) out[i] = d[i] - k * g[i];
  float len = sqrtf(out[0]*out[0] + out[1]*out[1] + out[2]*out[2]);
  if (len < 0.3f) return false;
  for (int i = 0; i < 3; i++) out[i] /= len;
  return true;
}

static void tiltCross(const float* a, const float* b, float* out) {
  out[0] = a[1]*b[2] - a[2]*b[1];
  out[1] = a[2]*b[0] - a[0]*b[2];
  out[2] = a[0]*b[1] - a[1]*b[0];
}

// Turn the averaged pull into the two directions a tilt is measured along:
// the calibrated "right" and "down", laid level for today's grip.
static void tiltSetLevel() {
  float g[3] = { tiltSum[0], tiltSum[1], tiltSum[2] };
  float len = sqrtf(g[0]*g[0] + g[1]*g[1] + g[2]*g[2]);
  if (len < 0.01f) { g[0] = g[1] = 0; g[2] = 1; len = 1; }
  for (int i = 0; i < 3; i++) g[i] /= len;

  const TiltCal& cal = tiltCal().set ? tiltCal() : TILT_DEFAULT;
  tiltGainX = cal.gainX;
  tiltGainY = cal.gainY;
  bool okU = tiltFlatten(cal.u, g, tiltU), okV = tiltFlatten(cal.v, g, tiltV);
  // Held bolt upright, "down the screen" points at the floor and can't be
  // laid level; on its side the same goes for "right". Square the lost one
  // off the other, the way round the readings above come out.
  if (!okV) tiltCross(tiltU, g, tiltV);
  else if (!okU) tiltCross(g, tiltV, tiltU);
}

// False while level is still being captured.
static bool tiltUpdate(uint32_t now) {
  float a[3];
  M5.Imu.getAccel(&a[0], &a[1], &a[2]);
  if ((int32_t)(now - tiltZeroUntil) < 0) {
    for (int i = 0; i < 3; i++) tiltSum[i] += a[i];
    tiltN++;
    return false;
  }
  if (tiltN) { tiltSetLevel(); tiltN = 0; }
  float x = (a[0]*tiltU[0] + a[1]*tiltU[1] + a[2]*tiltU[2]) * tiltGainX;
  float y = (a[0]*tiltV[0] + a[1]*tiltV[1] + a[2]*tiltV[2]) * tiltGainY;
  tiltX += (x - tiltX) * 0.5f;
  tiltY += (y - tiltY) * 0.5f;
  // Steering counts as playing; a stick lying still does not, so it can
  // still fall asleep in the middle of a tilt activity.
  if (fabsf(tiltX - tiltSeenX) > 0.06f || fabsf(tiltY - tiltSeenY) > 0.06f) {
    tiltSeenX = tiltX; tiltSeenY = tiltY;
    lastInteractMs = now;
  }
  return true;
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

#include "petlog.h"

#if defined(BOARD_STICKS3)
// Deep sleep only stops the ESP32: every other chip on the board stays in
// whatever state it was left in, so each one is switched off by hand first.
// M5.begin() brings all of it back on the next boot, bar the light: see setup().
static void railsDown() {
  M5.Speaker.end();                                      // amp enable (PM1 GPIO3) low
  M5.In_I2C.writeRegister8(0x18, 0x12, 0x02, 100000);    // ES8311 codec: DAC off
  M5.In_I2C.writeRegister8(0x18, 0x13, 0x00, 100000);    //   output drive off
  M5.In_I2C.writeRegister8(0x18, 0x0D, 0xFC, 100000);    //   analog off
  M5.In_I2C.writeRegister8(0x18, 0x00, 0x00, 100000);    //   clocks off
  M5.In_I2C.writeRegister8(0x69, 0x7D, 0x00, 400000);    // BMI270: accel + gyro off
  M5.In_I2C.writeRegister8(0x69, 0x7C, 0x01, 400000);    //   suspend
  M5.Power.setExtOutput(false);                          // 5 V boost (Grove port)
  M5.Power.M5pm1.setGPIOOutput(m5::M5PM1_Class::gpio2, false);   // LCD rail
  M5.Power.M5pm1.setLedEnLevel(false);                   // green light by the power button
}
#endif

// The real off switch. Saves nothing, so a nap that has run out can use it
// without loading the pet first.
static void powerOffNow() {
  M5.Display.sleep();
#if defined(BOARD_STICKS3)
  railsDown();   // for the USB case below, where the PMIC stays up
  // On USB the PMIC can't cut the rail, so powerOff() lands in deep sleep
  // instead. Arm the front button (GPIO11) so that case is still wakeable.
  rtc_gpio_pullup_en(GPIO_NUM_11);
  rtc_gpio_pulldown_dis(GPIO_NUM_11);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_11, 0);
#endif
  M5.Power.powerOff();
}

// Screen off but not gone: deep sleep with the blue button armed, and a
// timer that wakes the chip just long enough to power off properly (see
// setup()). Waking is a reboot either way; the pet comes back from flash.
static void napDown() {
  petSave();
#if defined(BOARD_STICKS3)
  logRow("nap");
  M5.Display.sleep();
  railsDown();
  rtc_gpio_pullup_en(GPIO_NUM_11);
  rtc_gpio_pulldown_dis(GPIO_NUM_11);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_11, 0);
  esp_sleep_enable_timer_wakeup(NAP_MS * 1000ULL);
  esp_deep_sleep_start();
#else
  logRow("off");
  powerOffNow();
#endif
}

#include "icons.h"

// Battery at a glance for a grown-up: five slim dashes along the very top
// edge, one per 20%. Dim on purpose so it reads as trim, not as a gauge; the
// last dash turns orange. A dash only comes back once the reading is clearly
// above its step, so a beep sagging the voltage doesn't make it flicker.
static void drawBatteryTrim() {
  static uint8_t segs = 0xFF;
  if (batMv <= 0) return;
  int pct = batPct();
  uint8_t want = (pct + 19) / 20;
  if (segs == 0xFF || want < segs || pct > segs * 20 + 5) segs = want;
  for (int i = 0; i < 5; i++)
    spr.fillRect(CX - 26 + i * 11, 1, 9, 2, i >= segs ? GRID : segs <= 1 ? HOT : DIM);
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
enum { ICON_FEED, ICON_DANCE, ICON_BED, ICON_N };
uint8_t iconSel = 0;
const int BOTTOM_TOP = 166;      // buddy strip at 2× clears y < 164

static void drawNeeds() {
  for (int i = 0; i < PET_MAX; i++) {
    int x = 34 + i * 22;
    drawApple(x, 177, 4, i < pet().hunger);
    drawHeart(x, 190, 1, i < pet().happy, i < pet().happy ? RED : DIM);
  }
}

static void drawBottom(bool showSel) {
  spr.fillRect(0, BOTTOM_TOP, W, H - BOTTOM_TOP, BG);
  drawNeeds();
  spr.drawFastHLine(0, 201, W, DIM);
  for (int i = 0; i < ICON_N; i++) {
    int cx = 22 + i * 45, cy = 222;
    if (i == ICON_FEED)      drawApple(cx, cy + 2, 9);
    else if (i == ICON_DANCE) drawParty(cx, cy);
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
    buddySetPeek(false);
    spr.fillSprite(BG);
    buddyInvalidate();
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
  if ((int32_t)(now - rewardUntil) >= 0) {
    enterHome();
    // The last sticker gets a proper party.
    triggerOneShot(P_CELEBRATE, pet().stickers == STICKER_ALL ? 8000 : 2000);
    return;
  }
  buddyTick(P_CELEBRATE);
  spr.fillRect(0, BOTTOM_TOP, W, H - BOTTOM_TOP, BG);
  spr.drawRoundRect(CX - 24, 178, 48, 48, 8, INK);
  spr.drawRoundRect(CX - 23, 179, 46, 46, 7, INK);
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

static void drawHungerRow() {
  for (int i = 0; i < PET_MAX; i++) drawApple(34 + i * 22, 177, 4, i < pet().hunger);
}

static void tickFood(uint32_t now, bool a, bool b) {
  if (b) { foodSel = (foodSel + 1) % FOOD_N; beep(1800, 25); }
  if (a) { beep(2400, 25); startFeed(now, foodSel); return; }

  buddyTick(P_IDLE);
  spr.fillRect(0, BOTTOM_TOP, W, H - BOTTOM_TOP, BG);
  drawHungerRow();
  spr.drawFastHLine(0, 190, W, DIM);
  for (int i = 0; i < FOOD_N; i++) {
    int x = 21 + i * 31, y = 216;
    drawFood(i, x, y);
    if (i == foodSel) {
      spr.drawRoundRect(x - 15, y - 17, 30, 34, 6, INK);
      spr.drawRoundRect(x - 14, y - 16, 28, 32, 5, INK);
    }
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

// --- Catch ------------------------------------------------------------------
// Snacks fall one at a time; tilt the stick left/right to slide the basket
// under them. No way to lose — every round ends in a celebration.
const uint8_t PLAY_ROUNDS = 5;
const int     FIELD_TOP   = 86, BASKET_Y = 214, CATCH_HALF = 30;
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
  buddySetPeek(true);
  spr.fillSprite(BG);
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

  buddyTick((int32_t)(now - playCheerUntil) < 0 ? P_HEART : P_IDLE);

  spr.fillRect(0, FIELD_TOP, W, H - FIELD_TOP, BG);
  drawFood(dropFood, dropX, dropY);
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
  buddySetPeek(false);
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
  if (!pet().stayOn && now - bedStartMs > BED_OFF_MS) napDown();
}

#include "activities.h"

// --- Toy box ----------------------------------------------------------------
// Two pages of six tiles. Side walks through them in reading order and rolls
// from one page onto the next. It always opens on the first tile.
const int TILE_W = 58, TILE_H = 66;
const uint32_t EGG_HOLD_MS = 3000;
uint8_t  menuSel = 0;
uint32_t eggHoldMs = 0, tileWobbleUntil = 0;

static uint8_t menuCount() { return pet().eggTile ? T_N : T_N - 1; }

static void openMenu(uint8_t sel) {
  mode = M_MENU;
  menuSel = sel < menuCount() ? sel : 0;
  eggHoldMs = 0;
  buddySetPeek(false);
  applyBrightness();
  lastInteractMs = millis();
}

static void backToEgg() {
  petReset();
  applyLook();
  enterEgg();
}

static void launchTile(uint8_t tile, uint32_t now) {
  switch (tile) {
    case T_WALK:     startWalk(); break;
    case T_BATH:     startBath(); break;
    case T_ROCK:     startRock(); break;
    case T_FREEZE:   startFreeze(now); break;
    case T_DRESS:    startDress(); break;
    case T_STICKERS: startStickers(); break;
    case T_CATCH:    startPlay(now); break;
    case T_DRAW:     startDraw(now); break;
    case T_MARBLE:   startMarble(now); break;
    case T_COPY:     startCopy(now); break;
    case T_HIDE:     startHide(now); break;
    case T_EGG:      tileWobbleUntil = now + 400; break;   // a tap only wobbles it
  }
}

// Hold blue on the egg tile: a ring fills over three seconds, then
// everything goes back to an unhatched egg. Letting go early cancels.
static bool tickEggHold(uint32_t now) {
  bool holding = menuSel == T_EGG && !swallowA && M5.BtnA.isPressed() && !M5.BtnB.isPressed();
  if (!holding) { eggHoldMs = 0; return false; }
  if (!eggHoldMs) eggHoldMs = now;
  uint32_t held = now - eggHoldMs;
  if (held < 250) return false;                  // an ordinary tap never shows the ring
  lastInteractMs = now;
  if (held >= EGG_HOLD_MS) {
    swallowA = true;
    eggHoldMs = 0;
    spr.fillSprite(INK);
    spr.pushSprite(0, 0);
    static const uint16_t TUNE[] = { 1568, 1047, 784 };
    jingle(TUNE, 3, 110);
    backToEgg();
    return true;
  }
  static uint8_t lastStep = 0;
  uint8_t step = held / 400;
  if (step != lastStep) { lastStep = step; beep(500 + step * 120, 50); }

  spr.fillSprite(BG);
  drawEggShape(CX, 108, 34, 44);
  int lit = held * 48 / EGG_HOLD_MS;
  for (int i = 0; i < 48; i++) {
    float ang = -1.5708f + i * 0.1309f;
    spr.fillCircle(CX + (int)(cosf(ang) * 58), 108 + (int)(sinf(ang) * 58), i < lit ? 3 : 1, i < lit ? HOT : GRID);
  }
  spr.fillCircle(CX, 210, 10 + ((now / 400) & 1) * 3, BTN_BLUE);
  return true;
}

static void tickMenu(uint32_t now, bool a, bool b) {
  uint8_t n = menuCount();
  if (b) { menuSel = (menuSel + 1) % n; beep(1800, 25); }
  if (tickEggHold(now)) return;
  if (a) { beep(2400, 25); launchTile(menuSel, now); if (mode != M_MENU) return; }

  spr.fillSprite(BG);
  uint8_t page = menuSel / 6;
  for (uint8_t i = page * 6; i < n && i < page * 6 + 6; i++) {
    int x = 6 + (i % 2) * 65, y = 12 + ((i % 6) / 2) * 72;
    if (i == menuSel) {
      spr.drawRoundRect(x, y, TILE_W, TILE_H, 8, INK);
      spr.drawRoundRect(x + 1, y + 1, TILE_W - 2, TILE_H - 2, 7, INK);
    } else {
      spr.drawRoundRect(x, y, TILE_W, TILE_H, 8, GRID);
    }
    int wob = (i == T_EGG && (int32_t)(now - tileWobbleUntil) < 0) ? (((now / 60) & 1) ? 3 : -3) : 0;
    drawTileIcon(i, x + TILE_W / 2 + wob, y + TILE_H / 2);
  }
  uint8_t pages = (n + 5) / 6;
  for (uint8_t p = 0; p < pages; p++)
    spr.fillCircle(CX - (pages - 1) * 6 + p * 12, 233, 2, p == page ? INK : DIM);
}

// Hold-blue from inside something: back to the pet. A drawing that got some
// real pen time still counts as playing.
static void leaveToHome(uint32_t now) {
  if (mode == M_DRAW && drawTravel > 300) { finishActivity(now); return; }
  enterHome();
}

// --- Tilt calibration -------------------------------------------------------
// From the parent screen. Five poses, blue on each: how the stick is held to
// play, then tilted as if rolling a ball left, right, up and down the
// screen. The sensor is averaged for a moment after each click, and the
// difference between opposite poses says which way "right" and "down"
// really read on this board and how far a hand comfortably tips it. The
// numbers stay on screen at the end, and go to the serial log, for tuning.
enum { CAL_LEVEL, CAL_LEFT, CAL_RIGHT, CAL_UP, CAL_DOWN, CAL_POSES };
const uint32_t CAL_SETTLE_MS = 150;    // let the button press stop wobbling the stick
const uint32_t CAL_READ_MS   = 400;
const float    CAL_MIN_REACH = 0.05f;  // g each way; less than this wasn't really a tilt
const float    CAL_FULL_TILT = 0.35f;  // what the games treat as a full tilt
uint8_t  calPose = 0;
bool     calReading = false, calGood = false;
uint32_t calAt = 0;
uint16_t calN = 0;
float    calSum[3], calG[CAL_POSES][3], calReachX = 0, calReachY = 0;

static void startCal() {
  mode = M_CAL;
  calPose = 0;
  calReading = false;
}

// Direction and half-length of the swing between two opposite poses.
static float calAxis(uint8_t from, uint8_t to, const float* g, float* out) {
  float d[3];
  for (int i = 0; i < 3; i++) d[i] = calG[to][i] - calG[from][i];
  float k = d[0]*g[0] + d[1]*g[1] + d[2]*g[2];
  for (int i = 0; i < 3; i++) out[i] = d[i] - k * g[i];
  float len = sqrtf(out[0]*out[0] + out[1]*out[1] + out[2]*out[2]);
  if (len > 0) for (int i = 0; i < 3; i++) out[i] /= len;
  return len / 2;
}

static void calFinish() {
  float g[3], len = 0;
  for (int i = 0; i < 3; i++) len += calG[CAL_LEVEL][i] * calG[CAL_LEVEL][i];
  len = sqrtf(len);
  for (int i = 0; i < 3; i++) g[i] = len > 0 ? calG[CAL_LEVEL][i] / len : 0;

  TiltCal c;
  calReachX = calAxis(CAL_LEFT, CAL_RIGHT, g, c.u);
  calReachY = calAxis(CAL_UP, CAL_DOWN, g, c.v);
  calGood = calReachX >= CAL_MIN_REACH && calReachY >= CAL_MIN_REACH;
  if (calGood) {
    c.set = true;
    c.gainX = constrain(CAL_FULL_TILT / calReachX, 0.5f, 3.0f);
    c.gainY = constrain(CAL_FULL_TILT / calReachY, 0.5f, 3.0f);
    tiltCal() = c;
    tiltCalSave();
  }

  static const char* const NAME[CAL_POSES] = { "level", "left", "right", "up", "down" };
  for (int i = 0; i < CAL_POSES; i++)
    Serial.printf("cal %-5s % .3f % .3f % .3f\n", NAME[i], calG[i][0], calG[i][1], calG[i][2]);
  Serial.printf("cal %s reach %.3f %.3f u % .2f % .2f % .2f v % .2f % .2f % .2f\n",
                calGood ? "saved" : "rejected", calReachX, calReachY,
                c.u[0], c.u[1], c.u[2], c.v[0], c.v[1], c.v[2]);
}

static void drawCalPose(uint8_t pose) {
  static const int8_t DX[CAL_POSES] = { 0, -1, 1, 0, 0 }, DY[CAL_POSES] = { 0, 0, 0, -1, 1 };
  int cx = CX, cy = 100, dx = DX[pose], dy = DY[pose];
  spr.drawRoundRect(cx - 34, cy - 50, 68, 100, 8, DIM);        // the stick, screen towards you
  spr.fillCircle(cx + dx * 14, cy + dy * 26, 7, AQUA);
  spr.drawCircle(cx + dx * 14, cy + dy * 26, 7, INK);
  if (!dx && !dy) return;
  int tx = cx + dx * 54, ty = cy + dy * 70;                    // arrowhead outside the outline
  spr.fillTriangle(tx, ty, tx - dx * 12 - dy * 9, ty - dy * 12 - dx * 9,
                   tx - dx * 12 + dy * 9, ty - dy * 12 + dx * 9, YEL);
}

static void tickCal(uint32_t now, bool a, bool b) {
  lastInteractMs = now;
  float v[3];
  M5.Imu.getAccel(&v[0], &v[1], &v[2]);

  spr.fillSprite(BG);
  spr.setTextSize(1);
  spr.setTextColor(INK, BG);
  spr.setCursor(6, 8);

  if (calPose >= CAL_POSES) {                    // results
    if (b) { beep(1800, 25); startCal(); return; }
    if (a) { beep(2400, 25); mode = M_PARENT; return; }
    spr.print(calGood ? "TILT CAL SAVED" : "NOT ENOUGH TILT");
    spr.setTextColor(DIM, BG);
    static const char* const TAG[CAL_POSES] = { "lvl", "lft", "rgt", "up ", "dn " };
    for (int i = 0; i < CAL_POSES; i++) {
      spr.setCursor(6, 30 + i * 14);
      spr.printf("%s%6.2f%6.2f%6.2f", TAG[i], calG[i][0], calG[i][1], calG[i][2]);
    }
    spr.setCursor(6, 112); spr.printf("reach l/r %.2f g", calReachX);
    spr.setCursor(6, 126); spr.printf("reach u/d %.2f g", calReachY);
    if (calGood) {
      spr.setCursor(6, 146); spr.printf("gain %.1f x  %.1f y", tiltCal().gainX, tiltCal().gainY);
    } else {
      spr.setCursor(6, 146); spr.print("old setting kept");
    }
    spr.setCursor(6, H - 24); spr.print("side: again");
    spr.setCursor(6, H - 12); spr.print("blue: back");
    return;
  }

  if (calReading) {
    if (now - calAt >= CAL_SETTLE_MS) {
      for (int i = 0; i < 3; i++) calSum[i] += v[i];
      calN++;
    }
    if (now - calAt >= CAL_SETTLE_MS + CAL_READ_MS) {
      for (int i = 0; i < 3; i++) calG[calPose][i] = calSum[i] / calN;
      calReading = false;
      beep(2000, 60);
      if (++calPose >= CAL_POSES) { calFinish(); return; }
    }
  } else if (a) {
    calReading = true;
    calAt = now;
    calN = 0;
    calSum[0] = calSum[1] = calSum[2] = 0;
    beep(2400, 25);
  } else if (b) {
    // Side backs out. On the first pose it also forgets a saved calibration,
    // which is the way back to the built-in one.
    if (calPose == 0 && tiltCal().set) { tiltCal().set = false; tiltCalSave(); }
    beep(1800, 25);
    mode = M_PARENT;
    return;
  }

  spr.printf("TILT CAL %u/%u", calPose + 1, CAL_POSES);
  drawCalPose(calPose);
  spr.setTextColor(calReading ? YEL : INK, BG);
  spr.setCursor(6, 178);
  spr.print(calReading ? "hold still..." : calPose ? "tilt so the ball" : "hold it the way");
  spr.setCursor(6, 190);
  spr.print(calReading ? "" : calPose ? "would roll there" : "you play");
  spr.setTextColor(DIM, BG);
  spr.setCursor(6, 204); spr.printf("%6.2f%6.2f%6.2f", v[0], v[1], v[2]);
  spr.setCursor(6, H - 24);
  spr.print(calPose ? "side: cancel" : tiltCal().set ? "side: clear saved" : "side: cancel");
  spr.setCursor(6, H - 12); spr.print("blue: read");
}

// --- Parent screen ----------------------------------------------------------
enum { PA_BRIGHT, PA_SOUND, PA_STAYON, PA_EGGTILE, PA_PET, PA_CAL, PA_LEARN, PA_RESET, PA_BACK, PA_N };
const char* const PARENT_ITEMS[PA_N] = {
  "bright", "sound", "stay on", "egg tile", "pet", "tilt cal", "lesson", "reset", "back"
};
uint8_t  parentSel = 0;
uint8_t  parentWasSpecies = 0;   // the pet as it was when the screen opened
uint32_t resetConfirmUntil = 0;

static void leaveParent() {
  if (pet().hatched)       enterHome();
  else if (!pet().learned) startLearn(millis());   // new or just reset: the lesson, then the egg
  else                     enterEgg();
}

static void applyParent(uint8_t idx, uint32_t now) {
  PetSave& s = pet();
  switch (idx) {
    case PA_BRIGHT: s.bright = (s.bright + 1) % 5; applyBrightness(); break;
    case PA_SOUND:  s.sound = !s.sound; break;
    case PA_STAYON: s.stayOn = !s.stayOn; break;
    case PA_EGGTILE: s.eggTile = !s.eggTile; break;   // hides "new egg" in the toy box
    case PA_PET:
      // random, then each animal in turn. Naming one swaps the pet there and
      // then; back on random it is whoever it was, and only eggs are random.
      s.pick = (s.pick + 1) % (buddySpeciesCount() + 1);
      s.species = s.pick ? s.pick - 1 : parentWasSpecies;
      buddySetSpeciesIdx(s.species);
      break;
    case PA_CAL:    startCal(); return;
    case PA_LEARN:  petSave(); startLearn(now); return;   // nothing is reset; it ends where the pet is
    case PA_RESET:
      // Tap-twice confirm: first tap arms (label flips to "really?"). Then
      // the pet, its stickers, the settings and the awake timer all go.
      if ((int32_t)(now - resetConfirmUntil) >= 0) { resetConfirmUntil = now + 3000; return; }
      resetConfirmUntil = 0;
      petFactoryReset();
      awakeMarkMs = now;
      applyBrightness();
      applyLook();
      leaveParent();
      return;
    case PA_BACK:   petSave(); leaveParent(); return;
  }
  petSave();
}

static void tickParent(uint32_t now, bool a, bool b) {
  lastInteractMs = now;
  if (b) { parentSel = (parentSel + 1) % PA_N; resetConfirmUntil = 0; beep(1800, 25); }
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
    bool armed = (i == PA_RESET) && (int32_t)(now - resetConfirmUntil) < 0;
    spr.setTextColor(armed ? HOT : sel ? INK : DIM, BG);
    spr.setCursor(6, y);
    spr.print(sel ? "> " : "  ");
    spr.print(armed ? "really?" : PARENT_ITEMS[i]);
    spr.setTextColor(DIM, BG);
    spr.setCursor(84, y);
    if (i == PA_BRIGHT)      spr.printf("%u/4", s.bright);
    else if (i == PA_SOUND)  spr.print(s.sound ? "on" : "off");
    else if (i == PA_STAYON) spr.print(s.stayOn ? "on" : "off");
    else if (i == PA_EGGTILE) spr.print(s.eggTile ? "on" : "off");
    else if (i == PA_PET)    spr.print(s.pick ? buddySpeciesName() : "random");
    else if (i == PA_CAL)    spr.print(tiltCal().set ? "set" : "-");
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

  if (!s.stayOn && now - lastInteractMs > IDLE_BED_MS) startBed(now, false);
}

void setup() {
  // USB-CDC (native USB on ESP32-S3) blocks on write when no serial monitor
  // is attached — the TX buffer fills and Serial.print() waits forever for a
  // host, hanging setup()/loop(). Make it non-blocking so the device runs
  // standalone. The StickC Plus has a real UART and no such call.
#if defined(BOARD_STICKS3)
  Serial.setTxTimeoutMs(0);
  esp_sleep_wakeup_cause_t woke = esp_sleep_get_wakeup_cause();
  bool napOver = woke == ESP_SLEEP_WAKEUP_TIMER;
  bool napWoken = woke == ESP_SLEEP_WAKEUP_EXT0;
  // A nap that ran out only wakes to switch off: keep the backlight dark.
  if (napOver) M5.Display.setBrightness(0);
#else
  const bool napOver = false, napWoken = false;
#endif
  M5.begin();
  logBegin(napOver ? "boot:timer" : napWoken ? "boot:button" : "boot:cold");
  if (napOver) { logRow("off"); powerOffNow(); }
#if defined(BOARD_STICKS3)
  // The PM1 keeps its settings through a nap and M5.begin() leaves this one
  // alone, so the light railsDown() put out is lit again here.
  M5.Power.M5pm1.setLedEnLevel(true);
  // Only the accelerometer is ever read; M5.begin() starts the gyro as well.
  M5.In_I2C.writeRegister8(0x69, 0x7D, 0x04, 400000);    // BMI270: accel only
#endif
  // No radio and 5 fps animation: 80 MHz is plenty and roughly halves the
  // CPU's share of the battery. (Lowest speed that keeps USB alive.)
  setCpuFrequencyMhz(80);
  M5.Lcd.setRotation(0);
  M5.Speaker.setVolume(90);      // IMU auto-inits in M5.begin()
  petLoad();
  petMigrateRoster();
  applyBrightness();
  spr.createSprite(W, H);
  buddyInit();
  if (pet().pick > buddySpeciesCount()) pet().pick = 0;   // that animal has since been removed
  buddySetSpeciesIdx(pet().species);
  applyLook();
  favFood = esp_random() % FOOD_N;

  uint32_t now = millis();
  awakeMarkMs = lastHungerMs = lastHappyMs = lastInteractMs = now;
  if (pet().hatched) {
    // Woke up a little hungry and a little bored: always something to do.
    // Not after a nap, or a kid who keeps waking it would starve it.
    if (!napWoken) {
      if (pet().hunger) pet().hunger--;
      if (pet().happy)  pet().happy--;
    }
    enterHome();
    triggerOneShot(P_SLEEP, 2500);
  } else if (!pet().learned) {
    startLearn(now);
  } else {
    mode = M_EGG;
  }
  // The press that ended the nap is still down: it wakes, it doesn't select.
  if (napWoken) {
    M5.update(); delay(20); M5.update();
    swallowA = M5.BtnA.isPressed();
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
    if (mode == M_LEARN) pet().learned = true;   // a grown-up stepping in skips the lesson
    buddySetPeek(false);
    applyBrightness();
    parentSel = 0;
    parentWasSpecies = pet().species;
    mode = M_PARENT;
    beep(800, 80);
  }

  // Hold the side button alone: mute/unmute. The chirp plays only when
  // sound has just come back on; the glyph covers the silent direction.
  if (mode != M_PARENT && mode != M_CAL && !swallowB && M5.BtnB.pressedFor(1500) && !M5.BtnA.isPressed()) {
    swallowB = true;
    b = false;
    pet().sound = !pet().sound;
    petSave();
    beep(1800, 80);
    soundFlashUntil = now + 1500;
  }

  // Hold the blue button alone: the toy box from the pet screen, and back to
  // the pet from the toy box or any activity. The egg tile keeps the hold
  // for itself (see tickEggHold).
  bool holdOk = mode == M_HOME || mode == M_FOOD || mode == M_PLAY || mode >= M_WALK ||
                (mode == M_MENU && menuSel != T_EGG);
  if (holdOk && !swallowA && M5.BtnA.pressedFor(HOLD_BLUE_MS) && !M5.BtnB.isPressed()) {
    swallowA = true;
    a = false;
    beep(1200, 60);
    if (mode == M_HOME) openMenu(0);
    else                leaveToHome(now);
  }

  bool shook = false;
  if (mode != M_BED && mode != M_PARENT) {
    sampleMotion(now);
    shook = imuFresh && imuDelta > SHAKE_DELTA;
  }
  logTick(now);

  // Left alone: the toy box and food tray drift back to the pet, and an
  // activity goes to bed like the pet screen does. Moving the stick counts
  // as playing, so a walk isn't cut short for want of a button press.
  if (mode == M_MENU || mode == M_FOOD || mode == M_STICKERS) {
    if (now - lastInteractMs > MENU_IDLE_MS) enterHome();
  } else if (mode >= M_WALK) {
    if (imuFresh && imuDelta > 0.12f) lastInteractMs = now;
    if (!pet().stayOn && now - lastInteractMs > IDLE_BED_MS) startBed(now, false);
  }

  switch (mode) {
    case M_EGG:
      tickEgg(now, a, b, shook);
      if (mode == M_EGG && !pet().stayOn && now - lastInteractMs > IDLE_BED_MS + BED_OFF_MS) napDown();
      break;
    case M_HOME:   tickHome(now, a, b, shook); break;
    case M_FOOD:   tickFood(now, a, b); break;
    case M_FEED:   tickFeed(now); break;
    case M_PLAY:   tickPlay(now); break;
    case M_BED:    tickBed(now); break;
    case M_PARENT: tickParent(now, a, b); break;
    case M_MENU:   tickMenu(now, a, b); break;
    case M_REWARD: tickReward(now); break;
    case M_CAL:    tickCal(now, a, b); break;
    case M_LEARN:
      tickLearn(now, a, b);
      if (mode == M_LEARN && !pet().stayOn && now - lastInteractMs > IDLE_BED_MS + BED_OFF_MS) napDown();
      break;
    case M_WALK:   tickWalk(now); break;
    case M_BATH:   tickBath(now); break;
    case M_ROCK:   tickRock(now); break;
    case M_FREEZE: tickFreeze(now); break;
    case M_DRESS:  tickDress(a, b); break;
    case M_STICKERS:
      tickStickers();
      if (a || b) openMenu(T_STICKERS);
      break;
    case M_DRAW:   tickDraw(now, a, shook); break;
    case M_MARBLE: tickMarble(now, a); break;
    case M_COPY:   tickCopy(now, a, b); break;
    case M_HIDE:   tickHide(now, a, b); break;
    case M_DANCE:  tickDance(now); break;
  }

  bool kidScreen = mode != M_PARENT && mode != M_CAL;
  if (kidScreen) drawBatteryTrim();
  if (kidScreen && (int32_t)(now - soundFlashUntil) < 0) drawSoundFlash();
  spr.pushSprite(0, 0);
  delay(mode == M_BED ? 100 : 33);
}
