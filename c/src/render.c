/* render.c - see render.h. */
#include "render.h"

#include <stdio.h>

#include "raylib.h"

#define BACKGROUND   0x303030u   /* same as the asm background (7.5) */
#define EMPTY_LIGHT  0xDCDCDCu   /* EMPTY cells: light checker by cell */
#define EMPTY_DARK   0xC4C4C4u
#define TEXT_COLOR   0xE0E0E0u

static Color rgb(uint32_t c)
{
    return (Color){ (unsigned char)(c >> 16), (unsigned char)(c >> 8),
                    (unsigned char)c, 0xFF };
}

static Rectangle cell_rect(int n, int row, int col)
{
    int cell = grid_cell_size(CANVAS_PX, n);
    int off = grid_offset(CANVAS_PX, n);
    return (Rectangle){ (float)(CANVAS_X + off + col * cell),
                        (float)(CANVAS_Y + off + row * cell),
                        (float)cell, (float)cell };
}

static void draw_canvas(const AppState *app)
{
    for (int row = 0; row < app->n; row++) {
        for (int col = 0; col < app->n; col++) {
            uint32_t c = grid_get(app->grid, row, col);
            if (c == EMPTY) {
                c = ((row + col) & 1) ? EMPTY_DARK : EMPTY_LIGHT;
            }
            DrawRectangleRec(cell_rect(app->n, row, col), rgb(c));
        }
    }
}

static void draw_cursor(const AppState *app)
{
    uint32_t cell = grid_get(app->grid, app->cur_row, app->cur_col);
    DrawRectangleLinesEx(cell_rect(app->n, app->cur_row, app->cur_col),
                         CURSOR_PX, rgb(cursor_color(cell)));
}

static void draw_status(const AppState *app)
{
    char line[96];
    snprintf(line, sizeof line, "%dx%d   cursor (%d, %d)   color #%06X   w: paint",
             app->n, app->n, app->cur_row, app->cur_col,
             (unsigned)(app->color & 0xFFFFFFu));
    DrawText(line, CANVAS_X, WINDOW_H - 20 - 10, 20, rgb(TEXT_COLOR));
}

void render_frame(const AppState *app)
{
    ClearBackground(rgb(BACKGROUND));
    draw_canvas(app);
    draw_cursor(app);
    draw_status(app);
}
