#include <FastLED.h>

#include "config.h"
#include "matrix.h"
#include "effects.h"
#include "input.h"
#include "globals.h"

// ===== LED buffer =====
// Single ownership of the LED array lives here.
CRGB leds[NUM_LEDS];

void setup() {
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(DEFAULT_BRIGHTNESS);
  FastLED.clear(true);

  initInputs();
}

void loop() {
  updateInputs();
  runBouncingBall();
  FastLED.show();
}