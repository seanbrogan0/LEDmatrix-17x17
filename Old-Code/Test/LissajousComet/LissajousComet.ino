
#include <FastLED.h>
#include <math.h>

// ====== Hardware Config ======
#define LED_PIN       3
#define WIDTH         17
#define HEIGHT        17
#define NUM_LEDS      (WIDTH * HEIGHT)
#define COLOR_ORDER   GRB
#define CHIPSET       WS2812B
#define BRIGHTNESS    160

// Inputs
#define POT_SPEED_PIN A1  // speed control
#define POT_HUE_PIN   A3  // color shift
#define BTN_TRAIL_PIN 7   // toggles decay/trail style
#define BTN_PALETTE_PIN 6 // cycles color palettes

// ====== Globals ======
CRGB leds[NUM_LEDS];

bool serpentine = true;   // set false if your matrix is progressive (not zigzag)
uint8_t trailMode = 0;    // 0 = punchy/short; 1 = smooth/long
uint8_t paletteIndex = 0; // palette selector
CRGBPalette16 palettes[6];

uint16_t timeBase = 0;    // for subtle palette drift
uint8_t baseHueShift = 0; // adjustable color shift (A3 pot)

// Lissajous parameters
const float PI_F = 3.1415926f;
float t = 0.0f;           // time accumulator
float a = 3.0f;           // frequency X (e.g., 3)
float b = 2.0f;           // frequency Y (e.g., 2)
float phaseX = 0.0f;      // phase offset for X
float phaseY = PI_F / 2;  // phase offset for Y (90° gives nice loops)

// Simple button state (with basic debounce)
struct Button {
  uint8_t pin;
  bool lastState;        // logical pressed/unpressed
  uint32_t lastChangeMs;
} btnTrail, btnPalette;

// ====== Mapping ======
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

// ====== Buttons ======
bool readButton(Button &b) {
  bool rawPressed = (digitalRead(b.pin) == LOW); // active low
  uint32_t now = millis();
  bool fired = false;

  if (rawPressed != b.lastState && (now - b.lastChangeMs) > 25) {
    b.lastState = rawPressed;
    b.lastChangeMs = now;
    if (rawPressed) fired = true; // count press on transition to LOW
  }
  return fired;
}

// ====== Palettes ======
void initPalettes() {
  // Custom 4-color gradient (deep blues to aqua/white)
  palettes[0] = CRGBPalette16(
    CRGB::Black, CRGB::DarkBlue, CRGB::Aqua, CRGB::White
  );

  // Aurora-like gradient in HSV space
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

// ====== Trails ======
void clearWithTrail() {
  // Two trail styles:
  // 0) light decay for punchier comet
  // 1) slow fade for dreamy persistence
  uint8_t fadeAmount = (trailMode == 0) ? 48 : 12;  // tweak to taste
  fadeToBlackBy(leds, NUM_LEDS, fadeAmount);
}

// ====== Comet Drawing (anti-aliased) ======
void drawLissajousComet() {
  // Read controls
  uint16_t speedRaw = analogRead(POT_SPEED_PIN); // 0..1023
  uint16_t hueRaw   = analogRead(POT_HUE_PIN);   // 0..1023

  // dt controls how fast time advances; scale for usable range
  float dt = 0.0025f + (speedRaw / 1023.0f) * 0.030f; // ~0.0025..0.0325
  baseHueShift = map(hueRaw, 0, 1023, 0, 255);

  t += dt;

  // Lissajous parametric position in normalized space [-1, +1]
  float sx = sinf(a * t + phaseX);
  float sy = sinf(b * t + phaseY);

  // Map to pixel coordinates [0, WIDTH-1] and [0, HEIGHT-1]
  float xf = (sx * 0.5f + 0.5f) * (WIDTH - 1);
  float yf = (sy * 0.5f + 0.5f) * (HEIGHT - 1);

  // Bilinear weights for anti-aliased rendering to 4 nearest pixels
  int x0 = (int)floorf(xf);
  int y0 = (int)floorf(yf);
  int x1 = min(x0 + 1, WIDTH - 1);
  int y1 = min(y0 + 1, HEIGHT - 1);

  float fx = xf - x0;
  float fy = yf - y0;

  float w00 = (1.0f - fx) * (1.0f - fy);
  float w10 = fx * (1.0f - fy);
  float w01 = (1.0f - fx) * fy;
  float w11 = fx * fy;

  // Color: index drifts over time + user hue shift
  const CRGBPalette16 &pal = palettes[paletteIndex];
  uint8_t colorIndex = baseHueShift + (timeBase >> 2);
  CRGB head = ColorFromPalette(pal, colorIndex, 255, LINEARBLEND);

  // Draw comet head (additively) with weight-scaled brightness
  uint16_t i00 = XY(x0, y0);
  uint16_t i10 = XY(x1, y0);
  uint16_t i01 = XY(x0, y1);
  uint16_t i11 = XY(x1, y1);

  CRGB c00 = head; c00.nscale8_video((uint8_t)(w00 * 255));
  CRGB c10 = head; c10.nscale8_video((uint8_t)(w10 * 255));
  CRGB c01 = head; c01.nscale8_video((uint8_t)(w01 * 255));
  CRGB c11 = head; c11.nscale8_video((uint8_t)(w11 * 255));

  leds[i00] += c00;
  leds[i10] += c10;
  leds[i01] += c01;
  leds[i11] += c11;

  // Optional small halo for a "comet glow"
  // Sample a second color slightly dimmer
  CRGB halo = ColorFromPalette(pal, colorIndex + 8, 180, LINEARBLEND);
  halo.nscale8_video(180);

  // Spread a little glow around head via blur (below) and a few neighbors
  // (You can comment this out if you prefer only the head)
  if (trailMode == 1) {
    // Add a tiny extra glow by nudging neighboring pixels
    leds[i00] += halo;
    leds[i10] += halo;
    leds[i01] += halo;
    leds[i11] += halo;
  }

  // Global blur blends trail; heavier in smooth mode
  uint8_t blurAmount = (trailMode == 1) ? 20 : 8;
  blur2d(leds, WIDTH, HEIGHT, blurAmount);

  timeBase += 3; // mild palette drift
}

void setup() {
  delay(200);

  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setDither(true);
  FastLED.clear(true);

  pinMode(POT_SPEED_PIN, INPUT);
  pinMode(POT_HUE_PIN, INPUT);
  pinMode(BTN_TRAIL_PIN, INPUT_PULLUP);
  pinMode(BTN_PALETTE_PIN, INPUT_PULLUP);

  btnTrail   = { BTN_TRAIL_PIN, false, 0 };
  btnPalette = { BTN_PALETTE_PIN, false, 0 };

  random16_set_seed(analogRead(A0) ^ millis());
  initPalettes();
}

void loop() {
  // Buttons
  if (readButton(btnTrail)) {
    trailMode = (trailMode + 1) % 2;
  }
  if (readButton(btnPalette)) {
    paletteIndex = (paletteIndex + 1) % 6;
  }

  clearWithTrail();     // fade previous frame to create trail
  drawLissajousComet(); // draw new comet position
  FastLED.show();
  delay(10);            // small delay to keep CPU cool
}
