#include <FastLED.h>
#include "config.h"
#include "matrix.h"
#include "effects.h"

CRGB leds[NUM_LEDS];

void setup() {
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(DEFAULT_BRIGHTNESS);
  FastLED.clear(true);
}

void loop() {
  runBouncingBall();
  FastLED.show();
}