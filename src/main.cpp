#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "HomeSpan.h"
#include "animations.h"

#define LED_PIN    17      // GPIO into the FIRST LED's DIN
#define LED_COUNT  20     // LEDs in the chain
#define FRAME_MS   15     // min ms between rendered frames (~66 FPS cap)

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Shared state written by the HomeKit services below, read by loop().
LampState lamp;

// On-characteristics of the animation switches, so turning one on can
// switch the others off in the Home app (mutually-exclusive modes).
#define NUM_MODES 2
static SpanCharacteristic* modeSwitches[NUM_MODES] = {nullptr, nullptr};

// ── HomeKit: the dimmable color light ────────────────────────
struct LampLight : Service::LightBulb {
  SpanCharacteristic* power;
  SpanCharacteristic* bright;
  SpanCharacteristic* hue;
  SpanCharacteristic* sat;

  LampLight() : Service::LightBulb() {
    power  = new Characteristic::On(false);
    bright = new Characteristic::Brightness(100);
    bright->setRange(0, 100, 1);          // 0-100%, 1% steps
    hue    = new Characteristic::Hue(30);
    sat    = new Characteristic::Saturation(100);
  }

  boolean update() override {
    lamp.power      = power->getNewVal();
    lamp.brightness = bright->getNewVal();
    lamp.hue        = hue->getNewVal<float>();
    lamp.sat        = sat->getNewVal<float>();
    return true;
  }
};

// ── HomeKit: one switch per animation mode ───────────────────
struct ModeSwitch : Service::Switch {
  SpanCharacteristic* on;
  LampMode myMode;
  int      myIndex;

  ModeSwitch(LampMode m, int idx) : Service::Switch() {
    myMode  = m;
    myIndex = idx;
    on      = new Characteristic::On(false);
    modeSwitches[idx] = on;
  }

  boolean update() override {
    if (on->getNewVal()) {
      lamp.mode = myMode;
      // Enforce one active mode: switch the others off in Home.
      for (int i = 0; i < NUM_MODES; i++)
        if (i != myIndex && modeSwitches[i]->getVal())
          modeSwitches[i]->setVal(false);
    } else if (lamp.mode == myMode) {
      lamp.mode = MODE_SOLID;             // back to solid color
    }
    return true;
  }
};

void setup() {
  Serial.begin(115200);

  strip.begin();
  strip.clear();
  strip.show();                          // start dark

  homeSpan.setPairingCode("46637726");   // Home pairing code 466-37-726
  homeSpan.begin(Category::Bridges, "ESP32 Lamp");

  // Accessory 1: the bridge itself.
  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      new Characteristic::Name("ESP32 Lamp");

  // Accessory 2: the color light.
  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      new Characteristic::Name("Lamp");
    new LampLight();

  // Accessory 3: Fire mode switch.
  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      new Characteristic::Name("Fire");
    new ModeSwitch(MODE_FIRE, 0);

  // Accessory 4: Breathe mode switch.
  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      new Characteristic::Name("Breathe");
    new ModeSwitch(MODE_BREATHE, 1);
}

void loop() {
  homeSpan.poll();                       // service HomeKit every iteration

  // Frame-rate cap so we don't hammer strip.show(); poll() still runs flat-out.
  static uint32_t lastFrame = 0;
  if (millis() - lastFrame < FRAME_MS) return;
  lastFrame = millis();

  // Clear the strip once when turned off, then idle.
  static bool prevPower = false;
  if (!lamp.power) {
    if (prevPower) { strip.clear(); strip.show(); prevPower = false; }
    return;
  }
  prevPower = true;

  strip.setBrightness(map(lamp.brightness, 0, 100, 0, 255));
  switch (lamp.mode) {
    case MODE_FIRE:    animFire(strip, LED_COUNT);    break;
    case MODE_BREATHE: animBreathe(strip, LED_COUNT); break;
    case MODE_SOLID:
    default:           animSolid(strip, LED_COUNT);   break;
  }
}
