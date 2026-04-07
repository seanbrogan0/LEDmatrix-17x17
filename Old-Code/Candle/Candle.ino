
#include <FastLED.h>

#define DATA_PIN 3

//////////////////////////////////////////////////
// DEFINE LED Matrix

// Params for width and height
const uint8_t MatWidth  = 17;
const uint8_t MatHeight = 17;
#define NUM_LEDS (MatWidth * MatHeight)

CRGB leds[NUM_LEDS];

// XY as LED number from x,y co-ordinates
// for serpentine wiring pattern
uint8_t x;
uint8_t y;

inline uint16_t XY(uint8_t x, uint8_t y) {
  // Bounds check kept minimal for speed; assumes caller uses valid coords
  // (Original behavior: no guard)
  if (y & 0x01) {
    // Odd rows run backwards
    const uint8_t reverseX = (MatWidth - 1) - x;
    return (y * MatWidth) + reverseX;
  } else {
    // Even rows run forwards
    return (y * MatWidth) + x;
  }
}

//////////////////////////////////////////////////
// DEFINE PINS (OTHER THAN LED STRIP)

const uint8_t aVARPIN = A1;   // left pot
const uint8_t bVARPIN = A3;   // right pot
const uint8_t BTN_CANDLE_PIN = 6;   // wired to GND when pressed (active LOW)

//////////////////////////////////////////////////
// VARIABLES

// o and p define width of fire (kept from your original constants)
#define o 11
#define p 5

byte MAXbright = 255;   // brightness - left pot
byte MAXheat   = 255;   // heat - right pot

// Variables and start level that will be adjusted by potentiometer
// arrays and values for smoothing
const uint8_t numReadings = 10;

uint16_t aRAYtot = 0; // 10 * 1023 = 10230 (fits uint16_t)
uint16_t bRAYtot = 0;

uint16_t aRAY[ numReadings ]; // use uint16_t for analog samples (0..1023)
uint16_t bRAY[ numReadings ];

uint8_t aRAYdex = 0;
uint8_t bRAYdex = 0;

uint16_t bRAYavg = 0; // averaged analog value (0..1023)

// ------ Candle color cycling state ------
const CRGB candlePalette[] = {
  CRGB::SaddleBrown, CRGB::BurlyWood, CRGB::DarkOrange,
  CRGB::Gold, CRGB::Chocolate, CRGB::White, CRGB::Black
};
const uint8_t CANDLE_COLORS = sizeof(candlePalette) / sizeof(candlePalette[0]);
uint8_t candleColorIndex = 0;
CRGB candleColor = candlePalette[candleColorIndex];

// Simple single-press debounce
const uint16_t DEBOUNCE_MS = 25;
uint32_t lastCandlePressMs = 0;
uint8_t  prevCandleLevel   = HIGH;   // INPUT_PULLUP: HIGH = released, LOW = pressed

void setup() {
  // start delay
  delay(1000);
  // Serial disabled to save RAM/flash (was unused in your runtime)

  // setup leds
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 8000);
  FastLED.clear(true);  // valid signature, same visual effect as your clear

  // Button uses internal pull-up (active LOW)
  pinMode(BTN_CANDLE_PIN, INPUT_PULLUP);
  prevCandleLevel = digitalRead(BTN_CANDLE_PIN);

  // Initialize smoothing arrays with current readings to avoid startup spikes
  uint16_t aInit = analogRead(aVARPIN);
  uint16_t bInit = analogRead(bVARPIN);

  aRAYtot = 0;
  bRAYtot = 0;
  for (uint8_t i = 0; i < numReadings; i++) {
    aRAY[i] = aInit; aRAYtot += aInit;
    bRAY[i] = bInit; bRAYtot += bInit;
  }
  bRAYavg = bInit;
}

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

void loop() {
  READ_allpots();
  handleCandleButton();

  DRAWFRAME_candle();
  // ANIMATE_flame(); // (commented in your original)

  SendColours();
}

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

void SendColours() {
  // sub to set brightness and show leds

  // Use smoothed A1 directly from aRAYtot to avoid extra function RAM
  // (Exact behavior of your original mapping preserved)
  const uint16_t aPOTavgVal = aRAYtot / numReadings;

  if (aPOTavgVal > 1017) {
    MAXbright = 255;
  } else {
    MAXbright = map(aPOTavgVal, 0, 1023, 15, 255);
  }
  FastLED.setBrightness(MAXbright);
  FastLED.show();
  delay(10);
}

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

void READ_allpots() {
  // A1 smoothing (brightness)
  aRAYtot -= aRAY[aRAYdex];
  aRAY[aRAYdex] = analogRead(aVARPIN);
  aRAYtot += aRAY[aRAYdex];
  aRAYdex++;
  if (aRAYdex >= numReadings) aRAYdex = 0;

  // A3 smoothing (heat/intensity)
  bRAYtot -= bRAY[bRAYdex];
  bRAY[bRAYdex] = analogRead(bVARPIN);
  bRAYtot += bRAY[bRAYdex];
  bRAYdex++;
  if (bRAYdex >= numReadings) bRAYdex = 0;
  bRAYavg = bRAYtot / numReadings;
}

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

// button handler to cycle candle color (single press only)
inline void handleCandleButton() {
  const uint32_t now = millis();
  const uint8_t cur = digitalRead(BTN_CANDLE_PIN); // LOW when pressed

  if (cur != prevCandleLevel) {
    prevCandleLevel = cur;
    if (cur == LOW && (now - lastCandlePressMs) > DEBOUNCE_MS) {
      lastCandlePressMs = now;
      // advance candle color
      candleColorIndex = (candleColorIndex + 1) % CANDLE_COLORS;
      candleColor = candlePalette[candleColorIndex];
    }
  }
}

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

void DRAWFRAME_candle() {
  // COOLING: Less cooling = taller flames.
  // SPARKING: chance (out of 255) that a new spark will be lit.
  const uint8_t COOLING  = 100; // keep your original values
  const uint8_t SPARKING = 120;

  // Temperature grid (0..255) — changed from int -> uint8_t to save RAM
  static uint8_t heat[NUM_LEDS];

  // map A3 to max spark temp
  if (bRAYavg > 1017) {
    MAXheat = 255;
  } else {
    MAXheat = map(bRAYavg, 0, 1023, 100, 255);
  }

  // Step 1. Cool down every cell a little
  for (uint8_t xx = 0; xx < MatWidth; xx++) {
    for (uint8_t yy = 0; yy < MatHeight; yy++) {
      const uint16_t idx = XY(xx, yy);
      heat[idx] = qsub8(heat[idx], random8(0, ((COOLING * 10) / MatHeight) + 2));
    }
  }

  // Step 2. Heat drifts 'up' and diffuses a little (same direction/order as your original)
  for (uint8_t xx = 0; xx < MatWidth; xx++) {
    for (int8_t k = MatHeight - 1; k >= 2; k--) {
      // Weighted average of cells below
      heat[XY(xx, k)] = (uint8_t)((heat[XY(xx, k - 1)] + heat[XY(xx, k - 2)] + heat[XY(xx, k - 2)]) / 3);
    }
  }

  // Step 2.5. Heat drifts 'Out' and diffuses a little
  for (uint8_t xx = 1; xx < MatWidth - 1; xx++) {
    for (int8_t k = MatHeight - 1; k >= 0; k--) {
      const uint16_t idx = XY(xx, k);
      const uint8_t self  = heat[idx];
      const uint8_t left  = heat[XY(xx - 1, k)];
      const uint8_t right = heat[XY(xx + 1, k)];
      heat[idx] = (uint8_t)((self + self + left + right) / 4);
    }
  }

  // Step 3. Randomly ignite new 'sparks' of heat near the bottom
  for (uint8_t xx = 7; xx < 10; xx++) {
    if (random8() < SPARKING) {
      const uint8_t yy = random8(9, 11); // same vertical spark band
      heat[XY(xx, yy)] = MAXheat;
    }
  }

  // Map heat to colors
  for (uint8_t jj = 0; jj < MatHeight; jj++) {
    for (uint8_t xx = 0; xx < MatWidth; xx++) {
      leds[XY(xx, jj)] = HeatColor(heat[XY(xx, jj)]);
    }
  }

  // Set candle dimensions (same placement as your original)
  for (uint8_t yy = 0; yy < 9; yy++) {
    for (uint8_t xx = 6; xx < 11; xx++) {
      leds[XY(xx, yy)] = candleColor;
    }
  }
}
