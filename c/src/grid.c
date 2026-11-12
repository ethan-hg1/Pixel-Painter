/* grid.c - see grid.h. No raylib in this file (BUILD_INSTRUCTIONS.txt 4.2). */
#include "grid.h"

uint32_t grid_get(const uint32_t *grid, int row, int col)
{
    return grid[grid_index(row, col)];
}

void grid_set(uint32_t *grid, int row, int col, uint32_t color)
{
    grid[grid_index(row, col)] = color;
}

void grid_clear(uint32_t *grid)
{
    for (int i = 0; i < GRID_CELLS; i++) {
        grid[i] = EMPTY;
    }
}

int grid_cell_size(int canvas, int n)
{
    return canvas / n;
}

int grid_offset(int canvas, int n)
{
    return (canvas - n * grid_cell_size(canvas, n)) / 2;
}

int luminance_q8(uint32_t color)
{
    uint32_t r = (color >> 16) & 0xFF;
    uint32_t g = (color >> 8) & 0xFF;
    uint32_t b = color & 0xFF;
    return (int)((77 * r + 150 * g + 29 * b) >> 8);
}

uint32_t cursor_color(uint32_t cell)
{
    if (cell == EMPTY || luminance_q8(cell) >= 128) {
        return 0x00000000u;
    }
    return 0x00FFFFFFu;
}
