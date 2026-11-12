/* app.h - AppState + one function per action (BUILD_INSTRUCTIONS.txt 7.3).
 * Keyboard, mouse, buttons and replay all call these. No raylib.
 * Fields are added step by step; see section 7.3 for the full list. */
#ifndef PP_APP_H
#define PP_APP_H

#include <stdint.h>

#include "config.h"
#include "grid.h"

typedef struct {
    uint32_t grid[GRID_CELLS];   /* stride GRID_STRIDE, EMPTY = no color */
    int      n;                  /* current grid size */
    int      cur_row, cur_col;
    uint32_t color;              /* current color 0x00RRGGBB */
} AppState;

/* Empty grid, n = START_N, cursor (0, 0), color black. */
void app_init(AppState *app);

/* Paints the cursor cell with the current color.
 * Returns 1 if the cell changed, 0 if it already had that color. */
int  do_paint(AppState *app);

#endif /* PP_APP_H */
