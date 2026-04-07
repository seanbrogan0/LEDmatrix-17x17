#pragma once
#include <stdint.h>

// ===== Matrix configuration =====

// Matrix dimensions
static const uint8_t MATRIX_WIDTH  = 17;
static const uint8_t MATRIX_HEIGHT = 17;

// Total LED count
static const uint16_t NUM_LEDS = MATRIX_WIDTH * MATRIX_HEIGHT;

// Hardware
static const uint8_t DATA_PIN = 3;

// Defaults
static const uint8_t DEFAULT_BRIGHTNESS = 80;