/* render.c - see render.h. */
#include "render.h"

#include <stdio.h>
#include <string.h>

#include "buttons.h"
#include "raylib.h"

#define TEXT_COLOR   0xE0E0E0u
#define DIM_COLOR    0x9A9A9Au
#define ERROR_COLOR  0xFF6060u
#define FONT_PX      20
#define PANEL_W      (WINDOW_W - PANEL_X - 14)
#define EMPTY_OUTLINE 0x505050u
#define BUTTON_FILL  0x3C3C3Cu
#define BUTTON_EDGE  0x8A8A8Au

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

/* Draws text in lines no wider than width px. A line breaks before the
 * character that would make it too wide (a path has no spaces to break at). */
static void draw_wrapped(const char *text, int x, int y, int width, int font,
                         int line_h, Color color)
{
    char line[STATUS_LEN];
    size_t len = 0;

    line[0] = '\0';
    for (const char *p = text; *p != '\0' && len + 2 < sizeof line; p++) {
        line[len] = *p;
        line[len + 1] = '\0';
        if (MeasureText(line, font) > width && len > 0) {
            line[len] = '\0';
            DrawText(line, x, y, font, color);
            y += line_h;
            line[0] = *p;
            line[1] = '\0';
            len = 1;
        } else {
            len++;
        }
    }
    DrawText(line, x, y, font, color);
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
    } else if (strncmp(app->status, "Saved: ", 7) == 0) {
        /* the path is long: "Saved:" then the path wrapped in the panel */
        DrawText("Saved:", PANEL_X, y, FONT_PX, rgb(TEXT_COLOR));
        draw_wrapped(app->status + 7, PANEL_X, y + 26, PANEL_W, 10, 13,
                     rgb(TEXT_COLOR));
    } else if (app->status[0] != '\0') {
        DrawText(app->status, PANEL_X, y, FONT_PX, rgb(ERROR_COLOR));
    }
}

/* Eight swatches (empty slots as a dark gray outline), a frame around the
 * current color if it is saved, and the keys under them. */
static void draw_palette(const AppState *app)
{
    DrawText("Saved colors", PANEL_X, PALETTE_TITLE_Y, FONT_PX, rgb(TEXT_COLOR));
    for (int k = 0; k < PALETTE_SLOTS; k++) {
        int x = PALETTE_X + (k & 1) * PALETTE_PITCH_X;
        int y = PALETTE_Y + (k >> 1) * PALETTE_PITCH_Y;
        Rectangle r = { (float)x, (float)y, PALETTE_SWATCH, PALETTE_SWATCH };
        uint32_t c = app->palette[k];

        if (c == EMPTY) {
            DrawRectangleLinesEx(r, 2, rgb(EMPTY_OUTLINE));
            continue;
        }
        DrawRectangleRec(r, rgb(c));
        if (c == app->color) {
            DrawRectangleLinesEx(r, 3, rgb(TEXT_COLOR));
        }
    }
    DrawText("p: save   1-8: pick", PANEL_X, PALETTE_Y + 4 * PALETTE_PITCH_Y - 4,
             10, rgb(DIM_COLOR));
}

static void draw_status(const AppState *app)
{
    char line[96];
    snprintf(line, sizeof line, "%dx%d   cursor (%d, %d)   arrows: move   w: paint   e: erase   t: color",
             app->n, app->n, app->cur_row, app->cur_col);
    DrawText(line, CANVAS_X, STATUS_Y, FONT_PX, rgb(TEXT_COLOR));
}

/* The button row (2.11): each button shows its label centered. */
static void draw_buttons(void)
{
    for (int i = 0; i < BUTTON_COUNT; i++) {
        int x = BUTTON_X + i * (BUTTON_W + BUTTON_GAP);
        const char *label = button_label(i);
        int tw = MeasureText(label, FONT_PX);

        DrawRectangle(x, BUTTON_Y, BUTTON_W, BUTTON_H, rgb(BUTTON_FILL));
        DrawRectangleLines(x, BUTTON_Y, BUTTON_W, BUTTON_H, rgb(BUTTON_EDGE));
        DrawText(label, x + (BUTTON_W - tw) / 2,
                 BUTTON_Y + (BUTTON_H - FONT_PX) / 2, FONT_PX,
                 rgb(TEXT_COLOR));
    }
}

void render_frame(const AppState *app)
{
    ClearBackground(rgb(BG_COLOR));
    draw_canvas(app);
    draw_cursor(app);
    draw_preview(app);
    draw_palette(app);
    draw_status(app);
    draw_buttons();
}
