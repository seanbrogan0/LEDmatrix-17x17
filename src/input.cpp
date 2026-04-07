#include <Arduino.h>
#include <FastLED.h>

#include "input.h"

// ===== Pin assignments =====
const uint8_t BRIGHTNESS_PIN = A1;   // Left pot
const uint8_t SPEED_PIN      = A3;   // Right pot

// ===== Pot smoothing =====
const uint8_t numReadings = 10;

// Brightness pot
int brightRAY[numReadings];
uint8_t brightIndex = 0;
int brightTotal = 0;
int brightAvg = 0;

// Speed pot
int speedRAY[numReadings];
uint8_t speedIndex = 0;
int speedTotal = 0;
int speedAvg = 0;

// ===== Initialisation =====
void initInputs() {

  int bInit = analogRead(BRIGHTNESS_PIN);
  int sInit = analogRead(SPEED_PIN);

  brightTotal = 0;
  speedTotal  = 0;

  for (uint8_t i = 0; i < numReadings; i++) {
    brightRAY[i] = bInit;
    speedRAY[i]  = sInit;
    brightTotal += bInit;
    speedTotal  += sInit;
  }

  brightAvg = bInit;
  speedAvg  = sInit;
}

// ===== Update smoothing buffers =====
void updateInputs() {

  // --- Brightness pot ---
  brightTotal -= brightRAY[brightIndex];
  brightRAY[brightIndex] = analogRead(BRIGHTNESS_PIN);
  brightTotal += brightRAY[brightIndex];
  brightIndex++;
  if (brightIndex >= numReadings) brightIndex = 0;
  brightAvg = brightTotal / numReadings;

  // --- Speed pot ---
  speedTotal -= speedRAY[speedIndex];
  speedRAY[speedIndex] = analogRead(SPEED_PIN);
  speedTotal += speedRAY[speedIndex];
  speedIndex++;
  if (speedIndex >= numReadings) speedIndex = 0;
  speedAvg = speedTotal / numReadings;
}

// ===== Public accessors =====

// Map brightness with a lower limit so display never looks "off"
uint8_t getBrightness() {
  if (brightAvg > 1017) return 255;
  return map(brightAvg, 0, 1023, 32, 255);
}

// Map speed: higher pot value = faster animation
uint16_t getFrameDelayMs() {
  return map(speedAvg, 0, 1023, 60, 10);
}