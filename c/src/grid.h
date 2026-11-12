/* grid.h - grid array, get/set cell, clear, layout and cursor color.
 * Maps onto asm/grid.asm (get/set/clear/resize) and the formulas in
 * BUILD_INSTRUCTIONS.txt section 7. No raylib. */
#ifndef PP_GRID_H
#define PP_GRID_H

#include <stdint.h>

#include "config.h"

#define GRID_CELLS (MAX_N * MAX_N)

/* Word index of cell (row, col): row * GRID_STRIDE + col.
 * The byte offset used by the asm is grid_index(row, col) << 2. */
static inline int grid_index(int row, int col)
{
    return row * GRID_STRIDE + col;
}

uint32_t grid_get(const uint32_t *grid, int row, int col);
void     grid_set(uint32_t *grid, int row, int col, uint32_t color);

/* Sets all GRID_CELLS words to EMPTY (not just the current n x n). */
void     grid_clear(uint32_t *grid);

/* Section 7.4 a: cell = canvas / n (floor), offset = (canvas - n*cell) / 2.
 * canvas is CANVAS_UNITS in asm and the canvas size in pixels in C. */
int      grid_cell_size(int canvas, int n);
int      grid_offset(int canvas, int n);

/* Section 7.4 b: Y = (77*R + 150*G + 29*B) >> 8, a Q8 fixed-point
 * weighted sum. Returns 0..255. */
int      luminance_q8(uint32_t color);

/* Cursor color for a cell: black (0x000000) on light cells (Y >= 128 or
 * EMPTY), white (0xFFFFFF) on dark ones. */
uint32_t cursor_color(uint32_t cell);

#endif /* PP_GRID_H */
