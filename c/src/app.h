/* app.h - AppState + one function per action (BUILD_INSTRUCTIONS.txt 7.3).
 * Keyboard, mouse, buttons and replay all call these. No raylib.
 * Fields are added step by step; see section 7.3 for the full list. */
#ifndef PP_APP_H
#define PP_APP_H

#include <stdint.h>

#include "config.h"
#include "grid.h"

#define STATUS_LEN 64

typedef struct {
    uint32_t grid[GRID_CELLS];   /* stride GRID_STRIDE, EMPTY = no color */
    int      n;                  /* current grid size */
    int      cur_row, cur_col;
    uint32_t color;              /* current color 0x00RRGGBB */
    int      entry_mode;         /* 1 while typing a hex code */
    char     entry_buf[ENTRY_MAX_LEN + 1];  /* typed chars + '\0' */
    int      entry_len;
    char     status[STATUS_LEN]; /* message line, "" when none (C only) */
} AppState;

/* Empty grid, n = START_N, cursor (0, 0), color black, not in entry mode. */
void app_init(AppState *app);

/* Paints the cursor cell with the current color.
 * Returns 1 if the cell changed, 0 if it already had that color. */
int  do_paint(AppState *app);

/* Sets the current color (0x00RRGGBB). */
void set_color(AppState *app, uint32_t color);

/* Color entry mode (key t), section 2.4. While entry_mode is 1 the caller
 * must route keys here and ignore every other shortcut. */
void entry_start(AppState *app);
/* Appends c if it is printable ASCII (0x20..0x7E) and the buffer has room
 * (ENTRY_MAX_LEN chars). Anything else is ignored. */
void entry_add_char(AppState *app, int c);
void entry_backspace(AppState *app);
void entry_cancel(AppState *app);
/* Leaves entry mode. Valid hex -> set_color; invalid -> status
 * "Invalid color" and the color is unchanged. Returns 1 if valid. */
int  entry_confirm(AppState *app);

#endif /* PP_APP_H */
