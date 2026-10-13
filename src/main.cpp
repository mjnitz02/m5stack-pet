#include "app.h"
#include "palette.h"
#include "buddy.h"
#include "pet.h"
#include "input.h"
#include "motion.h"
#include "power.h"
#include "petlog.h"

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
// main.cpp holds setup, the loop and the table of screens. The screens
// themselves are in home.h, activities.h, toybox.h and parent.h, and icons.h
// has every drawing; all five are compiled as part of this file. motion,
// power and input are the hardware underneath.

TFT_eSprite spr = TFT_eSprite(&M5.Lcd);

// Toy box tiles in reading order. The first page is care and moving, the
// second is the sit-still games. The egg stays last so it can be hidden.
enum { T_WALK, T_BATH, T_ROCK, T_FREEZE, T_DRESS, T_STICKERS,
       T_CATCH, T_DRAW, T_MARBLE, T_COPY, T_HIDE, T_EGG, T_N };

// Mostly healthy, one treat. Each fills one apple on the hunger row.
enum { FOOD_APPLE, FOOD_CARROT, FOOD_STRAWBERRY, FOOD_ICECREAM, FOOD_N };

Mode     mode = M_EGG;

// A new screen: the pet at full size or the small one beside a game, on black.
static void clearScreen(bool smallPet) {
  buddySetPeek(smallPet);
  spr.fillSprite(BG);
  buddyInvalidate();
}

static void applyBrightness() { M5.Display.setBrightness((pet().bright + 1) * 51); }

static void beep(uint16_t freq, uint16_t dur) {
  if (pet().sound) M5.Speaker.tone(freq, dur);
}

// Short blocking tune for one-off moments (hatch, win, bedtime).
static void jingle(const uint16_t* notes, uint8_t n, uint16_t each) {
  if (!pet().sound) return;
  for (uint8_t i = 0; i < n; i++) { M5.Speaker.tone(notes[i], each); delay(each + 15); }
}

static void startDance(uint32_t now);   // in activities.h; the home screen starts it

#include "icons.h"
#include "home.h"
#include "activities.h"
#include "toybox.h"
#include "parent.h"

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

// Speaker glyph shown for a moment after the hold-side mute toggle. Not
// every screen repaints that corner each frame, so it is wiped when its
// time is up and the pet, which may have been under it, is drawn afresh.
uint32_t soundFlashUntil = 0;
bool     soundFlashShown = false;
static void drawSoundFlash() {
  uint16_t c = pet().sound ? INK : HOT;
  soundFlashShown = true;
  spr.fillRect(2, 2, 22, 14, BG);
  spr.fillRect(4, 7, 4, 5, c);
  spr.fillTriangle(8, 7, 8, 11, 13, 4, c);
  spr.fillTriangle(8, 11, 13, 4, 13, 14, c);
  if (pet().sound) { spr.drawLine(16, 6, 16, 12, c); spr.drawLine(19, 4, 19, 14, c); }
  else             { spr.drawLine(16, 5, 22, 13, c); spr.drawLine(22, 5, 16, 13, c); }
}

// --- The screens ------------------------------------------------------------
// One row per Mode, in the same order. A screen is its name, what it does
// each frame, what happens when it is left alone, and what the loop does for
// it. A new screen is a new Mode and a new row.
struct Frame { uint32_t now; bool a, b, shook; };   // the two taps, and a shake

enum : uint8_t {
  IDLE_NONE,     // looks after itself
  IDLE_HOME,     // drifts back to the pet
  IDLE_BED,      // goes to bed like the pet screen does. Moving the stick counts
                 // as playing, so a walk isn't cut short for want of a button press
  IDLE_PARENT,   // opened by a squeeze in a bag, or walked away from: back to
                 // the kid's screen, which puts itself to bed
};
enum : uint8_t {
  SCR_KID       = 1,   // battery trim along the top; hold side is sound on/off
  SCR_MOTION    = 2,   // the accelerometer is sampled
  SCR_BLUE_BACK = 4,   // hold blue goes back to the pet
};
const uint8_t PLAIN = SCR_KID | SCR_MOTION, GAME = PLAIN | SCR_BLUE_BACK;

const uint32_t MENU_IDLE_MS = 15000;  // toy box / food tray left alone → back to the pet

// The egg and the lesson have no pet to put to bed: left alone, they nap
// where they are.
static void napIfLeft(Mode m, uint32_t now) {
  if (mode == m && !pet().stayOn && idleFor(now, IDLE_BED_MS + BED_OFF_MS)) napDown();
}

struct Screen {
  const char* name;
  void (*tick)(const Frame&);
  uint8_t idle, flags;
};
static const Screen SCREENS[] = {
  { "egg",      [](const Frame& f) { tickEgg(f.now, f.a, f.b, f.shook); napIfLeft(M_EGG, f.now); }, IDLE_NONE, PLAIN },
  { "home",     [](const Frame& f) { tickHome(f.now, f.a, f.b, f.shook); }, IDLE_NONE,   PLAIN },
  { "food",     [](const Frame& f) { tickFood(f.now, f.a, f.b); },          IDLE_HOME,   GAME },
  { "feed",     [](const Frame& f) { tickFeed(f.now); },                    IDLE_NONE,   PLAIN },
  { "play",     [](const Frame& f) { tickPlay(f.now); },                    IDLE_NONE,   GAME },
  { "bed",      [](const Frame& f) { tickBed(f.now); },                     IDLE_NONE,   SCR_KID },
  { "parent",   [](const Frame& f) { tickParent(f.now, f.a, f.b); },        IDLE_PARENT, 0 },
  { "menu",     [](const Frame& f) { tickMenu(f.now, f.a, f.b); },          IDLE_HOME,   GAME },
  { "reward",   [](const Frame& f) { tickReward(f.now); },                  IDLE_NONE,   PLAIN },
  { "cal",      [](const Frame& f) { tickCal(f.now, f.a, f.b); },           IDLE_PARENT, SCR_MOTION },
  { "learn",    [](const Frame& f) { tickLearn(f.now, f.a, f.b); napIfLeft(M_LEARN, f.now); }, IDLE_NONE, PLAIN },
  { "walk",     [](const Frame& f) { tickWalk(f.now); },                    IDLE_BED,    GAME },
  { "bath",     [](const Frame& f) { tickBath(f.now); },                    IDLE_BED,    GAME },
  { "rock",     [](const Frame& f) { tickRock(f.now); },                    IDLE_BED,    GAME },
  { "freeze",   [](const Frame& f) { tickFreeze(f.now); },                  IDLE_BED,    GAME },
  { "dress",    [](const Frame& f) { tickDress(f.a, f.b); },                IDLE_BED,    GAME },
  { "stickers", [](const Frame& f) { tickStickers(); if (f.a || f.b) openMenu(T_STICKERS); }, IDLE_HOME, GAME },
  { "draw",     [](const Frame& f) { tickDraw(f.now, f.a, f.shook); },      IDLE_BED,    GAME },
  { "marble",   [](const Frame& f) { tickMarble(f.now, f.a); },             IDLE_BED,    GAME },
  { "copy",     [](const Frame& f) { tickCopy(f.now, f.a, f.b); },          IDLE_BED,    GAME },
  { "hide",     [](const Frame& f) { tickHide(f.now, f.a, f.b); },          IDLE_BED,    GAME },
  { "dance",    [](const Frame& f) { tickDance(f.now); },                   IDLE_BED,    GAME },
};
static_assert(sizeof(SCREENS) / sizeof(SCREENS[0]) == M_N, "SCREENS is out of step with Mode");

const char* screenName() { return SCREENS[mode].name; }
static bool screenHas(uint8_t flag) { return SCREENS[mode].flags & flag; }

void setup() {
  // USB-CDC (native USB on ESP32-S3) blocks on write when no serial monitor
  // is attached — the TX buffer fills and Serial.print() waits forever for a
  // host, hanging setup()/loop(). Make it non-blocking so the device runs
  // standalone. The StickC Plus has a real UART and no such call.
#if defined(BOARD_STICKS3)
  Serial.setTxTimeoutMs(0);
#endif
  Wake wake = powerWake();
  bool napOver = wake == WAKE_TIMER, napWoken = wake == WAKE_BUTTON;
  // A nap that ran out only wakes to switch off: keep the backlight dark.
  if (napOver) M5.Display.setBrightness(0);
  // M5.begin() leaves the serial port unopened. On the StickS3 an unopened
  // port drops everything written to it and answers available() with -1.
  Serial.begin(115200);
  M5.begin();
  logBegin(napOver ? "boot:timer" : napWoken ? "boot:button" : "boot:cold");
  if (napOver) { logRow("off"); powerOffNow(); }
  powerBoardSetup();
  M5.Lcd.setRotation(0);
  M5.Speaker.setVolume(90);      // IMU auto-inits in M5.begin()
  petLoad();
  petMigrateRoster();
  applyBrightness();
  spr.createSprite(W, H);
  buddyInit();
  if (pet().pick > buddySpeciesCount()) pet().pick = 0;   // that animal has since been removed
  if (pet().color >= LOOK_N) pet().color = 0;             // as has that colour, hat or food
  if (pet().hat >= HAT_N)    pet().hat = 0;
  if (pet().food >= FOOD_N)  pet().food = 0;
  buddySetSpeciesIdx(pet().species);
  applyLook();
  favFood = esp_random() % FOOD_N;

  uint32_t now = millis();
  lastHungerMs = lastHappyMs = lastInteractMs = now;
  if (napWoken) {
    // Each timer picks up where the nap left it.
    awakeRestart(now - napCarry.awakeMs);
    lastHungerMs -= napCarry.hungerMs;
    lastHappyMs  -= napCarry.happyMs;
  } else {
    awakeRestart(now);
  }
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
  Mode     was = mode;
  powerTick(now);

  Frame f = { now, false, false, false };
  inputTaps(now, &f.a, &f.b);

  if (mode != M_PARENT && heldBoth()) {
    f.a = f.b = false;
    openParent();
  }

  // Hold the side button alone: mute/unmute. The chirp plays only when
  // sound has just come back on; the glyph covers the silent direction.
  if (screenHas(SCR_KID) && heldSide()) {
    f.b = false;
    pet().sound = !pet().sound;
    petSave();
    beep(1800, 80);
    soundFlashUntil = now + 1500;
  }

  // Hold the blue button alone: the toy box from the pet screen, and back to
  // the pet from the toy box or any activity. The egg tile keeps the hold
  // for itself (see tickEggHold).
  bool blueOk = mode == M_HOME ||
                (screenHas(SCR_BLUE_BACK) && !(mode == M_MENU && menuSel == T_EGG));
  if (blueOk && heldBlue()) {
    f.a = false;
    beep(1200, 60);
    if (mode == M_HOME) openMenu(0);
    else                leaveToHome(now);
  }

  if (screenHas(SCR_MOTION)) {
    sampleMotion(now);
    f.shook = imuFresh && imuDelta > SHAKE_DELTA;
  } else {
    imuFresh = false;
  }
  logTick(now);

  switch (SCREENS[mode].idle) {
    case IDLE_HOME:
      if (idleFor(now, MENU_IDLE_MS)) enterHome();
      break;
    case IDLE_BED:
      if (imuFresh && imuDelta > 0.12f) lastInteractMs = now;
      if (!pet().stayOn && idleFor(now, IDLE_BED_MS)) startBed(now, false);
      break;
    case IDLE_PARENT:
      if (!pet().stayOn && idleFor(now, IDLE_BED_MS)) leaveParent();
      break;
  }

  if (soundFlashShown && !before(now, soundFlashUntil)) {
    soundFlashShown = false;
    spr.fillRect(2, 2, 22, 14, BG);
    buddyInvalidate();
  }

  SCREENS[mode].tick(f);

  if (mode != was) inputScreenChanged();

  if (screenHas(SCR_KID)) drawBatteryTrim();
  if (screenHas(SCR_KID) && before(now, soundFlashUntil)) drawSoundFlash();
  spr.pushSprite(0, 0);
  delay(mode == M_BED ? 100 : 33);
}
