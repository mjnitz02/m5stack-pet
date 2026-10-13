#include "motion.h"
#include "app.h"
#include "input.h"
#include "pet.h"

// The one read of the accelerometer. A frame can ask for it several times
// over (the shake sample, the tilt, face-down); the sensor is read for the
// first and the rest get the same answer.
static float accel[3] = { 0, 0, 1 };
static bool  accelRead = false;

void motionNewFrame() { accelRead = false; }

static const float* readAccel() {
  if (!accelRead) {
    M5.Imu.getAccel(&accel[0], &accel[1], &accel[2]);
    accelRead = true;
  }
  return accel;
}

// One motion sample every 50 ms, shared by everything that cares about
// movement: how far the pull on the stick is from resting 1 g.
static float    accelBaseline = 1.0f;
static uint32_t lastImuMs = 0;
float    imuDelta = 0;
float    imuTurn = 0;
static float    turnX = 0, turnY = 0, turnZ = 0;
bool     imuFresh = false;

void sampleMotion(uint32_t now) {
  imuFresh = false;
  if (now - lastImuMs < 50) return;
  lastImuMs = now;
  const float* v = readAccel();
  float ax = v[0], ay = v[1], az = v[2];
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
static const TiltCal  TILT_DEFAULT = { false, { 0, -1, 0 }, { -1, 0, 0 }, 0.55f, 0.55f };
static const uint32_t TILT_ZERO_MS = 500;
float    tiltX = 0, tiltY = 0;
static float    tiltSeenX = 0, tiltSeenY = 0;
static float    tiltSum[3] = { 0, 0, 0 };
static float    tiltU[3] = { 0, -1, 0 }, tiltV[3] = { -1, 0, 0 };   // screen x and y, laid level
static float    tiltGainX = 1, tiltGainY = 1;
static uint16_t tiltN = 0;
static uint32_t tiltZeroUntil = 0;

void tiltBegin(uint32_t now) {
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

bool tiltUpdate(uint32_t now) {
  const float* a = readAccel();
  if (before(now, tiltZeroUntil)) {
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
bool isFaceDown() {
  const float* v = readAccel();
  return v[2] < -0.7f && fabsf(v[0]) < 0.4f && fabsf(v[1]) < 0.4f;
}

void motionRaw(float* v) { memcpy(v, readAccel(), sizeof(accel)); }
