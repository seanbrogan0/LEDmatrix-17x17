# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

PlatformIO-based firmware for a 17×17 serpentine-wired WS2812/NeoPixel LED matrix (289 LEDs) driven by an Arduino Nano (ATmega328P), using FastLED. The project is deliberately PlatformIO-native — it is not maintained with the Arduino IDE. It is a work in progress: core architecture is in place, additional effects are being ported.

## Commands

All builds target the single `nanoatmega328` environment defined in `platformio.ini`.

```bash
pio run                    # Build firmware
pio run -t upload          # Build and flash to the Nano over USB
pio run -t clean           # Clean build artifacts
pio device monitor         # Serial monitor
pio check                  # Static analysis (no separate linter configured)
```

There are no unit tests yet (`test/` contains only the PlatformIO placeholder README). If tests are added, they run with `pio test`.

Note: the ATmega328P has 2 KB of RAM and the LED buffer alone uses 867 bytes (289 × 3). Keep effect state small and prefer `static`/`const` (or PROGMEM) data; watch the RAM usage report printed at the end of `pio run`.

## Architecture

The firmware is a set of small modules with single ownership of resources, replacing the older monolithic sketches:

- `src/main.cpp` — entry point and orchestration. **Sole definition** of the `CRGB leds[NUM_LEDS]` buffer and the shared `uint8_t scratch[NUM_LEDS]` workspace; the loop is `updateInputs() → run<Effect>() → FastLED.show()`.
- `include/globals.h` — `extern` declarations of `leds` and `scratch` for effects to use.
- `include/config.h` — compile-time configuration: matrix dimensions, `NUM_LEDS`, `DATA_PIN` (D3), default brightness.
- `src/matrix.cpp` / `include/matrix.h` — `XY(x, y)` maps logical coordinates to physical LED index, encapsulating the serpentine wiring (even rows left→right, odd rows right→left; origin top-left, Y increases downward). Out-of-bounds coordinates clamp to index 0.
- `src/input.cpp` / `include/input.h` — potentiometer handling (brightness on A1, speed on A3), smoothed with an exponential moving average held in an 8×-scaled accumulator. Exposes `getBrightness()` and `getFrameDelayMs()`.
- `src/effects/` — one `.cpp` per animation, each exposing a single `run<Name>()` entry point (e.g. `runBouncingBall()`), declared in `include/effects.h`. Effects keep their own `static` state and do their own frame timing with `millis()` (non-blocking — never `delay()`).

### Conventions for effects

- Draw only through `XY(x, y)` — never compute raw LED indices.
- Read controls only through the input API — never call `analogRead()` directly in an effect.
- Access the LED buffer via `#include "globals.h"`.
- Per-cell working state (heat maps, cell grids, trail ages) goes in the shared `scratch[NUM_LEDS]` buffer from `globals.h`, not in new per-effect static arrays — only one effect runs at a time, and a second `CRGB`-sized buffer does not fit in RAM. Initialise the region you use; contents are undefined on entry.
- Constant data (palettes, sprites, frames) goes in `PROGMEM`, read back with `pgm_read_*`/`memcpy_P` — plain `const` arrays occupy RAM on AVR.
- Prefer FastLED's 8/16-bit fixed-point math (`sin8`, `beatsin8`, `scale8`, `lerp8by8`) over `float`, which is software-emulated and pulls the soft-float library into flash. The `Old-Code/` sketches use floats freely; convert when porting.
- Declare each new `run<Name>()` in `include/effects.h`.
- Effect selection is currently compile-time: `main.cpp` calls one effect's `run` function per loop. Runtime/build-time selection is planned but not implemented.

### Known inconsistencies

- `README.md` describes the intended architecture and project status; keep it in sync when adding effects.

## Other directories

- `Old-Code/` — legacy standalone Arduino sketches (Christmas tree, candle, fireplace, metaballs, etc.). These are the source material for porting effects into the modular architecture; do not build on them directly.
- `3d_Models/`, `Circuit/` — CAD (frame/diffuser) and Fritzing wiring assets; the firmware's wiring assumptions (serpentine layout, pin choices) must stay consistent with these.

## License

CC BY-NC 4.0 — non-commercial use only.
