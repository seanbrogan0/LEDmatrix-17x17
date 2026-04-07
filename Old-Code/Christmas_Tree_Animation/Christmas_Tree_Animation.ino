
#include <FastLED.h>

#define DATA_PIN    3
#define MAT_WIDTH   17
#define MAT_HEIGHT  17
#define NUM_LEDS    (MAT_WIDTH * MAT_HEIGHT)

CRGB leds[NUM_LEDS];

// ------------------ XY MAPPING ------------------
uint16_t XY(uint8_t x, uint8_t y) {
  return (y & 1) ? (y * MAT_WIDTH) + ((MAT_WIDTH - 1) - x)
                 : (y * MAT_WIDTH) + x;
}
inline bool inBounds(int8_t x, int8_t y) {
  return (x >= 0 && x < MAT_WIDTH && y >= 0 && y < MAT_HEIGHT);
}

// ------------------ PINS ------------------
const int potBrightnessPin = A1; // Brightness
const int potSpeedPin      = A3; // Colour-transition speed
const uint8_t BTN_TREE     = 7;  // Short press toggles tree rainbow
const uint8_t BTN_BAUBLES  = 6;  // Short press freezes/resumes baubles

// ------------------ BRIGHTNESS SMOOTHING (moving average) ------------------
const int numReadings = 10;
int bRAY[numReadings];
int bRAYdex = 0;
int bRAYtot = 0;
int bRAYavg = 0;
byte MAXbright = 255;

// ------------------ SPEED SMOOTHING (same logic as brightness) ------------------
const int numSpeedReadings = 10;
int sRAY[numSpeedReadings];
int sRAYdex = 0;
int sRAYtot = 0;
int sRAYavg = 0;

// Speed controls hue change rate; we compute hue from time * rate
float hueRate = 0.20f; // hue units per millisecond; set by A3 each frame

// ------------------ BUTTON STATE ------------------
bool lastTreeBtn = HIGH;
bool lastBaubleBtn = HIGH;
const uint16_t debounceMs = 40;

// Features toggled by buttons
bool treeRainbow   = false; // BTN_TREE short press toggles this
bool colourFreeze  = false; // BTN_BAUBLES short press toggles this

// ------------------ BAUBLES ------------------
struct Bauble { uint8_t x, y; };
Bauble baubles[] = {
  {6, 4}, {10, 3},
  {7, 6}, {9, 5},
  {5, 7}, {10, 8},
  {6, 9}, {9, 10},
  {6, 13}, {10, 12},
  {9, 14}
};
const uint8_t BAUBLE_COUNT = sizeof(baubles) / sizeof(baubles[0]);

// Per-bauble phase offsets (for variety)
uint8_t baublePhase[BAUBLE_COUNT];

// ------------------ TREE & STAR ------------------
const CRGB TREE_COLOR = CRGB::Green;
const uint8_t STAR_X = 8, STAR_Y = 16;

// ------------------ HUE GUARD (optional, keeps contrast vs green tree) ------------------
// FastLED HSV: pure green ~96; gently nudge hues inside ~85..105 away from exact green
uint8_t nudgeAwayFromGreen(uint8_t h) {
  if (h >= 85 && h <= 105) {
    // Push toward cyan or yellow side so it doesn't blend with the tree
    return (h < 96) ? 80 : 120; // 80≈yellow/orange, 120≈cyan
  }
  return h;
}

// ------------------ SETUP ------------------
void setup() {
  delay(1000);
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 6000); // adjust to your PSU/wiring

  pinMode(BTN_TREE,    INPUT_PULLUP);
  pinMode(BTN_BAUBLES, INPUT_PULLUP);

  // Prime brightness smoothing
  bRAYtot = 0;
  for (int i = 0; i < numReadings; i++) {
    bRAY[i] = analogRead(potBrightnessPin);
    bRAYtot += bRAY[i];
  }
  bRAYavg = bRAYtot / numReadings;
  MAXbright = (bRAYavg > 1017) ? 255 : map(bRAYavg, 0, 1023, 15, 255);

  // Prime speed smoothing
  sRAYtot = 0;
  for (int i = 0; i < numSpeedReadings; i++) {
    sRAY[i] = analogRead(potSpeedPin);
    sRAYtot += sRAY[i];
  }
  sRAYavg = sRAYtot / numSpeedReadings;
  // Map averaged speed to a noticeable hue change rate (slow..fast)
  // 0.03..0.60 hue units per millisecond ≈ full cycle in ~8.5s down to ~0.4s
  hueRate = map(sRAYavg, 0, 1023, 3, 60) / 100.0f;

  // Initialize per-bauble phase offsets (spread across the circle)
  for (uint8_t i = 0; i < BAUBLE_COUNT; i++) {
    baublePhase[i] = (i * (255 / BAUBLE_COUNT)) % 255;
  }
}

// ------------------ LOOP ------------------
void loop() {
  readPots();
  handleButtons();

  fill_solid(leds, NUM_LEDS, CRGB::Black);

  drawTree();
  if (treeRainbow) animateTreeLightsRainbow(); // rainbow sweep only when toggled
  animateBaubles();    // single smooth colour-transition animation
  drawStar();          // simple twinkle

  FastLED.setBrightness(MAXbright);
  FastLED.show();
}

// ------------------ INPUT HANDLING ------------------
void readPots() {
  // Brightness smoothing
  bRAYtot -= bRAY[bRAYdex];
  bRAY[bRAYdex] = analogRead(potBrightnessPin);
  bRAYtot += bRAY[bRAYdex];
  bRAYdex = (bRAYdex + 1) % numReadings;
  bRAYavg = bRAYtot / numReadings;
  MAXbright = (bRAYavg > 1017) ? 255 : map(bRAYavg, 0, 1023, 15, 255);

  // Speed smoothing
  sRAYtot -= sRAY[sRAYdex];
  sRAY[sRAYdex] = analogRead(potSpeedPin);
  sRAYtot += sRAY[sRAYdex];
  sRAYdex = (sRAYdex + 1) % numSpeedReadings;
  sRAYavg = sRAYtot / numSpeedReadings;

  // Map averaged speed to hueRate (slow..fast). Adjust range to taste.
  hueRate = map(sRAYavg, 0, 1023, 3, 60) / 100.0f; // 0.03..0.60 hue/ms
}

// ------------------ BUTTONS ------------------
void handleButtons() {
  static uint32_t lastCheck = 0;
  uint32_t now = millis();
  if (now - lastCheck < debounceMs) return;
  lastCheck = now;

  bool t = digitalRead(BTN_TREE);
  bool b = digitalRead(BTN_BAUBLES);

  // Button 1: short press toggles tree between green and rainbow sweep
  if (t == LOW && lastTreeBtn == HIGH) {
    treeRainbow = !treeRainbow;
  }
  lastTreeBtn = t;

  // Button 2: single press freezes/resumes baubles animation
  if (b == LOW && lastBaubleBtn == HIGH) {
    colourFreeze = !colourFreeze;
  }
  lastBaubleBtn = b;
}

// ------------------ DRAW TREE ------------------
void drawTree() {
  // Trunk
  for (uint8_t y = 0; y < 2; y++)
    for (uint8_t x = 7; x <= 9; x++)
      leds[XY(x,y)] = CRGB::SaddleBrown;

  // Layer 1
  uint8_t xStart = 2, yStart = 2, width = 13;
  for (uint8_t y = yStart; y < yStart + 5; y++) {
    for (uint8_t x = xStart; x < xStart + width; x++)
      leds[XY(x, y)] = TREE_COLOR;
    width -= 2; xStart++;
  }

  // Layer 2
  xStart = 3; yStart = 7; width = 11;
  for (uint8_t y = yStart; y < yStart + 5; y++) {
    for (uint8_t x = xStart; x < xStart + width; x++)
      leds[XY(x, y)] = TREE_COLOR;
    width -= 2; xStart++;
  }

  // Layer 3
  xStart = 4; yStart = 12; width = 9;
  for (uint8_t y = yStart; y < MAT_HEIGHT; y++) {
    for (uint8_t x = xStart; x < xStart + width; x++)
      leds[XY(x, y)] = TREE_COLOR;
    width -= 2; xStart++;
  }
}

// ------------------ TREE RAINBOW SWEEP ------------------
void animateTreeLightsRainbow() {
  // Position-based hue + time modulation; respects green tree pixels
  uint8_t t = (millis() / 12) % 255;
  for (uint8_t y = 2; y < MAT_HEIGHT; y++) {
    for (uint8_t x = 2; x < MAT_WIDTH - 2; x++) {
      uint16_t idx = XY(x, y);
      if (leds[idx] == TREE_COLOR) {
        uint8_t h = (x * 10 + y * 5 + t) % 255;
        // keep saturation full, value a bit lower so baubles remain dominant
        leds[idx] = CHSV(h, 255, 200);
      }
    }
  }
}

// ------------------ BAUBLE ANIMATION (single) ------------------
void animateBaubles() {
  // Compute base hue from time * rate
  static uint32_t lastMillis = 0;
  uint32_t now = millis();
  uint32_t dt = now - lastMillis;
  lastMillis = now;

  static float huePos = 0.0f;
  if (!colourFreeze) {
    huePos += hueRate * dt;  // hue units
    // Wrap into [0..255)
    while (huePos >= 255.0f) huePos -= 255.0f;
    while (huePos < 0.0f)    huePos += 255.0f;
  }

  for (uint8_t i = 0; i < BAUBLE_COUNT; i++) {
    uint16_t idx = XY(baubles[i].x, baubles[i].y);

    // Per-bauble offset for variety
    uint8_t h = (uint8_t)fmodf(huePos + baublePhase[i], 255.0f);

    // Keep contrast vs tree by nudging away from exact green band (optional)
    h = nudgeAwayFromGreen(h);

    // Saturated, bright colours; smooth transition comes from huePos progression
    leds[idx] = CHSV(h, 255, 255);
  }
}

// ------------------ STAR ------------------
void drawStar() {
  static uint32_t lastChange = 0;
  static CRGB starColor = CRGB::Yellow;

  // Gentle twinkle independent of speed
  if (millis() - lastChange > 250) {
    CRGB target = (random8() < 90) ? CRGB::Yellow : CRGB::White;
    nblend(starColor, target, 32);
    lastChange = millis();
  }

  leds[XY(STAR_X, STAR_Y)] = starColor;
}
