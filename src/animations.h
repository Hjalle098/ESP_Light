#pragma once
#include <Adafruit_NeoPixel.h>

// Which look the lamp is currently showing.
enum LampMode { MODE_SOLID, MODE_FIRE, MODE_BREATHE };

// Shared lamp state. HomeKit (in main.cpp) writes it; the render loop reads it.
struct LampState {
  bool      power      = false;   // master on/off
  int       brightness = 100;     // 0-100 (HomeKit scale)
  float     hue        = 30.0f;   // 0-360 (HomeKit scale)
  float     sat        = 100.0f;  // 0-100 (HomeKit scale)
  LampMode  mode       = MODE_SOLID;
};

extern LampState lamp;

// Each renders exactly ONE frame and returns immediately (no delay()),
// reading color/brightness from the global `lamp`. Call from loop().
void animSolid  (Adafruit_NeoPixel& strip, int count);
void animFire   (Adafruit_NeoPixel& strip, int count);
void animBreathe(Adafruit_NeoPixel& strip, int count);
