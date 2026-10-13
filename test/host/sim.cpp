#include "sim.h"

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "M5Unified.h"
#include "probe.h"

// The firmware's entry points (src/main.cpp, by way of firmware.cpp).
void setup();
void loop();

SerialFake    Serial;

namespace sim {

// --- The simulated stick --------------------------------------------------------
// One per process. The parent process, which runs the tests, never touches
// it; each boot is a forked child that starts from this untouched state, the
// same as the firmware's own globals.
namespace {

enum Motion { STILL, SHAKE, WIGGLE, SWAY };
const float HELD[3] = { -0.55f, 0.0f, 0.84f };   // motion.cpp, TILT_DEFAULT's readings

const uint64_t FNV_SEED = 1469598103934665603ull;

struct World {
  uint32_t ms = 250;                 // a real boot is already a moment old at setup()
  bool     btn[2] = { false, false };
  Motion   motion = STILL;
  float    pose[3] = { HELD[0], HELD[1], HELD[2] };
  uint32_t rng = 1;
  int      batMv = 3900, vbusMv = 0, wake = 0;
  int      bright = 127;             // M5GFX's own default
  bool     buttonArmed = false;
  uint64_t timerUs = 0;
  Nvs      nvs;
  int      beeps = 0;

  // The trace: event lines as they happen, and one checksum line for the
  // drawing in each second that drew anything.
  std::string trace;
  uint32_t sec = 0, frames = 0;
  uint64_t frameHash = 0, secHash = 0, askedHash = FNV_SEED;
  bool     secUsed = false, frameUsed = false;
  FILE*    dump = nullptr;
  bool     watching = false;
  std::vector<std::string> drawn;
};
World w;

uint64_t fnv(uint64_t h, const char* s) {
  for (; *s; s++) { h ^= (uint8_t)*s; h *= 1099511628211ull; }
  return h;
}

void foldFrame() {
  if (!w.frameUsed) return;
  char buf[24];
  snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)w.frameHash);
  w.secHash = fnv(w.secUsed ? w.secHash : FNV_SEED, buf);
  w.secUsed = true;
  w.frameUsed = false;
}

void flushSecond() {
  if (w.secUsed) {
    char buf[96];
    snprintf(buf, sizeof(buf), "%7us      frames=%-3u draw=%016llx\n", w.sec, w.frames, (unsigned long long)w.secHash);
    w.trace += buf;
  }
  w.secUsed = false;
  w.frames = 0;
}

void rollTo(uint32_t sec) {
  if (sec == w.sec) return;
  flushSecond();
  w.sec = sec;
}

std::string hexOf(const std::vector<uint8_t>& v) {
  std::string s;
  char b[4];
  for (uint8_t c : v) { snprintf(b, sizeof(b), "%02x", c); s += b; }
  return s.empty() ? "-" : s;
}

std::vector<uint8_t> unhex(const std::string& s) {
  std::vector<uint8_t> v;
  if (s == "-") return v;
  for (size_t i = 0; i + 1 < s.size(); i += 2) v.push_back((uint8_t)strtoul(s.substr(i, 2).c_str(), nullptr, 16));
  return v;
}

}  // namespace

// --- What the fake hardware calls ---------------------------------------------
namespace hw {

uint32_t millis() { return w.ms; }
void     delay(uint32_t ms) { w.ms += ms; }

uint32_t random() {
  w.rng ^= w.rng << 13;
  w.rng ^= w.rng >> 17;
  w.rng ^= w.rng << 5;
  return w.rng;
}

bool button(int which) { return w.btn[which]; }

void accel(float* x, float* y, float* z) {
  float k = 1, dy = 0;
  switch (w.motion) {
    case STILL:  break;
    case SHAKE:  k = ((w.ms / 100) & 1) ? 2.4f : 0.2f; break;
    case WIGGLE: k = ((w.ms / 150) & 1) ? 1.35f : 0.65f; break;
    case SWAY:   dy = 0.25f * (float)sin(w.ms * 6.283185307179586 / 1200.0); break;
  }
  *x = w.pose[0] * k;
  *y = w.pose[1] * k + dy;
  *z = w.pose[2] * k;
}

int batteryMv() { return w.batMv; }
int vbusMv() { return w.vbusMv; }
int wakeCause() { return w.wake; }

void setBrightness(int v) {
  if (v == w.bright) return;
  w.bright = v;
  event("brightness %d", v);
}
int brightness() { return w.bright; }

void event(const char* fmt, ...) {
  char buf[256];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  if (!strncmp(buf, "beep ", 5)) w.beeps++;
  rollTo(w.ms / 1000);
  char line[300];
  snprintf(line, sizeof(line), "%11.3f  %s\n", w.ms / 1000.0, buf);
  w.trace += line;
  if (w.dump) fputs(line, w.dump);
}

void draw(const char* target, const char* fmt, ...) {
  char buf[256];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  w.frameHash = fnv(fnv(fnv(w.frameUsed ? w.frameHash : FNV_SEED, target), " "), buf);
  w.frameUsed = true;
  if (!strcmp(target, "spr")) w.askedHash = fnv(fnv(w.askedHash, buf), "\n");
  if (w.watching) w.drawn.push_back(std::string(target) + " " + buf);
  if (w.dump) fprintf(w.dump, "%11.3f    %s %s\n", w.ms / 1000.0, target, buf);
}

void pushed() {
  rollTo(w.ms / 1000);
  w.frameHash = fnv(w.frameUsed ? w.frameHash : FNV_SEED, "push");
  w.frameUsed = true;
  foldFrame();
  w.frames++;
  if (w.dump) fprintf(w.dump, "%11.3f    push\n", w.ms / 1000.0);
}

uint64_t drawnSinceAsked() {
  uint64_t h = w.askedHash;
  w.askedHash = FNV_SEED;
  return h;
}

void armButtonWake() { w.buttonArmed = true; }
void armTimerWake(uint64_t us) { w.timerUs = us; }

static std::string nvsKey(const char* ns, const char* key) { return std::string(ns) + "/" + key; }

size_t nvsLen(const char* ns, const char* key) {
  auto it = w.nvs.kv.find(nvsKey(ns, key));
  return it == w.nvs.kv.end() ? 0 : it->second.size();
}

bool nvsRead(const char* ns, const char* key, void* out, size_t len) {
  auto it = w.nvs.kv.find(nvsKey(ns, key));
  if (it == w.nvs.kv.end() || it->second.size() != len) return false;
  memcpy(out, it->second.data(), len);
  return true;
}

// Only a value that changed is an event, as only that costs a flash write.
void nvsWrite(const char* ns, const char* key, const void* in, size_t len) {
  std::vector<uint8_t> v((const uint8_t*)in, (const uint8_t*)in + len);
  auto& slot = w.nvs.kv[nvsKey(ns, key)];
  if (slot == v) return;
  slot = v;
  if (len <= 4) {
    uint32_t n = 0;
    memcpy(&n, in, len);
    event("save %s=%u", key, (unsigned)n);
  } else {
    // The size only: a saved struct carries padding bytes that hold whatever
    // was in memory, and differ from run to run.
    event("save %s=<%u bytes>", key, (unsigned)len);
  }
}

void nvsClear(const char* ns) {
  std::string prefix = std::string(ns) + "/";
  for (auto it = w.nvs.kv.begin(); it != w.nvs.kv.end();)
    it = it->first.compare(0, prefix.size(), prefix) == 0 ? w.nvs.kv.erase(it) : std::next(it);
  event("save cleared");
}

}  // namespace hw

// --- The Stick a boot script drives ---------------------------------------------
uint32_t Stick::now() const { return w.ms; }
void Stick::frame() { ::loop(); }

void Stick::run(uint32_t ms) {
  uint32_t until = w.ms + ms;
  while ((int32_t)(w.ms - until) < 0) ::loop();
}

bool Stick::runUntil(const std::function<bool()>& done, uint32_t maxMs) {
  uint32_t until = w.ms + maxMs;
  while (!done()) {
    if ((int32_t)(w.ms - until) >= 0) return false;
    ::loop();
  }
  return true;
}

void Stick::press(Btn b) { w.btn[b] = true; }
void Stick::release(Btn b) { w.btn[b] = false; }
void Stick::tap(Btn b) { press(b); run(70); release(b); run(40); }
void Stick::hold(Btn b, uint32_t ms) { press(b); run(ms); release(b); run(40); }
void Stick::holdBoth(uint32_t ms) { press(A); press(B); run(ms); release(A); release(B); run(40); }

void Stick::pose(float x, float y, float z) { w.pose[0] = x; w.pose[1] = y; w.pose[2] = z; }
void Stick::heldToPlay() { pose(HELD[0], HELD[1], HELD[2]); }
void Stick::tilt(float right, float down) {
  // On the StickS3 screen-right is the sensor's -y and screen-down its -x.
  float v[3] = { HELD[0] - 0.6f * down, HELD[1] - 0.6f * right, HELD[2] };
  float len = (float)sqrt((double)(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]));
  pose(v[0] / len, v[1] / len, v[2] / len);
}
void Stick::faceDown() { pose(0, 0, -1); }
void Stick::still() { w.motion = STILL; }
void Stick::shake() { w.motion = SHAKE; }
void Stick::wiggle() { w.motion = WIGGLE; }
void Stick::sway() { w.motion = SWAY; }

int Stick::brightness() const { return w.bright; }
int Stick::beeps() const { return w.beeps; }
void Stick::watchDrawing() { w.watching = true; w.drawn.clear(); }
const std::vector<std::string>& Stick::drawn() const { return w.drawn; }

// --- Saved settings -------------------------------------------------------------
bool Nvs::has(const std::string& key) const { return kv.count(key) > 0; }

uint32_t Nvs::get(const std::string& key, uint32_t def) const {
  auto it = kv.find(key);
  if (it == kv.end() || it->second.size() > 4) return def;
  uint32_t n = 0;
  memcpy(&n, it->second.data(), it->second.size());
  return n;
}

void Nvs::set(const std::string& key, uint32_t v, size_t bytes) {
  kv[key] = std::vector<uint8_t>((const uint8_t*)&v, (const uint8_t*)&v + bytes);
}

// --- One boot, in a process of its own ------------------------------------------
namespace {

std::string oneLine(std::string s) {
  for (char& c : s) if (c == '\n') c = ' ';
  return s;
}

[[noreturn]] void runBoot(const Device& d, Wake wake, const std::function<void(Stick&)>& script, int fd) {
  w.nvs = d.nvs;
  w.rng = d.seed ? d.seed : 1;
  w.batMv = d.batteryMv;
  w.vbusMv = d.vbusMv;
  w.wake = (int)wake;
  w.btn[A] = d.blueHeldAtBoot;
  void*  nap;
  size_t napLen;
  probe::napMemory(&nap, &napLen);
  if (wake != Wake::Cold && d.napMemory.size() == napLen) memcpy(nap, d.napMemory.data(), napLen);
  if (const char* path = getenv("SIM_DUMP")) w.dump = fopen(path, "a");
  if (w.dump) fprintf(w.dump, "--- boot (wake %d) ---\n", (int)wake);

  const char* end = "running";
  std::string failure;
  try {
    ::setup();
    Stick s;
    if (script) script(s);
  } catch (const DeepSleep&) {
    end = "nap";
  } catch (const PowerOff&) {
    end = "off";
  } catch (const Failure& f) {
    failure = f.msg;
  } catch (const std::exception& e) {
    failure = std::string("exception: ") + e.what();
  }
  foldFrame();
  flushSecond();
  if (w.dump) fclose(w.dump);

  std::string out = std::string("END ") + end + " " + std::to_string(w.timerUs) + " " + (w.buttonArmed ? "1" : "0") + "\n";
  if (!failure.empty()) out += "FAIL " + oneLine(failure) + "\n";
  for (const auto& kv : w.nvs.kv) out += "NVS " + kv.first + " " + hexOf(kv.second) + "\n";
  out += "NAP " + hexOf(std::vector<uint8_t>((const uint8_t*)nap, (const uint8_t*)nap + napLen)) + "\n";
  out += "TRACE\n" + w.trace;
  for (size_t off = 0; off < out.size();) {
    ssize_t n = write(fd, out.data() + off, out.size() - off);
    if (n <= 0) break;
    off += (size_t)n;
  }
  close(fd);
  _exit(0);
}

}  // namespace

Boot Device::boot(Wake wake, const std::function<void(Stick&)>& script) {
  int fd[2];
  if (pipe(fd) != 0) throw Failure{ "pipe() failed" };
  fflush(stdout);
  fflush(stderr);
  pid_t pid = fork();
  if (pid < 0) throw Failure{ "fork() failed" };
  if (pid == 0) {
    close(fd[0]);
    runBoot(*this, wake, script, fd[1]);
  }
  close(fd[1]);
  std::string in;
  char buf[4096];
  for (ssize_t n; (n = read(fd[0], buf, sizeof(buf))) > 0;) in.append(buf, (size_t)n);
  close(fd[0]);
  int status = 0;
  waitpid(pid, &status, 0);

  Boot b;
  Nvs saved;
  bool ended = false;
  size_t pos = 0;
  while (pos < in.size()) {
    size_t nl = in.find('\n', pos);
    if (nl == std::string::npos) nl = in.size();
    std::string line = in.substr(pos, nl - pos);
    pos = nl + 1;
    if (line == "TRACE") { b.trace = pos < in.size() ? in.substr(pos) : ""; break; }
    std::istringstream ls(line);
    std::string tag;
    ls >> tag;
    if (tag == "END") {
      std::string how;
      int armed = 0;
      ls >> how >> b.napUs >> armed;
      b.end = how == "nap" ? End::Nap : how == "off" ? End::Off : End::Running;
      b.buttonArmed = armed != 0;
      ended = true;
    } else if (tag == "FAIL") {
      throw Failure{ line.substr(5) };
    } else if (tag == "NVS") {
      std::string key, hex;
      ls >> key >> hex;
      saved.kv[key] = unhex(hex);
    } else if (tag == "NAP") {
      std::string hex;
      ls >> hex;
      napMemory = unhex(hex);
    }
  }
  if (!ended) throw Failure{ "the simulated boot crashed (its output is above)" };
  nvs = saved;
  return b;
}

// --- Stored traces --------------------------------------------------------------
void golden(const std::string& name, const std::string& trace) {
  std::string path = "test/golden/" + name + ".txt";
  if (getenv("UPDATE_GOLDEN")) {
    std::ofstream(path, std::ios::binary) << trace;
    return;
  }
  std::ifstream f(path, std::ios::binary);
  if (!f) throw Failure{ path + " does not exist: run `make test-host-update`, then read it over" };
  std::stringstream ss;
  ss << f.rdbuf();
  std::string want = ss.str();
  if (want == trace) return;

  mkdir(".pio", 0755);
  mkdir(".pio/host", 0755);
  std::string actual = ".pio/host/" + name + ".actual.txt";
  std::ofstream(actual, std::ios::binary) << trace;
  std::istringstream a(want), b(trace);
  std::string la, lb;
  int n = 0;
  for (;;) {
    bool ha = (bool)std::getline(a, la), hb = (bool)std::getline(b, lb);
    n++;
    if (!ha && !hb) break;
    if (!ha) la = "<end>";
    if (!hb) lb = "<end>";
    if (la != lb) break;
  }
  throw Failure{ "trace differs from " + path + " at line " + std::to_string(n) + "\n       stored: " + la +
                 "\n       now:    " + lb + "\n       full trace: " + actual };
}

// --- Checks and the test list -----------------------------------------------------
void fail(const char* file, int line, const std::string& what) {
  const char* base = strrchr(file, '/');
  throw Failure{ std::string(base ? base + 1 : file) + ":" + std::to_string(line) + ": " + what };
}

std::vector<Test>& tests() {
  static std::vector<Test> all;
  return all;
}

}  // namespace sim

// `pet_tests [text]` runs the tests whose name contains text.
int main(int argc, char** argv) {
  const char* only = argc > 1 ? argv[1] : "";
  int ran = 0, failed = 0;
  for (const sim::Test& t : sim::tests()) {
    if (!strstr(t.name, only)) continue;
    ran++;
    try {
      t.fn();
      printf("ok    %s\n", t.name);
    } catch (const sim::Failure& f) {
      failed++;
      printf("FAIL  %s\n      %s\n", t.name, f.msg.c_str());
    }
  }
  printf("\n%d tests, %d failed\n", ran, failed);
  return failed || !ran ? 1 : 0;
}
