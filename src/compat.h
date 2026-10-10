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
// display names it still uses (TFT_eSprite, TFT_eSPI) on top of M5GFX.
// ---------------------------------------------------------------------------
#include <M5Unified.h>
#include <Arduino.h>

// --- Display surfaces -------------------------------------------------------
// TFT_eSprite -> M5Canvas (off-screen sprite, same drawing API).
// TFT_eSPI    -> lgfx::LGFXBase, the common base of both M5GFX (M5.Lcd /
//                M5.Display) and M5Canvas. buddyRenderTo() takes a pointer
//                to it so it can draw to either the sprite or the live LCD.
using TFT_eSprite = M5Canvas;
using TFT_eSPI    = lgfx::LGFXBase;

// --- Legacy bare color names (the old build pulled these from TFT_eSPI) -----
#ifndef GREEN
#define GREEN 0x07E0
#endif
#ifndef RED
#define RED 0xF800
#endif
