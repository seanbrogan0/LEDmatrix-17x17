#pragma once
#include <stdint.h>

// ===== Input Interface =====
//
// Centralised access to physical controls.
// All effects must read input through this API.
//
// No effect should call analogRead() directly.

// Call once from setup()
void initInputs();

// Call once per loop()
void updateInputs();

// Brightness (0..255)
uint8_t getBrightness();

// Frame delay in milliseconds (smaller = faster)
uint16_t getFrameDelayMs();