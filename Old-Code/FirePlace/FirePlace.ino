
#include <FastLED.h>

// Uncomment to enable Serial for debugging (uses extra flash memory)
// #define ENABLE_SERIAL

#define DATA_PIN 3

//////////////////////////////////////////////////
// DEFINE LED Matrix

constexpr uint8_t MatWidth  = 17;
constexpr uint8_t MatHeight = 17;
#define NUM_LEDS (MatWidth * MatHeight)

CRGB leds[NUM_LEDS];

// XY for serpentine wiring
inline uint16_t XY(uint8_t x, uint8_t y)
{
  if (y & 0x01) {
    // Odd rows run backwards
    return (uint16_t)y * MatWidth + ((MatWidth - 1) - x);
  } else {
    // Even rows run forwards
    return (uint16_t)y * MatWidth + x;
  }
}

//////////////////////////////////////////////////
// PINS

constexpr uint8_t aVARPIN = A1;   // left pot (brightness)
constexpr uint8_t bVARPIN = A3;   // right pot (max heat)

// Button 1: Spark Burst (wired to GND when pressed)
constexpr uint8_t burstButtonPin = 6;
bool burstActive = false;
unsigned long burstStartMs = 0;
constexpr unsigned long burstDurationMs = 10000UL; // 10 seconds

// Button 2: Palette Toggle (wired to GND when pressed)
constexpr uint8_t paletteButtonPin = 7;
bool useAltPalette = false;  // false = classic HeatColor, true = Alt palette

//////////////////////////////////////////////////
// VARIABLES

// o and p define width of fire (x-range inclusive)
constexpr uint8_t o = 11;
constexpr uint8_t p = 5;

uint8_t MAXbright = 255;   // left pot -> brightness
uint8_t MAXheat   = 255;   // right pot -> heat

// Tuning
constexpr uint8_t COOLING  = 40;
constexpr uint8_t SPARKING = 120;
constexpr uint8_t EMBER    = 40;

// Pot smoothing
constexpr uint8_t numReadings = 10;

// Use 16-bit for analog samples (0..1023) and their totals (<= 10230)
uint16_t aRAY[numReadings];
uint8_t  aRAYdex = 0;
uint16_t aRAYtot = 0;
uint16_t aRAYavg = 0;

uint16_t bRAY[numReadings];
uint8_t  bRAYdex = 0;
uint16_t bRAYtot = 0;
uint16_t bRAYavg = 0;

// Array of temperature readings at each simulation cell (0..255)
static uint8_t heat[NUM_LEDS];

//////////////////////////////////////////////////
// PALETTES (PROGMEM to save RAM)

// Alternative "Ice Fire" palette with BLACK at low heat (keeps background dark)
const TProgmemRGBPalette16 IceFire_p FL_PROGMEM = {
  CRGB::Black,       CRGB::Black,       CRGB::Navy,        CRGB::Blue,
  CRGB::Blue,        CRGB::DeepSkyBlue, CRGB::DeepSkyBlue, CRGB::Aqua,
  CRGB::Aqua,        CRGB::Cyan,        CRGB::LightCyan,   CRGB::LightCyan,
  CRGB::White,       CRGB::White,       CRGB::White,       CRGB::White
};

//////////////////////////////////////////////////

void setup() {
  delay(1000);

#ifdef ENABLE_SERIAL
  Serial.begin(57600);
#endif

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 8000);

  // Clear once at startup
  FastLED.clear(true);

  // Buttons use internal pull-up; pressed == LOW
  pinMode(burstButtonPin, INPUT_PULLUP);
  pinMode(paletteButtonPin, INPUT_PULLUP);
}

void loop() {
  READ_allpots();

  // --- Spark burst trigger (edge detection, non-blocking) ---
  static bool lastBurstBtn = HIGH;            // pull-up idle (not pressed)
  const bool burstBtn = digitalRead(burstButtonPin);
  if (burstBtn == LOW && lastBurstBtn == HIGH) {  // just pressed
    burstActive = true;
    burstStartMs = millis();
  }
  lastBurstBtn = burstBtn;

  // auto-stop burst after duration
  if (burstActive && (millis() - burstStartMs >= burstDurationMs)) {
    burstActive = false;
  }

  // --- Palette toggle button (edge detection) ---
  static bool lastPalBtn = HIGH;
  const bool palBtn = digitalRead(paletteButtonPin);
  if (palBtn == LOW && lastPalBtn == HIGH) {   // just pressed
    useAltPalette = !useAltPalette;            // toggle palette flag
  }
  lastPalBtn = palBtn;

  FireARRAYCol();
  SendColours();

  delay(20); // ~50 FPS
}

void SendColours() {
  // brightness from left pot (kept as-is from your version)
  if (aRAYavg > 1017) {
    MAXbright = 255;
  } else {
    MAXbright = (uint8_t)map(aRAYavg, 0, 1023, 15, 255);
  }

  FastLED.setBrightness(MAXbright);
  FastLED.show();
  // delay(1); // safe to remove; doesn't change behavior visually
}

void READ_allpots() {
  // Left pot smoothing
  aRAYtot -= aRAY[aRAYdex];
  aRAY[aRAYdex] = analogRead(aVARPIN);
  aRAYtot += aRAY[aRAYdex];
  aRAYdex++;
  if (aRAYdex >= numReadings) aRAYdex = 0;
  aRAYavg = aRAYtot / numReadings;

  // Right pot smoothing
  bRAYtot -= bRAY[bRAYdex];
  bRAY[bRAYdex] = analogRead(bVARPIN);
  bRAYtot += bRAY[bRAYdex];
  bRAYdex++;
  if (bRAYdex >= numReadings) bRAYdex = 0;
  bRAYavg = bRAYtot / numReadings;
}

void FireARRAYCol() {
  // set max temp from right pot (unchanged mapping)
  if (bRAYavg > 1017) {
    MAXheat = 255;
  } else {
    MAXheat = (uint8_t)map(bRAYavg, 0, 1023, 100, 255);
  }

  // Step 1. Cool down every cell a little
  for (uint8_t xx = 0; xx < MatWidth; xx++) {
    for (uint8_t yy = 0; yy < MatHeight; yy++) {
      const uint8_t cool = random8(0, ((COOLING * 10) / MatHeight) + 2);
      const uint16_t idx = XY(xx, yy);
      heat[idx] = qsub8(heat[idx], cool);
    }
  }

  // Step 2. Heat from each cell drifts 'up' and diffuses a little
  for (uint8_t xx = 0; xx < MatWidth; xx++) {
    for (int8_t yy = MatHeight - 1; yy >= 2; yy--) {
      const uint16_t a = XY(xx, yy - 1);
      const uint16_t b = XY(xx, yy - 2);
      // 16-bit sum to avoid overflow, then divide
      const uint16_t sum = (uint16_t)heat[a] + (uint16_t)heat[b] + (uint16_t)heat[b];
      heat[XY(xx, yy)] = (uint8_t)(sum / 3);
    }
  }

  // Step 2.5. Heat drifts 'out' and diffuses a little (kept as in your version)
  for (uint8_t xx = 1; xx < MatWidth - 1; xx++) {
    for (int8_t yy = MatHeight - 1; yy >= 0; yy--) {
      const uint16_t c = XY(xx, yy);
      const uint16_t l = XY(xx - 1, yy);
      const uint16_t r = XY(xx + 1, yy);
      const uint16_t sum = (uint16_t)heat[c] + (uint16_t)heat[c] + (uint16_t)heat[l] + (uint16_t)heat[r];
      heat[c] = (uint8_t)(sum / 4);
    }
  }

  // ---- Step 3: ignition
  // Normal baseline sparking probability
  const uint8_t sparkingNow = SPARKING;

  // During burst: DO NOT change sparking probability;
  // only make new sparks as hot as if the heat pot were at max (255).
  const uint8_t newSparkHeat = burstActive ? 255 : MAXheat;

  // Bottom ignition zone across x = [p..o]; y = 0..1
  for (int8_t xx = o; xx >= (int8_t)p; xx--) {
    if (random8() < sparkingNow) {
      const uint8_t yy = random8(2); // 0 or 1
      heat[XY((uint8_t)xx, yy)] = newSparkHeat;
    }
  }

  // Step 4. Map heat to LED colors
  if (!useAltPalette) {
    // Classic fire (HeatColor)
    for (uint8_t yy = 0; yy < MatHeight; yy++) {
      for (uint8_t xx = 0; xx < MatWidth; xx++) {
        leds[XY(xx, yy)] = HeatColor(heat[XY(xx, yy)]);
      }
    }
  } else {
    // Alt palette with low-heat cutoff (keeps background dark)
    for (uint8_t yy = 0; yy < MatHeight; yy++) {
      for (uint8_t xx = 0; xx < MatWidth; xx++) {
        const uint8_t h = heat[XY(xx, yy)];
        if (h < 3) {
          leds[XY(xx, yy)] = CRGB::Black;  // very cold cells stay off
        } else {
          leds[XY(xx, yy)] = ColorFromPalette(IceFire_p, h, 255, LINEARBLEND);
        }
      }
    }
  }
}
