#pragma once

// The grown-up's screens, in text: settings, and the tilt calibration they
// lead to. Header-only like activities.h: include from main.cpp only.

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
  memset(&c, 0, sizeof(c));   // the gaps between its fields go to flash too
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
  float v[3];
  motionRaw(v);

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

// Hold both buttons, from anywhere.
static void openParent() {
  if (mode == M_LEARN) pet().learned = true;   // a grown-up stepping in skips the lesson
  buddySetPeek(false);
  applyBrightness();
  parentSel = 0;
  parentWasSpecies = pet().species;
  mode = M_PARENT;
  beep(800, 80);
}

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
      if (!before(now, resetConfirmUntil)) { resetConfirmUntil = now + 3000; return; }
      resetConfirmUntil = 0;
      petFactoryReset();
      awakeRestart(now);
      applyBrightness();
      applyLook();
      leaveParent();
      return;
    case PA_BACK:   petSave(); leaveParent(); return;
  }
  petSave();
}

static void tickParent(uint32_t now, bool a, bool b) {
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
    bool armed = (i == PA_RESET) && before(now, resetConfirmUntil);
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
