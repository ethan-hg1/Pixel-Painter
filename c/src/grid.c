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

/* One axis of grid_cell_at: maps pixel p to an index 0..n-1, or -1. */
static int axis_index(int p, int origin, int canvas, int n)
{
    int cell = grid_cell_size(canvas, n);
    int rel = p - origin - grid_offset(canvas, n);

    if (rel < 0 || rel >= n * cell) {
        return -1;
    }
    return rel / cell;
}

int grid_cell_at(int px, int py, int origin_x, int origin_y, int canvas,
                 int n, int *row, int *col)
{
    int r = axis_index(py, origin_y, canvas, n);
    int c = axis_index(px, origin_x, canvas, n);

    if (r < 0 || c < 0) {
        return 0;
    }
    *row = r;
    *col = c;
    return 1;
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
