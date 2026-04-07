
/*
  17×17 LED Matrix – New Year's Show (UNO-safe, no lambdas)
  Animations: Fireworks, Confetti Rain, Text Scroll, Champagne Clink
  Board: Arduino Nano/Uno (ATmega328P)
  LEDs: WS2812B/NeoPixel (GRB)
  Matrix: 17 wide × 17 high, serpentine, origin at top-left (0,0)

  Controls:
    - DATA pin: 3
    - Button1 (mode next): 7 (wired to GND) -> INPUT_PULLUP
    - Button2 (autoplay toggle): 6 (wired to GND) -> INPUT_PULLUP
    - Pot A1: speed
    - Pot A3: brightness
*/

#include <Arduino.h>
#include <FastLED.h>

// -------------------- USER CONFIG --------------------
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define DATA_PIN    3

#define BTN_MODE    7   // next animation (to GND)
#define BTN_AUTO    6   // toggle autoplay (to GND)
#define POT_SPEED   A1
#define POT_BRIGHT  A3

#define WIDTH       17
#define HEIGHT      17
#define NUM_LEDS    (WIDTH * HEIGHT)

#define MAX_BRIGHT  255
#define MIN_BRIGHT  20

#ifndef TWO_PI
#define TWO_PI 6.28318530717958647692
#endif

CRGB leds[NUM_LEDS];

// -------------------- STATE --------------------
enum Mode { FIREWORKS, CONFETTI, TEXT_SCROLL, CHAMPAGNE, MODE_COUNT };
Mode mode = FIREWORKS;
bool autoplay = true;

uint32_t lastModeChange = 0;
uint32_t autoplayIntervalMs = 12000;

uint32_t nowMs = 0;

// Buttons (debounce)
uint32_t lastBtnModeMs = 0, lastBtnAutoMs = 0;
const uint32_t debounceMs = 180;

// Speed & brightness (smoothed)
float speedNorm = 0.6f;   // 0..1
uint8_t brightness = 160;
float smooth(float prev, float target, float alpha = 0.15f) { return prev + alpha * (target - prev); }


// --------------- MAPPING (SERPENTINE, BOTTOM-LEFT ORIGIN) ---------------
uint16_t XY(uint8_t x, uint8_t y) {
  // Flip the vertical axis
  y = (HEIGHT - 1) - y;

  // Serpentine rows
  if (y & 0x01) { // odd row: right-to-left
    return y * WIDTH + (WIDTH - 1 - x);
  } else {        // even row: left-to-right
    return y * WIDTH + x;
  }
}


inline void setPix(int x, int y, const CRGB &c) {
  if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) leds[XY(x, y)] = c;
}

// -------------------- RANDOM HELPERS --------------------
long irand(long a, long b) { return random(a, b + 1); } // inclusive
uint8_t urand8(uint8_t span) { return random8(span); }

// ======================================================
//                     FIREWORKS (Q8.8 fixed point)
// ======================================================
struct Particle {
  int16_t x, y;   // Q8.8 position
  int16_t vx, vy; // Q8.8 velocity
  uint8_t hue;
  uint8_t life;   // 0..255
  uint8_t active; // 0/1
};

#define MAX_PARTICLES 36  // safe for UNO/Nano SRAM
Particle parts[MAX_PARTICLES];
uint32_t lastExplosionMs = 0;
uint8_t gStepBase = 7; // gravity step in Q8.8 (≈0.027 px/frame)

void clearParticles() {
  for (uint8_t i = 0; i < MAX_PARTICLES; i++) parts[i].active = 0;
}

void spawnExplosion() {
  // Center-ish to avoid clipping
  int16_t cx = (int16_t)((irand(4, WIDTH - 5) << 8));   // Q8.8
  int16_t cy = (int16_t)((irand(5, HEIGHT - 8) << 8));  // Q8.8
  uint8_t baseHue = random8();
  // Create a small burst
  for (uint8_t i = 0; i < MAX_PARTICLES; i++) {
    // random angle & speed
    uint8_t a = random8(); // 0..255 -> angle
    uint16_t spd =  (uint16_t)((205 + urand8(150))); // ~0.8..1.4 px/frame
    int16_t ca = (int16_t)((int8_t)cos8(a) - 128); // -128..127
    int16_t sa = (int16_t)((int8_t)sin8(a) - 128);
    int16_t vx = (int16_t)(( (ca * (int16_t)spd) ) / 100);
    int16_t vy = (int16_t)(( (sa * (int16_t)spd) ) / 100);

    parts[i].x = cx;
    parts[i].y = cy;
    parts[i].vx = vx;
    parts[i].vy = vy;
    parts[i].hue = baseHue + random8(30);
    parts[i].life = irand(140, 240);
    parts[i].active = 1;
  }
}

void updateFireworks() {
  // Trails
  uint8_t fadeAmt = 28 + (uint8_t)(speedNorm * 40); // more fade at higher speed
  fadeToBlackBy(leds, NUM_LEDS, fadeAmt);

  // Gravity scales with speed
  uint8_t gStep = gStepBase + (uint8_t)(speedNorm * 12); // Q8.8 increment per frame

  // Spawn
  uint32_t coolMs = 1800 - (uint32_t)(speedNorm * 1200);
  if (nowMs - lastExplosionMs > coolMs) {
    spawnExplosion();
    lastExplosionMs = nowMs;
  }

  // Update and render
  for (uint8_t i = 0; i < MAX_PARTICLES; i++) {
    if (!parts[i].active) continue;

    parts[i].vy += gStep;         // gravity
    parts[i].x  += parts[i].vx;
    parts[i].y  += parts[i].vy;

    // Life fade
    parts[i].life = (parts[i].life > 2) ? parts[i].life - 2 : 0;
    if (parts[i].life == 0) { parts[i].active = 0; continue; }

    int ix = (parts[i].x + 128) >> 8; // round Q8.8
    int iy = (parts[i].y + 128) >> 8;

    if (ix >= 0 && ix < WIDTH && iy >= 0 && iy < HEIGHT) {
      CRGB c = CHSV(parts[i].hue, 200, qadd8(90, parts[i].life));
      leds[XY(ix, iy)] += c; // additive
      // occasional sparkle nearby
      if (random8() < 18) {
        int sx = ix + ((int8_t)(random8() % 3) - 1);
        int sy = iy + ((int8_t)(random8() % 3) - 1);
        setPix(sx, sy, CHSV(parts[i].hue, 150, 255));
      }
    }
  }
}

// ======================================================
//                    CONFETTI RAIN
// ======================================================
struct Drop {
  int16_t x, y;   // Q8.8
  int16_t vy;     // Q8.8
  uint8_t hue;
};

#define MAX_DROPS 24
Drop drops[MAX_DROPS];

void initConfetti() {
  for (uint8_t i = 0; i < MAX_DROPS; i++) {
    drops[i].x = (int16_t)(irand(0, WIDTH - 1) << 8);
    drops[i].y = (int16_t)(irand(-HEIGHT, 0) << 8);
    drops[i].vy = (int16_t)( (40 + urand8(60)) ); // 0.16..0.39 px/frame
    drops[i].hue = random8();
  }
}

void respawnDrop(uint8_t i) {
  drops[i].x = (int16_t)(irand(0, WIDTH - 1) << 8);
  drops[i].y = (int16_t)(irand(-HEIGHT / 2, 0) << 8);
  drops[i].vy = (int16_t)( (40 + urand8(60)) );
  drops[i].hue = random8();
}

void updateConfetti() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  // gentle wind sway
  int8_t wind = ((int8_t)sin8(nowMs >> 4) - 128) / 8; // small signed value

  for (uint8_t i = 0; i < MAX_DROPS; i++) {
    drops[i].y += drops[i].vy + (uint16_t)(10 + (uint16_t)(speedNorm * 40)); // gravity boost
    drops[i].x += wind; // sway

    int ix = (drops[i].x + 128) >> 8;
    int iy = (drops[i].y + 128) >> 8;

    // wrap / respawn
    if (iy >= HEIGHT) {
      // tiny flash at bottom
      for (int dx = -1; dx <= 1; dx++) setPix(ix + dx, HEIGHT - 1, CHSV(drops[i].hue, 200, 180));
      respawnDrop(i);
      continue;
    }
    if (ix < 0) { drops[i].x = (WIDTH - 1) << 8; ix = WIDTH - 1; }
    if (ix >= WIDTH) { drops[i].x = 0; ix = 0; }

    setPix(ix, iy, CHSV(drops[i].hue, 220, 255));
    setPix(ix, iy - 1, CHSV(drops[i].hue, 220, 140));
  }
}

// ======================================================
//                      TEXT SCROLL
// ======================================================
#define CHAR_W 5
#define CHAR_H 7

// Minimal 5x7 font for "HAPPY NEW YEAR 2026"
struct Glyph { char c; uint8_t rows[CHAR_H]; };
const Glyph font[] = {
  {' ', {0b00000,0b00000,0b00000,0b00000,0b00000,0b00000,0b00000}},
  {'A', {0b01110,0b10001,0b10001,0b11111,0b10001,0b10001,0b10001}},
  {'E', {0b11111,0b10000,0b10000,0b11110,0b10000,0b10000,0b11111}},
  {'H', {0b10001,0b10001,0b10001,0b11111,0b10001,0b10001,0b10001}},
  {'N', {0b10001,0b11001,0b10101,0b10011,0b10001,0b10001,0b10001}},
  {'P', {0b11110,0b10001,0b10001,0b11110,0b10000,0b10000,0b10000}},
  {'R', {0b11110,0b10001,0b10001,0b11110,0b10100,0b10010,0b10001}},
  {'W', {0b10001,0b10001,0b10101,0b10101,0b10101,0b11011,0b10001}},
  {'Y', {0b10001,0b01010,0b00100,0b00100,0b00100,0b00100,0b00100}},
  {'0', {0b01110,0b10001,0b10011,0b10101,0b11001,0b10001,0b01110}},
  {'2', {0b01110,0b10001,0b00001,0b00010,0b00100,0b01000,0b11111}},
  {'6', {0b01110,0b10000,0b11110,0b10001,0b10001,0b10001,0b01110}},
};
const char message[] = "  HAPPY NEW YEAR 2026  ";

bool getGlyph(char ch, uint8_t outRows[CHAR_H]) {
  for (uint8_t i = 0; i < sizeof(font) / sizeof(font[0]); i++) {
    if (font[i].c == ch) {
      for (uint8_t r = 0; r < CHAR_H; r++) outRows[r] = font[i].rows[r];
      return true;
    }
  }
  // fallback to space
  for (uint8_t r = 0; r < CHAR_H; r++) outRows[r] = 0;
  return false;
}

int scrollX = WIDTH;
uint32_t lastScrollMs = 0;

void updateText() {
  // subtle background gradient
  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      uint8_t hue = (uint8_t)(x * 9 + (nowMs >> 5));
      leds[XY(x, y)] = CHSV(hue, 30, 20); // very dim backdrop
    }
  }
  // Speed controls scroll step
  uint32_t stepMs = 55 + (uint32_t)((1.0f - speedNorm) * 140);
  if (nowMs - lastScrollMs >= stepMs) {
    scrollX--;
    lastScrollMs = nowMs;
  }

  // Draw the message across the matrix at vertical center
  int yTop = (HEIGHT - CHAR_H) / 2; // 5x7 font centered
  int xCursor = scrollX;

  for (size_t i = 0; i < strlen(message); i++) {
    uint8_t rows[CHAR_H];
    getGlyph(message[i], rows);

    for (int r = 0; r < CHAR_H; r++) {
      uint8_t rowBits = rows[r];
      for (int b = 0; b < CHAR_W; b++) {
        if (rowBits & (1 << (CHAR_W - 1 - b))) {
          int px = xCursor + b;
          int py = yTop + r;
          // shimmering warm-white/gold
          uint8_t v = 180 + (sin8(nowMs >> 4) >> 2);
          CRGB col = CHSV(20, 40, v);
          setPix(px, py, col);
        }
      }
    }
    xCursor += CHAR_W + 1; // 1px space
  }

  // Reset when fully off-screen
  int msgWidth = (CHAR_W + 1) * (int)strlen(message);
  if (scrollX < -msgWidth) scrollX = WIDTH;
}

// ======================================================
//                   CHAMPAGNE CLINK (no lambdas)
// ======================================================

// helper to map sin8 (0..255) into -2..+2 tilt
int tiltValue() {
  uint8_t s = sin8((uint8_t)(nowMs >> 3)); // slower oscillation
  // map 0..255 to -2..+2
  int t = map((int)s, 0, 255, -2, 2);
  // small speed contribution
  if (speedNorm > 0.7f) { t = constrain(t, -2, 2); }
  return t;
}

void drawGlass(int gx, int tiltPix, int gw, int gh, int gy) {
  // Rim
  for (int i = 0; i < gw; i++) setPix(gx + i + tiltPix, gy, CRGB(230, 230, 255));
  // Sides
  for (int y = 1; y < gh - 3; y++) {
    setPix(gx + tiltPix, gy + y, CRGB(200, 200, 255));
    setPix(gx + gw - 1 + tiltPix, gy + y, CRGB(200, 200, 255));
  }
  // Bowl base
  for (int i = 1; i < gw - 1; i++) setPix(gx + i + tiltPix, gy + gh - 3, CRGB(180, 180, 240));
  // Stem
  int sx = gx + gw / 2 + tiltPix;
  for (int y = gh - 3; y < gh; y++) setPix(sx, gy + y, CRGB(160, 160, 220));
  // Foot
  for (int i = -2; i <= 2; i++) setPix(sx + i, gy + gh, CRGB(160, 160, 220));

  // Fill liquid (sparkling gold)
  for (int y = gy + 2; y < gy + gh - 4; y++) {
    for (int x = gx + 1; x < gx + gw - 1; x++) {
      uint8_t v = 80 + (uint8_t)(sin8((uint8_t)(x * 14 + y * 9 + (nowMs >> 3))) / 5);
      leds[XY(x + tiltPix, y)] = CHSV(20, 80, v);
    }
  }
}

void drawBubbles(int gx, int tiltPix, int gw, int gh, int gy) {
  for (uint8_t i = 0; i < 10; i++) {
    int bx = gx + 1 + tiltPix + urand8((uint8_t)(max(1, gw - 2)));
    int by = gy + gh - 5 - urand8((uint8_t)max(1, gh - 7));
    setPix(bx, by, CHSV(32, 40, 220));
    setPix(bx, by - 1, CHSV(32, 40, 120));
  }
}

void updateChampagne() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  int gw = 5, gh = 12;  // glass width/height
  int gy = 3;           // top y
  int lx = 3;
  int rx = WIDTH - 3 - gw;

  // gentle opposing tilt
  int tilt = tiltValue(); // -2..+2
  int leftTilt = -tilt;
  int rightTilt = tilt;

  drawGlass(lx, leftTilt, gw, gh, gy);
  drawGlass(rx, rightTilt, gw, gh, gy);

  // "Clink" sparkle between rims
  int clx = (lx + leftTilt) + gw;     // right edge of left rim
  int crx = (rx + rightTilt) - 1;     // left edge of right rim
  int midx = (clx + crx) / 2;
  int midy = gy;
  if ((nowMs / 600) % 2 == 0) {
    setPix(midx, midy, CRGB::White);
    setPix(midx - 1, midy + 1, CRGB::White);
    setPix(midx + 1, midy + 1, CRGB::White);
  }

  // Sparkling bubbles (randomized each frame)
  drawBubbles(lx, leftTilt, gw, gh, gy);
  drawBubbles(rx, rightTilt, gw, gh, gy);
}

// ======================================================
//                 INPUT & MODE HANDLING
// ======================================================
void readControls() {
  // Brightness (A3)
  int rawB = analogRead(POT_BRIGHT);
  uint8_t targetB = map(rawB, 0, 1023, MIN_BRIGHT, MAX_BRIGHT);
  brightness = (uint8_t)smooth(brightness, targetB, 0.2f);
  FastLED.setBrightness(brightness);

  // Speed (A1)
  int rawS = analogRead(POT_SPEED);
  float targetS = constrain((float)rawS / 1023.0f, 0.0f, 1.0f);
  speedNorm = smooth(speedNorm, targetS, 0.18f);

  // Buttons (active LOW)
  if (digitalRead(BTN_MODE) == LOW && (nowMs - lastBtnModeMs) > debounceMs) {
    mode = (Mode)((mode + 1) % MODE_COUNT);
    lastBtnModeMs = nowMs;
    lastModeChange = nowMs;
  }
  if (digitalRead(BTN_AUTO) == LOW && (nowMs - lastBtnAutoMs) > debounceMs) {
    autoplay = !autoplay;
    lastBtnAutoMs = nowMs;
  }
}

void maybeAutoplay() {
  uint32_t baseMs = 12000;
  autoplayIntervalMs = baseMs + (uint32_t)((1.0f - speedNorm) * 8000);
  if (autoplay && (nowMs - lastModeChange) > autoplayIntervalMs) {
    mode = (Mode)((mode + 1) % MODE_COUNT);
    lastModeChange = nowMs;
    // small cleanup per mode switch
    if (mode == FIREWORKS) clearParticles();
    if (mode == CONFETTI) initConfetti();
  }
}

// ======================================================
//                  SETUP & MAIN LOOP
// ======================================================
void setup() {
  randomSeed(analogRead(A0)); // free analog noise as entropy
  pinMode(BTN_MODE, INPUT_PULLUP);
  pinMode(BTN_AUTO, INPUT_PULLUP);

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.clear(true);
  FastLED.setBrightness(brightness);

  clearParticles();
  initConfetti();
}

void loop() {
  nowMs = millis();
  readControls();
  maybeAutoplay();

  switch (mode) {
    case FIREWORKS:   updateFireworks(); break;
    case CONFETTI:    updateConfetti();  break;
    case TEXT_SCROLL: updateText();      break;
    case CHAMPAGNE:   updateChampagne(); break;
    default:          fill_solid(leds, NUM_LEDS, CRGB::Black); break;
  }

  FastLED.show();

  // Frame pacing based on speedNorm (faster → lower delay)
  uint16_t baseDelay = 18;
  uint16_t extra = (uint16_t)((1.0f - speedNorm) * 32);
  delay(baseDelay + extra);
}
