#pragma once

#include <stdint.h>

// ===== Matrix Coordinate Mapping =====
//
// Provides a single function for converting 2D matrix coordinates
// into a linear LED index, based on the physical wiring layout.
//
// All animations must use XY(x, y) — never raw LED indices.

uint16_t XY(uint8_t x, uint8_t y);