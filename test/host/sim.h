#pragma once
#include <cstdint>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

// A simulated stick for tests that run on the computer.
//
// A Device is what survives a power cycle: the saved settings, and the nap
// memory that lasts only as far as the next cold boot. Each call to
// boot() starts the real firmware from nothing in a process of its own, runs
// setup(), and hands the script a Stick to press buttons on and move about.
// The boot ends when the script returns, or when the firmware naps or powers
// off. What it saved comes back into the Device, ready for the next boot.
// A nap is two boots: one that ends in End::Nap, then one with Wake::Button.
//
//   sim::Device d = hatchedPet();
//   sim::Boot b = d.boot(sim::Wake::Cold, [](sim::Stick& s) {
//     s.run(61000);
//     CHECK_EQ(probe::screen(), "bed");
//   });
//
// Time is simulated: it only moves when the firmware calls delay(), so a
// minute of play takes milliseconds and every run is identical.

namespace sim {

enum Btn { A = 0, B = 1 };                 // A is the blue front button, B the side one
enum class Wake { Cold = 0, Button = 2, Timer = 4 };
enum class End { Running, Nap, Off };

// Only valid inside a boot script.
class Stick {
 public:
  uint32_t now() const;
  void frame();                            // one pass of loop()
  void run(uint32_t ms);
  bool runUntil(const std::function<bool()>& done, uint32_t maxMs);

  void press(Btn b);
  void release(Btn b);
  void tap(Btn b);
  void hold(Btn b, uint32_t ms);
  void holdBoth(uint32_t ms);

  // How the stick is being held and moved. pose() is the raw sensor reading
  // at rest; the others move it about that pose until the next call.
  void pose(float x, float y, float z);
  void heldToPlay();                       // the default: in the hands, top tipped up
  void tilt(float right, float down);      // from heldToPlay, 1 = a comfortable full tilt
  void faceDown();
  void still();
  void shake();                            // hard enough to count as a shake
  void wiggle();                           // walking, dancing
  void sway();                             // gentle rocking

  void battery(int mv);                    // the voltage from here on (read every 10 s)

  int brightness() const;
  int beeps() const;                       // since this boot started

  // Every drawing call from here on, oldest first, as "spr fillRect 0 0 8 8 FFFF".
  void watchDrawing();
  const std::vector<std::string>& drawn() const;
};

struct Nvs {
  std::map<std::string, std::vector<uint8_t>> kv;   // "pet/hunger" -> bytes
  bool     has(const std::string& key) const;
  uint32_t get(const std::string& key, uint32_t def = 0) const;
  void     set(const std::string& key, uint32_t v, size_t bytes = 1);
};

struct Boot {
  End         end = End::Running;
  uint64_t    napUs = 0;                   // timer armed before sleeping, 0 = none
  bool        buttonArmed = false;         // blue button armed to wake it
  std::string trace;
};

struct Device {
  Nvs      nvs;
  std::vector<uint8_t> napMemory;          // kept through a nap, gone on a cold boot
  uint32_t seed = 1;                       // the firmware's random numbers
  int      batteryMv = 3900;
  int      vbusMv = 0;
  bool     blueHeldAtBoot = false;         // the press that woke it is still down in setup()
  Boot boot(Wake wake = Wake::Cold, const std::function<void(Stick&)>& script = nullptr);
};

// Compare a trace with test/golden/<name>.txt. `make test-host-update`
// rewrites the stored files instead.
void golden(const std::string& name, const std::string& trace);

// --- Checks and the test list -------------------------------------------------
struct Failure { std::string msg; };
[[noreturn]] void fail(const char* file, int line, const std::string& what);

template <typename T>
std::string show(const T& v) {
  std::ostringstream s;
  if constexpr (std::is_integral_v<T> || std::is_enum_v<T>) s << (long long)v;
  else s << v;
  return s.str();
}

struct Test { const char* name; void (*fn)(); };
std::vector<Test>& tests();
struct Register {
  Register(const char* name, void (*fn)()) { tests().push_back({ name, fn }); }
};

}  // namespace sim

#define CHECK(cond) \
  do { if (!(cond)) sim::fail(__FILE__, __LINE__, #cond); } while (0)

#define CHECK_EQ(a, b) \
  do { \
    auto&& _a = (a); auto&& _b = (b); \
    if (!(_a == _b)) sim::fail(__FILE__, __LINE__, std::string(#a " == " #b "  (") + sim::show(_a) + " vs " + sim::show(_b) + ")"); \
  } while (0)

#define TEST(name) \
  static void test_##name(); \
  static sim::Register register_##name(#name, test_##name); \
  static void test_##name()
