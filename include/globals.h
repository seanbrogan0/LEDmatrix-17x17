#pragma once
#include <FastLED.h>
#include "config.h"

// ===== Global LED buffer =====
//
// The LED array is defined exactly once in main.cpp.
// All effects access it via this extern declaration.

extern CRGB leds[NUM_LEDS];