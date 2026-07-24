#pragma once
#include <FastLED.h>
#include "config.h"

// ===== Global LED buffer =====
//
// The LED array is defined exactly once in main.cpp.
// All effects access it via this extern declaration.

extern CRGB leds[NUM_LEDS];

// ===== Shared effect workspace =====
//
// One scratch buffer, reused by whichever effect is currently running.
// Effects needing per-cell state (heat maps, cell grids, trail ages)
// must reinterpret this buffer instead of declaring their own static
// arrays — only one effect runs at a time, so private arrays would
// waste RAM that is already spoken for here.
//
// Contents are undefined when an effect first runs; every effect must
// initialise the region it uses.

extern uint8_t scratch[NUM_LEDS];