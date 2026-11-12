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

int do_paint(AppState *app)
{
    if (grid_get(app->grid, app->cur_row, app->cur_col) == app->color) {
        return 0;
    }
    grid_set(app->grid, app->cur_row, app->cur_col, app->color);
    return 1;
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

void entry_add_char(AppState *app, int c)
{
    if (c < 0x20 || c > 0x7E || app->entry_len >= ENTRY_MAX_LEN) {
        return;
    }
    app->entry_buf[app->entry_len++] = (char)c;
    app->entry_buf[app->entry_len] = '\0';
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
