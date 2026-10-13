#pragma once
// ---------------------------------------------------------------------------
// Hardware compatibility shim.
//
// The firmware was originally written against the board-specific
// `M5StickCPlus` library (ESP32 + AXP192 + MPU6886). That library does not
// support the ESP32-S3, so the project now builds against `M5Unified`, which
// supports BOTH the original M5StickC Plus and the newer M5StickS3
// (ESP32-S3 + M5PM1 PMIC + BMI270 IMU + ST7789P3 LCD + ES8311 audio) and
// auto-detects the board at runtime.
//
// To keep the render code untouched, this header re-creates the legacy
// display name it still uses (TFT_eSprite) on top of M5GFX.
// ---------------------------------------------------------------------------
#include <M5Unified.h>
#include <Arduino.h>

// --- Display surfaces -------------------------------------------------------
// TFT_eSprite -> M5Canvas (off-screen sprite, same drawing API).
using TFT_eSprite = M5Canvas;
