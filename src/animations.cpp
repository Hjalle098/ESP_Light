#include "animations.h"
#include <Arduino.h>
#include <math.h>

// Convert the lamp's HomeKit hue/saturation into a packed RGB color.
// `value` (0-255) lets callers dim the color for effects like breathing;
// the global strip brightness is applied separately at show() time.
static uint32_t lampColor(Adafruit_NeoPixel& strip, uint8_t value = 255) {
    uint16_t h = (uint16_t)(lamp.hue / 360.0f * 65535.0f);
    uint8_t  s = (uint8_t)(lamp.sat / 100.0f * 255.0f);
    return strip.gamma32(strip.ColorHSV(h, s, value));
}

// Solid color across the whole strip.
void animSolid(Adafruit_NeoPixel& strip, int count) {
    uint32_t c = lampColor(strip);
    for (int i = 0; i < count; i++) strip.setPixelColor(i, c);
    strip.show();
}

// Smooth sine "breathing" of the chosen color, ~4s per cycle. Non-blocking:
// the brightness is derived from millis() each call instead of a delay loop.
void animBreathe(Adafruit_NeoPixel& strip, int count) {
    float phase = (millis() % 4000) / 4000.0f * 2.0f * (float)PI;
    float f = (sinf(phase - (float)PI / 2.0f) + 1.0f) / 2.0f;  // 0..1
    uint32_t c = lampColor(strip, (uint8_t)(f * 255.0f));
    for (int i = 0; i < count; i++) strip.setPixelColor(i, c);
    strip.show();
}

// One frame of a flickering flame. Fixed warm-orange hue (ignores the color
// picker on purpose). Self-paced with millis() so it never blocks HomeKit.
void animFire(Adafruit_NeoPixel& strip, int count) {
    static int heat = 255, target = 255, counter = 0;
    static uint32_t lastFrame = 0, frameGap = 20;

    // Irregular timing makes the flicker livelier — without blocking.
    if (millis() - lastFrame < frameGap) return;
    lastFrame = millis();
    frameGap  = random(10, 45);

    // Occasionally pick a new target heat: mostly bright flicker, rare dips.
    if (counter <= 0) {
        target  = (random(0, 100) < 15) ? random(10, 80) : random(160, 255);
        counter = random(1, 10);
    }
    counter--;
    heat += (target - heat) / 15;   // ease toward target

    const int FLAME_R = 255, FLAME_G = 80, FLAME_B = 0;
    for (int j = 0; j < count; j++) {
        int h = constrain(heat + random(-30, 31), 0, 255);
        strip.setPixelColor(j, strip.Color(FLAME_R * h / 255,
                                           FLAME_G * h / 255,
                                           FLAME_B * h / 255));
    }
    strip.show();
}
