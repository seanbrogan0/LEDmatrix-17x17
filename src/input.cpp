#include <Arduino.h>

#include "input.h"

// ===== Pin assignments =====
const uint8_t BRIGHTNESS_PIN = A1;   // Left pot
const uint8_t SPEED_PIN      = A3;   // Right pot

// ===== Pot smoothing =====
//
// Exponential moving average held in an 8x-scaled accumulator:
// acc converges to 8 * reading, so (acc >> 3) is the smoothed value
// and reaches the pot's actual value exactly at steady state despite
// integer truncation. Max value 8 * 1023 fits in uint16_t.

static uint16_t brightAcc = 0;
static uint16_t speedAcc  = 0;

// ===== Initialisation =====
void initInputs() {
  brightAcc = (uint16_t)analogRead(BRIGHTNESS_PIN) << 3;
  speedAcc  = (uint16_t)analogRead(SPEED_PIN) << 3;
}

// ===== Update smoothing =====
void updateInputs() {
  brightAcc += analogRead(BRIGHTNESS_PIN) - (brightAcc >> 3);
  speedAcc  += analogRead(SPEED_PIN)      - (speedAcc >> 3);
}

// ===== Public accessors =====

// Map brightness with a lower limit so display never looks "off"
uint8_t getBrightness() {
  uint16_t avg = brightAcc >> 3;
  if (avg > 1017) return 255;
  return map(avg, 0, 1023, 32, 255);
}

// Map speed: higher pot value = faster animation
uint16_t getFrameDelayMs() {
  return map(speedAcc >> 3, 0, 1023, 60, 10);
}
