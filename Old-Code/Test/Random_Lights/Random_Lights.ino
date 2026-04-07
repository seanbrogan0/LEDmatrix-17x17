
#include <FastLED.h>

#define DATA_PIN 3

//////////////////////////////////////////////////
// MATRIX

const uint8_t MatWidth  = 17;
const uint8_t MatHeight = 17;
#define NUM_LEDS (MatWidth * MatHeight)

CRGB leds[NUM_LEDS];

// Serpentine XY mapper
uint16_t XY(uint8_t x, uint8_t y) {
  uint16_t i;
  if (y & 0x01) {
    uint8_t reverseX = (MatWidth - 1) - x;
    i = (y * MatWidth) + reverseX;
  } else {
    i = (y * MatWidth) + x;
  }
  return i;
}

//////////////////////////////////////////////////
// CONTROLS

const int BRIGHT_POT_PIN = A1;   // brightness pot (existing)
const int SPEED_POT_PIN  = A3;   // NEW: speed pot

const int BTN_NEXT_PIN   = 7;    // NEW: next mode (single press)
const int BTN_PREV_PIN   = 6;    // NEW: previous mode (single press)

//////////////////////////////////////////////////
// VARIABLES

// Brightness smoothing
const int numReadings = 10;
int bRAY[numReadings];
int bRAYdex = 0;
int bRAYtot = 0;
int bRAYavg = 0;
byte MAXbright = 255;

// Mode
byte MODE = 1;                   // 1..3
const byte MODE_MIN = 1;
const byte MODE_MAX = 3;

// Speed control
uint16_t frameDelayMs = 50;      // default; will be set by A3
const uint16_t FRAME_MIN_MS = 5;   // fastest
const uint16_t FRAME_MAX_MS = 300; // slowest
unsigned long lastFrameMs = 0;

// Button debouncing / edge detection (active-low)
bool btnNextPrevStateLast[2] = {true, true}; // true = HIGH (not pressed)
unsigned long btnDebounceMs[2] = {0, 0};
const unsigned long DEBOUNCE_MS = 25;

void setup() {
  delay(1000);
  Serial.begin(57600);

  // Seed RNG (use pot entropy)
  random16_add_entropy(analogRead(BRIGHT_POT_PIN));
  random16_add_entropy(analogRead(SPEED_POT_PIN));

  // FastLED setup
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 8000);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setDither(1);

  // Prime brightness smoothing buffer
  for (int i = 0; i < numReadings; i++) {
    bRAY[i] = analogRead(BRIGHT_POT_PIN);
    bRAYtot += bRAY[i];
  }

  // Buttons: pull-ups, active-low
  pinMode(BTN_NEXT_PIN, INPUT_PULLUP);
  pinMode(BTN_PREV_PIN, INPUT_PULLUP);
}

void loop() {
  // Read inputs continuously
  READ_brightness_pot();
  READ_speed_pot();
  HANDLE_buttons();

  // Auto mode change every 90 seconds (still allowed)
  EVERY_N_SECONDS(90) {
    Change_MODE_random();
  }

  // Frame pacing via speed pot (non-blocking)
  unsigned long now = millis();
  if (now - lastFrameMs >= frameDelayMs) {
    lastFrameMs = now;

    // Draw current mode
    switch (MODE) {
      case 1: DRAW_random();    break;
      case 2: DRAW_randomROW(); break;
      case 3: DRAW_randomCOL(); break;
    }

    // Show with brightness
    SendColours();
  }
}

//////////////////////////////////////////////////////////////////////////////////////
// INPUTS

void READ_brightness_pot() {
  bRAYtot -= bRAY[bRAYdex];
  bRAY[bRAYdex] = analogRead(BRIGHT_POT_PIN);
  bRAYtot += bRAY[bRAYdex];
  bRAYdex++;
  if (bRAYdex >= numReadings) bRAYdex = 0;
  bRAYavg = bRAYtot / numReadings;
}

void READ_speed_pot() {
  int raw = analogRead(SPEED_POT_PIN); // 0..1023
  // Map pot to a usable frame delay (fast to slow)
  // Ensure a small floor so FastLED.show has time and avoids flicker
  frameDelayMs = map(raw, 0, 1023, FRAME_MIN_MS, FRAME_MAX_MS);
}

void HANDLE_buttons() {
  // Index 0: NEXT (pin 7), Index 1: PREV (pin 6)
  const int pins[2] = {BTN_NEXT_PIN, BTN_PREV_PIN};
  for (int i = 0; i < 2; i++) {
    bool reading = digitalRead(pins[i]); // HIGH=not pressed, LOW=pressed
    unsigned long now = millis();

    // Simple debounce on state change
    if (reading != btnNextPrevStateLast[i]) {
      btnDebounceMs[i] = now;            // reset debounce timer
      btnNextPrevStateLast[i] = reading; // update last seen state
    } else {
      // Stable state; check if pressed and past debounce
      if (reading == LOW && (now - btnDebounceMs[i]) >= DEBOUNCE_MS) {
        // Edge: act on press, then wait until release to avoid repeats
        if (i == 0) {
          MODE = (MODE < MODE_MAX) ? MODE + 1 : MODE_MIN; // NEXT
        } else {
          MODE = (MODE > MODE_MIN) ? MODE - 1 : MODE_MAX; // PREV
        }
        FastLED.clear();
        // Wait for release before allowing another press
        while (digitalRead(pins[i]) == LOW) {
          // keep loop responsive: update brightness/speed during hold
          READ_brightness_pot();
          READ_speed_pot();
          delay(1);
        }
        // small post-release settle
        delay(DEBOUNCE_MS);
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////////////////
// RENDER & UTILS

void SendColours() {
  // Map pot to brightness
  if (bRAYavg > 1017) {
    MAXbright = 255;
  } else {
    MAXbright = map(bRAYavg, 0, 1023, 15, 255);
  }
  FastLED.setBrightness(MAXbright);
  FastLED.show();
}

void DRAW_random() {
  // Safe random index within 0..NUM_LEDS-1
  leds[random16(NUM_LEDS)] = CRGB(random8(), random8(), random8());
  // Optional gentle decay for sparkle
  // fadeToBlackBy(leds, NUM_LEDS, 1);
}

void DRAW_randomROW() {
  uint8_t R = random8(), G = random8(), B = random8();
  uint8_t y = random(MatHeight);
  for (uint8_t x = 0; x < MatWidth; x++) {
    leds[XY(x, y)] = CRGB(R, G, B);
  }
  fadeToBlackBy(leds, NUM_LEDS, 5);
}

void DRAW_randomCOL() {
  uint8_t R = random8(), G = random8(), B = random8();
  uint8_t x = random(MatWidth);
  for (uint8_t y = 0; y < MatHeight; y++) {
    leds[XY(x, y)] = CRGB(R, G, B);
  }
  fadeToBlackBy(leds, NUM_LEDS, 5);
}

void Change_MODE_random() {
  MODE = random(MODE_MIN, MODE_MAX + 1); // 1..3 inclusive
  FastLED.clear();
}
