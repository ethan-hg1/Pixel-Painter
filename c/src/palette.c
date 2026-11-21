/* palette.c - 8-slot saved colors, FIFO replace (2.6).
 * No raylib in this file. */
#include "palette.h"

void palette_init(uint32_t slots[PALETTE_SLOTS], int *next)
{
    for (int i = 0; i < PALETTE_SLOTS; i++) {
        slots[i] = EMPTY;
    }
    *next = 0;
}

int palette_find(const uint32_t slots[PALETTE_SLOTS], uint32_t color)
{
    for (int i = 0; i < PALETTE_SLOTS; i++) {
        if (slots[i] == color) {
            return i;
        }
    }
    return -1;
}

int palette_insert(uint32_t slots[PALETTE_SLOTS], int *next, uint32_t color)
{
    if (color == EMPTY || palette_find(slots, color) >= 0) {
        return 0;
    }
    slots[*next] = color;
    *next = (*next + 1) & (PALETTE_SLOTS - 1);
    return 1;
}

int palette_slot_at(int px, int py, int x0, int y0, int size,
                    int pitch_x, int pitch_y, int *slot)
{
    int dx = px - x0;
    int dy = py - y0;
    int col, row;

    if (dx < 0 || dy < 0) {
        return 0;
    }
    col = dx / pitch_x;
    row = dy / pitch_y;
    if (dx % pitch_x >= size || dy % pitch_y >= size) {
        return 0;
    }
    if (col > 1 || row * 2 + col >= PALETTE_SLOTS) {
        return 0;
    }
    *slot = row * 2 + col;
    return 1;
}
