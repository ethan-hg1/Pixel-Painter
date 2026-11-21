/* app.h - AppState + one function per action.
 * Keyboard, mouse, buttons and replay all call these. No raylib.
 * Fields are added step by step; see section 7.3 for the full list. */
#ifndef PP_APP_H
#define PP_APP_H

#include <stdint.h>

#include "config.h"
#include "grid.h"
#include "history.h"
#include "palette.h"

#define STATUS_LEN 512             /* long enough for a saved path (2.10) */

typedef struct {
    uint32_t grid[GRID_CELLS];   /* stride GRID_STRIDE, EMPTY = no color */
    int      n;                  /* current grid size */
    int      cur_row, cur_col;
    uint32_t color;              /* current color 0x00RRGGBB */
    uint32_t palette[PALETTE_SLOTS];  /* saved colors, EMPTY = unused */
    int      palette_next;       /* slot the next save overwrites (FIFO) */
    History  hist;               /* undo/redo, section 10 */
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

/* Sets the cell under the cursor to EMPTY (key e, right click).
 * Returns 1 if the cell changed, 0 if it was already EMPTY. */
int  do_erase(AppState *app);

/* Sets every cell to EMPTY (ctrl+delete). Returns 1 if any cell changed,
 * 0 if the canvas was already empty. */
int  do_clear(AppState *app);

/* Changes n by delta (+RESIZE_STEP grow, -RESIZE_STEP shrink). Returns 1 if
 * n changed. Returns 0 and does nothing if the new size would leave
 * [MIN_N, MAX_N]. Shrinking sets cells outside the new size to EMPTY, and
 * the cursor is clamped into the new grid. */
int  do_resize(AppState *app, int delta);

/* Undo (ctrl+z) and redo (ctrl+shift+z) the last recorded action. Return 1
 * if something was undone or redone, 0 if there was nothing to do. They
 * change the grid and n but never push a new record. */
int  do_undo(AppState *app);
int  do_redo(AppState *app);

/* Saves the current color into the palette (key p). Returns 1 if it was
 * added, 0 if it is already saved. Palette changes are not undoable. */
int  palette_add(AppState *app);

/* Makes palette slot (0..7) the current color (keys 1..8, click). Returns 1
 * if the color changed, 0 if the slot is empty or already the current color. */
int  palette_pick(AppState *app, int slot);

/* Writes the picture (n x n cells, EXPORT_SCALE px each) to the PNG file at
 * path (ctrl+s, section 2.10). Sets status "Saved: <path>" on success, or
 * "Save failed" (returns 0). The caller picks the path (Downloads or cwd). */
int  do_save(AppState *app, const char *path);

/* Moves the cursor by (drow, dcol), clamped to 0..n-1 (no wrap).
 * Returns 1 if the cursor moved, 0 if it was already at that edge. */
int  move_cursor(AppState *app, int drow, int dcol);

/* Sets the current color (0x00RRGGBB). */
void set_color(AppState *app, uint32_t color);

/* Color entry mode (key t), section 2.4. While entry_mode is 1 the caller
 * must route keys here and ignore every other shortcut. */
void entry_start(AppState *app);
/* Appends c if it is printable ASCII (0x20..0x7E) and the buffer has room
 * (ENTRY_MAX_LEN chars); returns 1. Anything else is ignored; returns 0. */
int  entry_add_char(AppState *app, int c);
void entry_backspace(AppState *app);
void entry_cancel(AppState *app);
/* Leaves entry mode. Valid hex -> set_color; invalid -> status
 * "Invalid color" and the color is unchanged. Returns 1 if valid. */
int  entry_confirm(AppState *app);

#endif /* PP_APP_H */
