/* app.c - see app.h. No raylib in this file (BUILD_INSTRUCTIONS.txt 4.2). */
#include "app.h"

void app_init(AppState *app)
{
    grid_clear(app->grid);
    app->n = START_N;
    app->cur_row = 0;
    app->cur_col = 0;
    app->color = 0x00000000u;
}

int do_paint(AppState *app)
{
    if (grid_get(app->grid, app->cur_row, app->cur_col) == app->color) {
        return 0;
    }
    grid_set(app->grid, app->cur_row, app->cur_col, app->color);
    return 1;
}
