/* app.c - see app.h. No raylib in this file (BUILD_INSTRUCTIONS.txt 4.2). */
#include "app.h"

#include <string.h>

#include "hexparse.h"

static void set_status(AppState *app, const char *msg)
{
    strncpy(app->status, msg, STATUS_LEN - 1);
    app->status[STATUS_LEN - 1] = '\0';
}

void app_init(AppState *app)
{
    grid_clear(app->grid);
    app->n = START_N;
    app->cur_row = 0;
    app->cur_col = 0;
    app->color = 0x00000000u;
    app->entry_mode = 0;
    app->entry_len = 0;
    app->entry_buf[0] = '\0';
    app->status[0] = '\0';
}

/* Shared by paint and erase: writes color to the cursor cell unless it
 * already holds it. */
static int set_cursor_cell(AppState *app, uint32_t color)
{
    if (grid_get(app->grid, app->cur_row, app->cur_col) == color) {
        return 0;
    }
    grid_set(app->grid, app->cur_row, app->cur_col, color);
    return 1;
}

int do_paint(AppState *app)
{
    return set_cursor_cell(app, app->color);
}

int do_erase(AppState *app)
{
    return set_cursor_cell(app, EMPTY);
}

int do_clear(AppState *app)
{
    int changed = 0;

    for (int row = 0; row < app->n; row++) {
        for (int col = 0; col < app->n; col++) {
            if (grid_get(app->grid, row, col) != EMPTY) {
                grid_set(app->grid, row, col, EMPTY);
                changed = 1;
            }
        }
    }
    return changed;
}

int do_resize(AppState *app, int delta)
{
    int next = app->n + delta;

    if (next == app->n || next < MIN_N || next > MAX_N) {
        return 0;
    }
    if (next < app->n) {
        /* cells outside the new size become EMPTY (undo can restore them) */
        for (int row = 0; row < app->n; row++) {
            for (int col = 0; col < app->n; col++) {
                if (row >= next || col >= next) {
                    grid_set(app->grid, row, col, EMPTY);
                }
            }
        }
    }
    app->n = next;
    move_cursor(app, 0, 0);     /* clamps the cursor into the new grid */
    return 1;
}

static int clamp(int v, int lo, int hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

int move_cursor(AppState *app, int drow, int dcol)
{
    int row = clamp(app->cur_row + drow, 0, app->n - 1);
    int col = clamp(app->cur_col + dcol, 0, app->n - 1);
    int moved = (row != app->cur_row || col != app->cur_col);

    app->cur_row = row;
    app->cur_col = col;
    return moved;
}

void set_color(AppState *app, uint32_t color)
{
    app->color = color & 0x00FFFFFFu;
}

void entry_start(AppState *app)
{
    app->entry_mode = 1;
    app->entry_len = 0;
    app->entry_buf[0] = '\0';
    set_status(app, "");
}

int entry_add_char(AppState *app, int c)
{
    if (c < 0x20 || c > 0x7E || app->entry_len >= ENTRY_MAX_LEN) {
        return 0;
    }
    app->entry_buf[app->entry_len++] = (char)c;
    app->entry_buf[app->entry_len] = '\0';
    return 1;
}

void entry_backspace(AppState *app)
{
    if (app->entry_len > 0) {
        app->entry_buf[--app->entry_len] = '\0';
    }
}

void entry_cancel(AppState *app)
{
    app->entry_mode = 0;
}

int entry_confirm(AppState *app)
{
    uint32_t color;

    app->entry_mode = 0;
    if (!parse_hex(app->entry_buf, &color)) {
        set_status(app, "Invalid color");
        return 0;
    }
    set_color(app, color);
    return 1;
}
