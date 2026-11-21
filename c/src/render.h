/* render.h - draws canvas, cursor, preview, palette, buttons.
 * The only file that calls raylib drawing functions. */
#ifndef PP_RENDER_H
#define PP_RENDER_H

#include "app.h"

#define WINDOW_W 900
#define WINDOW_H 760                 /* 700 + the button row (buttons.h) */

/* Canvas: a CANVAS_PX square at (CANVAS_X, CANVAS_Y). Cells use the section
 * 7.4 a formula with canvas = CANVAS_PX (n = 8 -> 80 px, n = 16 -> 40 px). */
#define CANVAS_X     30
#define CANVAS_Y     30
#define CANVAS_PX    640

/* Side panel, right of the canvas. */
#define PANEL_X      710
#define SWATCH_Y     60
#define SWATCH_PX    140

/* Saved-color swatches under the preview: two columns (slot k in column
 * k & 1, row k >> 1), like the asm layout. Also the hit area for clicks. */
#define PALETTE_TITLE_Y  360
#define PALETTE_X        PANEL_X
#define PALETTE_Y        390
#define PALETTE_SWATCH   50
#define PALETTE_PITCH_X  80
#define PALETTE_PITCH_Y  60

/* Status line: under the canvas, above the button row. */
#define STATUS_Y     (CANVAS_Y + CANVAS_PX + 10)

/* Cursor border thickness in pixels (the asm uses 1 display unit). */
#define CURSOR_PX    3

/* Draws one full frame. Call between BeginDrawing() and EndDrawing(). */
void render_frame(const AppState *app);

#endif /* PP_RENDER_H */
