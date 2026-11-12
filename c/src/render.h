/* render.h - draws canvas, cursor, preview, palette, buttons.
 * The only file that calls raylib drawing functions. */
#ifndef PP_RENDER_H
#define PP_RENDER_H

#include "app.h"

#define WINDOW_W 900
#define WINDOW_H 700

/* Canvas: a CANVAS_PX square at (CANVAS_X, CANVAS_Y). Cells use the section
 * 7.4 a formula with canvas = CANVAS_PX (n = 8 -> 80 px, n = 16 -> 40 px). */
#define CANVAS_X     30
#define CANVAS_Y     30
#define CANVAS_PX    640

/* Cursor border thickness in pixels (the asm uses 1 display unit). */
#define CURSOR_PX    3

/* Draws one full frame. Call between BeginDrawing() and EndDrawing(). */
void render_frame(const AppState *app);

#endif /* PP_RENDER_H */
