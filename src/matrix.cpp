#include "matrix.h"
#include "config.h"

// ===== Serpentine XY Mapper =====
//
// Physical layout assumptions:
//
// - Matrix dimensions are defined in config.h
// - Rows are wired in a serpentine pattern:
//     * Even rows: left → right
//     * Odd rows:  right → left
//
// Coordinate system:
// - (0,0) is the top-left corner
// - X increases to the right
// - Y increases downward

uint16_t XY(uint8_t x, uint8_t y) {

  // ----- Safety bounds -----
  // Prevent out-of-range access if an effect misbehaves
  if (x >= MATRIX_WIDTH || y >= MATRIX_HEIGHT) {
    return 0;
  }

  // ----- Serpentine wiring -----
  if (y & 0x01) {
    // Odd row: right-to-left
    return (y * MATRIX_WIDTH) + (MATRIX_WIDTH - 1 - x);
  } else {
    // Even row: left-to-right
    return (y * MATRIX_WIDTH) + x;
  }
}