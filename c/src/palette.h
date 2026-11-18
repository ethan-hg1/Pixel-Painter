/* palette.h - 8-slot saved colors, FIFO replace (BUILD_INSTRUCTIONS.txt 2.6).
 * Pure functions on a plain array so they can be unit-tested without
 * raylib. app.c wraps them as palette_add / palette_pick (which also touch
 * the app's current color). */
#ifndef PP_PALETTE_H
#define PP_PALETTE_H

#include <stdint.h>

#include "config.h"

/* Every slot EMPTY, next = 0. */
void palette_init(uint32_t slots[PALETTE_SLOTS], int *next);

/* Index of color in slots, or -1 if it is not saved. */
int palette_find(const uint32_t slots[PALETTE_SLOTS], uint32_t color);

/* Saves color at *next and advances *next (mod 8, the FIFO ring). Returns 1
 * if saved. A color already in the palette is not saved again (returns 0),
 * and neither is EMPTY. */
int palette_insert(uint32_t slots[PALETTE_SLOTS], int *next, uint32_t color);

/* Maps a point to a swatch slot. The swatches form two columns (slot k is
 * in column k & 1, row k >> 1). The top-left swatch is at (x0, y0), each
 * swatch is size x size, and columns and rows are pitch_x and pitch_y apart.
 * Returns 1 and sets *slot if (px, py) is on a swatch, 0 otherwise. */
int palette_slot_at(int px, int py, int x0, int y0, int size,
                    int pitch_x, int pitch_y, int *slot);

#endif /* PP_PALETTE_H */
