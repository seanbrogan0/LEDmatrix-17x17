
#include <FastLED.h>

// ====== Hardware Config ======
#define LED_PIN       3
#define WIDTH         17
#define HEIGHT        17
#define NUM_LEDS      (WIDTH * HEIGHT)
#define COLOR_ORDER   GRB
#define CHIPSET       WS2812B
#define BRIGHTNESS    160  // You can cap global brightness here

// Inputs
#define POT_SPEED_PIN A1  // speed control
#define POT_HUE_PIN   A3  // color shift
#define BTN_TRAIL_PIN 7   // toggles decay/trail style
#define BTN_PALETTE_PIN 6 // cycles palettes

// ====== Globals ======
CRGB leds[NUM_LEDS];

bool serpentine = true;   // set to false if your matrix is wired progressive
uint8_t trailMode = 0;    // 0 = light decay (fast), 1 = slow fade for smoother trails
uint8_t paletteIndex = 0; // which palette to use
CRGBPalette16 palettes[6];

// Metaball parameters
const uint8_t BALLS = 3;      // Number of blobs
float bx[BALLS], by[BALLS];   // Positions
float vx[BALLS], vy[BALLS];   // Velocities (units: cells per frame base)
uint16_t timeBase = 0;        // used for palette animation
uint8_t baseHueShift = 0;

// Simple button state (with basic debounce)
struct Button {
  uint8_t pin;
  bool lastState;        // store as logical pressed/unpressed
  uint32_t lastChangeMs;
} btnTrail, btnPalette;

uint16_t XY(uint8_t x, uint8_t y) {
  if (x >= WIDTH || y >= HEIGHT) return 0;
  if (serpentine) {
    if (y & 0x01) {
      // odd rows go right->left
      return (y * WIDTH) + (WIDTH - 1 - x);
    } else {
      // even rows go left->right
      return (y * WIDTH) + x;
    }
  } else {
    return (y * WIDTH) + x;
  }
}

// Helper to read a button with basic debounce
bool readButton(Button &b) {
  bool rawPressed = (digitalRead(b.pin) == LOW); // active low
  uint32_t now = millis();
  bool fired = false;

  if (rawPressed != b.lastState && (now - b.lastChangeMs) > 25) {
    b.lastState = rawPressed;
    b.lastChangeMs = now;
    if (rawPressed) fired = true; // count rising edge of press (to LOW)
  }
  return fired;
}

// Make a few nice palettes (all valid constructors)
void initPalettes() {
  // Custom 4-color gradients
  palettes[0] = CRGBPalette16(  // Deep blues to aqua/white
    CRGB::Black, CRGB::DarkBlue, CRGB::Aqua, CRGB::White
  );

  // Aurora-like gradient in HSV space → convert via CHSV
  // (Still using CRGBPalette16 with 4 colors after conversion)
  palettes[1] = CRGBPalette16(
    CHSV(96, 255, 30),   // greenish
    CHSV(128, 200, 180), // cyan
    CHSV(160, 180, 240), // blue-cyan
    CRGB::White
  );

  // Built-in palettes
  palettes[2] = HeatColors_p;
  palettes[3] = OceanColors_p;
  palettes[4] = ForestColors_p;
  palettes[5] = RainbowColors_p;
}

void setupBalls() {
  // Seed balls at different points with gentle velocities
  for (uint8_t i = 0; i < BALLS; i++) {
    bx[i] = random(0, WIDTH);
    by[i] = random(0, HEIGHT);
    // Velocity base; we scale it each frame by speed knob
    vx[i] = (random(0, 200) - 100) / 200.0f; // ~ -0.5..+0.5
    vy[i] = (random(0, 200) - 100) / 200.0f;
  }
}

void bounceBalls() {
  // Keep balls in bounds and bounce on edges
  for (uint8_t i = 0; i < BALLS; i++) {
    if (bx[i] < 0) { bx[i] = 0; vx[i] = -vx[i]; }
    if (bx[i] > (WIDTH - 1)) { bx[i] = WIDTH - 1; vx[i] = -vx[i]; }
    if (by[i] < 0) { by[i] = 0; vy[i] = -vy[i]; }
    if (by[i] > (HEIGHT - 1)) { by[i] = HEIGHT - 1; vy[i] = -vy[i]; }
  }
}

void clearWithTrail() {
  // Two trail modes:
  // 0) light decay for punchier blobs
  // 1) slow fade for smoother persistence
  if (trailMode == 0) {
    // Scale down existing pixels quickly
    fadeToBlackBy(leds, NUM_LEDS, 40);
  } else {
    // Very slow fade
    fadeToBlackBy(leds, NUM_LEDS, 10);
  }
}

// Compute the metaballs field and colorize
void drawMetaballs() {
  // Read controls
  uint16_t speedRaw = analogRead(POT_SPEED_PIN); // 0..1023
  uint16_t hueRaw   = analogRead(POT_HUE_PIN);
  // Map speed: slow to fast. Also use for per-frame dt scaling
  float speedScale = 0.15f + (speedRaw / 1023.0f) * 1.35f; // 0.15 .. 1.5
  baseHueShift = map(hueRaw, 0, 1023, 0, 255);

  // Move balls
  for (uint8_t i = 0; i < BALLS; i++) {
    bx[i] += vx[i] * speedScale;
    by[i] += vy[i] * speedScale;
  }
  bounceBalls();

  // Slight drift to velocities for organic motion
  for (uint8_t i = 0; i < BALLS; i++) {
    vx[i] += (random(-2, 3)) * 0.003f;  // small random walk
    vy[i] += (random(-2, 3)) * 0.003f;
    // clamp velocities to avoid runaway
    if (vx[i] > 1.2f) vx[i] = 1.2f; if (vx[i] < -1.2f) vx[i] = -1.2f;
    if (vy[i] > 1.2f) vy[i] = 1.2f; if (vy[i] < -1.2f) vy[i] = -1.2f;
  }

  clearWithTrail();

  // Field strength and color
  // Each pixel accumulates "influence" from each ball ~ 1 / distance^2
  // Then we map that to a color value from the selected palette
  const CRGBPalette16 &pal = palettes[paletteIndex];
  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      float field = 0.0f;

      for (uint8_t i = 0; i < BALLS; i++) {
        float dx = (float)x - bx[i];
        float dy = (float)y - by[i];
        float d2 = dx*dx + dy*dy + 0.0001f; // avoid div by zero
        field += 1.0f / d2;  // stronger when closer
      }

      // Normalize and scale field to 0..255
      // The constant here tunes "blob size" and contrast
      float scaled = field * 140.0f;       // tweak for your taste
      if (scaled > 255.0f) scaled = 255.0f;
      uint8_t v = (uint8_t)scaled;

      // Use both value and time for palette index
      uint8_t colorIndex = v + baseHueShift + (timeBase >> 2);
      CRGB c = ColorFromPalette(pal, colorIndex, v, LINEARBLEND);

      // Additively blend for a glowy look
      leds[XY(x, y)] += c;
    }
  }

  // Subtle global blur helps blend
  if (trailMode == 1) {
    // heavier blur when in smooth mode
    blur2d(leds, WIDTH, HEIGHT, 20);
  } else {
    blur2d(leds, WIDTH, HEIGHT, 8);
  }

  timeBase += 3; // mild palette drift
}

void setup() {
  delay(200);
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear(true);

  pinMode(POT_SPEED_PIN, INPUT);
  pinMode(POT_HUE_PIN, INPUT);
  pinMode(BTN_TRAIL_PIN, INPUT_PULLUP);
  pinMode(BTN_PALETTE_PIN, INPUT_PULLUP);

  // Initialize button structs: not pressed (false)
  btnTrail = { BTN_TRAIL_PIN, false, 0 };
  btnPalette = { BTN_PALETTE_PIN, false, 0 };

  random16_set_seed(analogRead(A0) ^ millis());
  initPalettes();
  setupBalls();
}

void loop() {
  // Handle buttons
  if (readButton(btnTrail)) {
    trailMode = (trailMode + 1) % 2;
  }
  if (readButton(btnPalette)) {
    paletteIndex = (paletteIndex + 1) % 6;
  }

  drawMetaballs();
  FastLED.show();
  // Small frame delay; speed is handled internally per frame,
  // but this helps avoid pegging the MCU at 100%.
  delay(10);
}
