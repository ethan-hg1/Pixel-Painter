/* test_main.c - tiny assert-based test runner. No raylib.
 * Exits 0 when every check passes. Add one test function per module and
 * call it from main(). Required list: BUILD_INSTRUCTIONS.txt 12.5. */
#include <stdio.h>

#include "app.h"
#include "config.h"
#include "grid.h"

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        g_checks++;                                                       \
        if (!(cond)) {                                                    \
            g_failures++;                                                 \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                 \
    } while (0)

static void test_config(void)
{
    CHECK(MIN_N == 8);
    CHECK(MAX_N == 16);
    CHECK(START_N >= MIN_N && START_N <= MAX_N);
    CHECK(GRID_STRIDE == MAX_N);
    CHECK((HISTORY_DEPTH & (HISTORY_DEPTH - 1)) == 0); /* power of two */
    CHECK(EMPTY != 0x00000000u);                       /* black is a color */
    CHECK(CANVAS_UNITS <= DISPLAY_W && CANVAS_UNITS <= DISPLAY_H);
}

/* 12.5: cell address formula for the four corners. */
static void test_grid_address(void)
{
    /* byte offset = ((row << 4) + col) << 2, as in the asm */
    CHECK((grid_index(0, 0) << 2) == 0);
    CHECK((grid_index(0, 15) << 2) == 60);
    CHECK((grid_index(15, 0) << 2) == 960);
    CHECK((grid_index(15, 15) << 2) == 1020);
    CHECK(grid_index(15, 15) == GRID_CELLS - 1);

    uint32_t grid[GRID_CELLS];
    grid_clear(grid);
    for (int i = 0; i < GRID_CELLS; i++) {
        CHECK(grid[i] == EMPTY);
    }
    grid_set(grid, 15, 0, 0x00123456u);
    CHECK(grid[240] == 0x00123456u);
    CHECK(grid_get(grid, 15, 0) == 0x00123456u);
}

/* 12.5: cell size/offset for every n from 8 to 16 (asm canvas, 96 units). */
static void test_cell_layout(void)
{
    static const int expect_cell[] = { 12, 10, 9, 8, 8, 7, 6, 6, 6 };
    static const int expect_off[]  = {  0,  3, 3, 4, 0, 2, 6, 3, 0 };
    for (int n = MIN_N; n <= MAX_N; n++) {
        int cell = grid_cell_size(CANVAS_UNITS, n);
        int off = grid_offset(CANVAS_UNITS, n);
        CHECK(cell == expect_cell[n - MIN_N]);
        CHECK(off == expect_off[n - MIN_N]);
        CHECK(2 * off + n * cell <= CANVAS_UNITS); /* fits, centered */
    }
}

/* 12.5: luminance picks the cursor color. */
static void test_luminance(void)
{
    CHECK(luminance_q8(0x00000000u) == 0);
    CHECK(luminance_q8(0x00FFFFFFu) == 255);  /* (77+150+29)*255 >> 8 */
    CHECK(cursor_color(0x00000000u) == 0x00FFFFFFu); /* black -> white */
    CHECK(cursor_color(0x00FFFFFFu) == 0x00000000u); /* white -> black */
    CHECK(cursor_color(EMPTY) == 0x00000000u);       /* EMPTY is light */
    CHECK(cursor_color(0x000000FFu) == 0x00FFFFFFu); /* blue: Y = 28 */
    CHECK(cursor_color(0x0000FF00u) == 0x00000000u); /* green: Y = 149 */
}

/* Step 1: app_init and do_paint. */
static void test_paint(void)
{
    AppState app;
    app_init(&app);
    CHECK(app.n == START_N);
    CHECK(app.cur_row == 0 && app.cur_col == 0);
    CHECK(app.color == 0x00000000u);
    CHECK(grid_get(app.grid, 0, 0) == EMPTY);

    CHECK(do_paint(&app) == 1);
    CHECK(app.grid[0] == 0x00000000u);
    CHECK(do_paint(&app) == 0); /* same color: no change */

    app.color = 0x00FF5733u;
    app.cur_row = 3;
    app.cur_col = 5;
    CHECK(do_paint(&app) == 1);
    CHECK(app.grid[3 * GRID_STRIDE + 5] == 0x00FF5733u);
}

int main(void)
{
    test_config();
    test_grid_address();
    test_cell_layout();
    test_luminance();
    test_paint();

    printf("%d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
