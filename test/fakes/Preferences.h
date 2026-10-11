#pragma once
// Host stand-in for the ESP32 Preferences library, backed by the simulator's
// saved-settings store so values survive from one simulated boot to the next.
#include <string>

#include "Arduino.h"

class Preferences {
 public:
  bool begin(const char* ns, bool /*readOnly*/ = false) {
    _ns = ns;
    return true;
  }
  void end() {}
  bool clear() {
    sim::hw::nvsClear(_ns.c_str());
    return true;
  }
  bool   isKey(const char* key) { return sim::hw::nvsLen(_ns.c_str(), key) > 0; }
  size_t getBytesLength(const char* key) { return sim::hw::nvsLen(_ns.c_str(), key); }
  size_t getBytes(const char* key, void* out, size_t len) { return sim::hw::nvsRead(_ns.c_str(), key, out, len) ? len : 0; }
  size_t putBytes(const char* key, const void* in, size_t len) {
    sim::hw::nvsWrite(_ns.c_str(), key, in, len);
    return len;
  }

  bool     getBool(const char* key, bool def) { return get<uint8_t>(key, def) != 0; }
  uint8_t  getUChar(const char* key, uint8_t def) { return get<uint8_t>(key, def); }
  uint16_t getUShort(const char* key, uint16_t def) { return get<uint16_t>(key, def); }
  uint32_t getUInt(const char* key, uint32_t def) { return get<uint32_t>(key, def); }

  size_t putBool(const char* key, bool v) { return put<uint8_t>(key, v); }
  size_t putUChar(const char* key, uint8_t v) { return put<uint8_t>(key, v); }
  size_t putUShort(const char* key, uint16_t v) { return put<uint16_t>(key, v); }
  size_t putUInt(const char* key, uint32_t v) { return put<uint32_t>(key, v); }

 private:
  template <typename T>
  T get(const char* key, T def) {
    T v;
    return sim::hw::nvsRead(_ns.c_str(), key, &v, sizeof(v)) ? v : def;
  }
  template <typename T>
  size_t put(const char* key, T v) {
    sim::hw::nvsWrite(_ns.c_str(), key, &v, sizeof(v));
    return sizeof(v);
  }
  std::string _ns;
};
