
#include <FastLED.h>

#define DATA_PIN 3

// ===== Matrix params =====
const uint8_t MatWidth  = 17;
const uint8_t MatHeight = 17;
#define NUM_LEDS (MatWidth * MatHeight)

CRGB leds[NUM_LEDS];

// ===== Serpentine XY mapper =====
uint16_t XY(uint8_t x, uint8_t y) {
  if (x >= MatWidth || y >= MatHeight) return 0; // safety
  if (y & 0x01) {
    uint8_t reverseX = (MatWidth - 1) - x;   // odd rows reversed
    return (y * MatWidth) + reverseX;
  } else {
    return (y * MatWidth) + x;               // even rows forward
  }
}

// ===== Pins =====
const uint8_t aVARPIN = A1;   // left pot: brightness
const uint8_t bVARPIN = A3;   // right pot: speed
const uint8_t BTN_BALL_PIN   = 6; // to GND when pressed
const uint8_t BTN_BOUND_PIN  = 7; // to GND when pressed

// ===== Pot smoothing =====
const int numReadings = 10;

int aRAY[numReadings];
int aRAYdex = 0;
int aRAYtot = 0;
int aRAYavg = 0;

int bRAY[numReadings];
int bRAYdex = 0;
int bRAYtot = 0;
int bRAYavg = 0;

// ===== Brightness =====
byte MAXbright = 255;

// ===== Bounce ball state =====
int xDIR = 1; // x direction (+1 / -1)
int yDIR = 1; // y direction
int d = 0;    // x position
int f = 2;    // y position

// ===== Timing =====
uint16_t frameDelayMs = 20; // base speed; will be modulated by A3

// ===== Button debounce (simple single-press) =====
const uint16_t DEBOUNCE_MS = 25;
uint32_t lastBallPressMs  = 0;
uint32_t lastBoundPressMs = 0;
bool prevBallLevel  = HIGH;  // not pressed (INPUT_PULLUP)
bool prevBoundLevel = HIGH;  // not pressed

// ===== Color palettes =====
const CRGB ballPalette[] = {
  CRGB::Red, CRGB::Orange, CRGB::Yellow, CRGB::Green,
  CRGB::Aqua, CRGB::Blue, CRGB::Purple, CRGB::HotPink, CRGB::White
};
const uint8_t BALL_COLORS = sizeof(ballPalette) / sizeof(ballPalette[0]);
uint8_t ballColorIndex = 0;
CRGB ballColor = ballPalette[ballColorIndex];

const CRGB boundaryPalette[] = {
  CRGB::Blue, CRGB::White, CRGB::Gold, CRGB::DeepSkyBlue,
  CRGB::LawnGreen, CRGB::Magenta, CRGB::OrangeRed
};
const uint8_t BOUND_COLORS = sizeof(boundaryPalette) / sizeof(boundaryPalette[0]);
uint8_t boundaryColorIndex = 0;
CRGB boundaryColor = boundaryPalette[boundaryColorIndex];

void setup() {
  delay(1000);
  Serial.begin(57600);

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 8000);
  FastLED.clear(true);

  // Buttons: internal pull-ups (active LOW when pressed to GND)
  pinMode(BTN_BALL_PIN, INPUT_PULLUP);
  pinMode(BTN_BOUND_PIN, INPUT_PULLUP);

  // ---- Initialize smoothing arrays with actual readings ----
  int aInit = analogRead(aVARPIN);
  int bInit = analogRead(bVARPIN);

  aRAYtot = 0;
  bRAYtot = 0;
  for (int i = 0; i < numReadings; i++) {
    aRAY[i] = aInit;
    bRAY[i] = bInit;
    aRAYtot += aInit;
    bRAYtot += bInit;
  }
  aRAYavg = aInit;
  bRAYavg = bInit;

  // Initialize previous button levels
  prevBallLevel  = digitalRead(BTN_BALL_PIN);
  prevBoundLevel = digitalRead(BTN_BOUND_PIN);
}

void loop() {
  READ_allpots();

  // Map A3 to speed (10..60 ms per frame)
  frameDelayMs = map(bRAYavg, 0, 1023, 60, 10);

  // Handle buttons (simple, single-press debounce)
  handleButtonsSimple();

  BounceBall();
  SendColours();

  delay(frameDelayMs);
}

// ===== Brightness & show =====
void SendColours() {
  if (aRAYavg > 1017) {
    MAXbright = 255;
  } else {
    // Keep a minimum so it never looks "off"
    MAXbright = map(aRAYavg, 0, 1023, 32, 255);
  }
  FastLED.setBrightness(MAXbright);
  FastLED.show();
}

// ===== Pot smoothing =====
void READ_allpots() {
  // A1
  aRAYtot -= aRAY[aRAYdex];
  aRAY[aRAYdex] = analogRead(aVARPIN);
  aRAYtot += aRAY[aRAYdex];
  aRAYdex++;
  if (aRAYdex >= numReadings) aRAYdex = 0;
  aRAYavg = aRAYtot / numReadings;

  // A3
  bRAYtot -= bRAY[bRAYdex];
  bRAY[bRAYdex] = analogRead(bVARPIN);
  bRAYtot += bRAY[bRAYdex];
  bRAYdex++;
  if (bRAYdex >= numReadings) bRAYdex = 0;
  bRAYavg = bRAYtot / numReadings;
}

// ===== Simple single-press debounce (INPUT_PULLUP) =====
void handleButtonsSimple() {
  uint32_t now = millis();

  // Ball button
  bool curBall = digitalRead(BTN_BALL_PIN); // LOW when pressed
  if (curBall != prevBallLevel) {
    prevBallLevel = curBall;
    if (curBall == LOW && (now - lastBallPressMs) > DEBOUNCE_MS) {
      lastBallPressMs = now;
      // single press: advance ball color
      ballColorIndex = (ballColorIndex + 1) % BALL_COLORS;
      ballColor = ballPalette[ballColorIndex];
    }
  }

  // Boundary button
  bool curBound = digitalRead(BTN_BOUND_PIN); // LOW when pressed
  if (curBound != prevBoundLevel) {
    prevBoundLevel = curBound;
    if (curBound == LOW && (now - lastBoundPressMs) > DEBOUNCE_MS) {
      lastBoundPressMs = now;
      // single press: advance boundary color
      boundaryColorIndex = (boundaryColorIndex + 1) % BOUND_COLORS;
      boundaryColor = boundaryPalette[boundaryColorIndex];
    }
  }
}

// ===== Bounce animation (correct inner limits at y = 1 and y = 15) =====
void BounceBall() {
  // Fade the entire frame first so static elements can be redrawn solid
  fadeToBlackBy(leds, NUM_LEDS, 40);

  // Draw top/bottom boundary lines (solid, not faded)
  for (uint8_t i = 0; i < MatWidth; i++) {
    leds[XY(i, 0)]  = boundaryColor;   // top boundary (y = 0)
    leds[XY(i, 16)] = boundaryColor;   // bottom boundary (y = 16)
  }

  // Draw ball
  leds[XY(d, f)] = ballColor;

  // Update position
  d += xDIR;
  f += yDIR;

  // --- Horizontal bounce (allow 0..16 inclusive) ---
  if (d > (MatWidth - 1)) {
    d = MatWidth - 1;
    xDIR = -xDIR;
  }
  if (d < 0) {
    d = 0;
    xDIR = -xDIR;
  }

  // --- Vertical bounce within inner limits 1..15 (between boundaries) ---
  if (f > (MatHeight - 2)) {   // crossed 15
    f = MatHeight - 2;         // clamp at 15
    yDIR = -yDIR;
  }
  if (f < 1) {                  // crossed 1
    f = 1;                      // clamp at 1
    yDIR = -yDIR;
  }
}
