/* render.c - see render.h. */
#include "render.h"

#include <stdio.h>

#include "raylib.h"

#define TEXT_COLOR   0xE0E0E0u
#define DIM_COLOR    0x9A9A9Au
#define ERROR_COLOR  0xFF6060u
#define FONT_PX      20

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

/* Preview swatch with a light frame (so black and the background color
 * stay visible), the hex text, and the entry box while typing. */
static void draw_preview(const AppState *app)
{
    char hex[16];
    int y = SWATCH_Y + SWATCH_PX + 16;

    DrawText("Color", PANEL_X, SWATCH_Y - 28, FONT_PX, rgb(TEXT_COLOR));
    DrawRectangle(PANEL_X - 2, SWATCH_Y - 2, SWATCH_PX + 4, SWATCH_PX + 4,
                  rgb(TEXT_COLOR));
    DrawRectangle(PANEL_X, SWATCH_Y, SWATCH_PX, SWATCH_PX, rgb(app->color));
    snprintf(hex, sizeof hex, "#%06X", (unsigned)(app->color & 0xFFFFFFu));
    DrawText(hex, PANEL_X, y, FONT_PX, rgb(TEXT_COLOR));
    y += 40;

    if (app->entry_mode) {
        char typed[ENTRY_MAX_LEN + 2];
        snprintf(typed, sizeof typed, "%s_", app->entry_buf);
        DrawText("Type a hex color:", PANEL_X, y, FONT_PX, rgb(TEXT_COLOR));
        y += 28;
        DrawRectangleLines(PANEL_X - 2, y - 4, SWATCH_PX + 4, FONT_PX + 8,
                           rgb(TEXT_COLOR));
        DrawText(typed, PANEL_X + 4, y, FONT_PX, rgb(TEXT_COLOR));
        y += 36;
        DrawText("Enter: OK  Esc: cancel", PANEL_X, y, 10, rgb(DIM_COLOR));
    } else if (app->status[0] != '\0') {
        DrawText(app->status, PANEL_X, y, FONT_PX, rgb(ERROR_COLOR));
    }
}

static void draw_status(const AppState *app)
{
    char line[96];
    snprintf(line, sizeof line, "%dx%d   cursor (%d, %d)   w: paint   t: color",
             app->n, app->n, app->cur_row, app->cur_col);
    DrawText(line, CANVAS_X, WINDOW_H - FONT_PX - 10, FONT_PX, rgb(TEXT_COLOR));
}

void render_frame(const AppState *app)
{
    ClearBackground(rgb(BG_COLOR));
    draw_canvas(app);
    draw_cursor(app);
    draw_preview(app);
    draw_status(app);
}
