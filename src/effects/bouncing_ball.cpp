// ===== Bounce Ball Effect =====

#include <Arduino.h>
#include <FastLED.h>

#include "effects.h"
#include "config.h"
#include "matrix.h"
#include "input.h"

// ===== Bounce ball state =====
static int xDIR = 1;   // +1 / -1 horizontal direction
static int yDIR = 1;   // +1 / -1 vertical direction
static int xPos = 0;   // Ball X position
static int yPos = 2;   // Ball Y position (start inside bounds)

// ===== Timing =====
static uint16_t frameDelayMs = 20;  // Frame delay (from input module)
static uint32_t lastFrameMs  = 0;

// ===== Color palettes =====
const CRGB ballPalette[] = {
  CRGB::Red, CRGB::Orange, CRGB::Yellow, CRGB::Green,
  CRGB::Aqua, CRGB::Blue, CRGB::Purple, CRGB::HotPink, CRGB::White
};

static uint8_t ballColorIndex = 0;
static CRGB ballColor = ballPalette[ballColorIndex];

// Boundary colour (solid, not faded)
static CRGB boundaryColor = CRGB::Blue;

// ===== Main effect entry point =====
void runBouncingBall() {
  uint32_t now = millis();

  // ----- Read inputs (via input module) -----
  uint8_t brightness = getBrightness();
  frameDelayMs       = getFrameDelayMs();

  FastLED.setBrightness(brightness);

  // ----- Frame timing -----
  if (now - lastFrameMs < frameDelayMs) return;
  lastFrameMs = now;

  // ----- Fade previous frame -----
  fadeToBlackBy(leds, NUM_LEDS, 40);

  // ----- Draw top and bottom boundaries -----
  for (uint8_t i = 0; i < MATRIX_WIDTH; i++) {
    leds[XY(i, 0)] = boundaryColor;                     // Top boundary
    leds[XY(i, MATRIX_HEIGHT - 1)] = boundaryColor;     // Bottom boundary
  }

  // ----- Draw ball -----
  leds[XY(xPos, yPos)] = ballColor;

  // ----- Update ball position -----
  xPos += xDIR;
  yPos += yDIR;

  // ----- Horizontal bounce (full width) -----
  if (xPos >= MATRIX_WIDTH - 1 || xPos <= 0) {
    xDIR = -xDIR;
    xPos += xDIR;
  }

  // ----- Vertical bounce (inside inner bounds 1..15) -----
  // Keeps the ball between the two solid boundary lines
  if (yPos >= MATRIX_HEIGHT - 2 || yPos <= 1) {
    yDIR = -yDIR;
    yPos += yDIR;
  }
}