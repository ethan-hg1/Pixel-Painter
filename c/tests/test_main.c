/* test_main.c - tiny assert-based test runner. No raylib.
 * Exits 0 when every check passes. Add one test function per module and
 * call it from main(). Required list: BUILD_INSTRUCTIONS.txt 12.5. */
#include <stdio.h>
#include <string.h>

#include "app.h"
#include "config.h"
#include "grid.h"
#include "hexparse.h"

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

/* 12.5: parse_hex valid / invalid list, plus edge cases. */
static void test_parse_hex(void)
{
    uint32_t c = 0x00ABCDEFu;

    CHECK(parse_hex("#FF5733", &c) == 1 && c == 0x00FF5733u);
    CHECK(parse_hex("ff5733", &c) == 1 && c == 0x00FF5733u);
    CHECK(parse_hex("#ff5733", &c) == 1 && c == 0x00FF5733u);
    CHECK(parse_hex("#00FF00", &c) == 1 && c == 0x0000FF00u);
    CHECK(parse_hex("000000", &c) == 1 && c == 0x00000000u);
    CHECK(parse_hex("#aBcDeF", &c) == 1 && c == 0x00ABCDEFu);

    c = 0x00123456u;
    CHECK(parse_hex("", &c) == 0);
    CHECK(parse_hex("#", &c) == 0);
    CHECK(parse_hex("FF573", &c) == 0);
    CHECK(parse_hex("FF57333", &c) == 0);
    CHECK(parse_hex("#GG0000", &c) == 0);
    CHECK(parse_hex("12 456", &c) == 0);
    CHECK(parse_hex("#12345G", &c) == 0);
    CHECK(parse_hex("##FF573", &c) == 0);
    CHECK(parse_hex("FF5733#", &c) == 0);
    CHECK(parse_hex("0xFF57", &c) == 0);
    CHECK(c == 0x00123456u);  /* invalid input leaves *out unchanged */

    CHECK(hex_digit_value('0') == 0 && hex_digit_value('9') == 9);
    CHECK(hex_digit_value('a') == 10 && hex_digit_value('F') == 15);
    CHECK(hex_digit_value('g') == -1 && hex_digit_value('/') == -1);
    CHECK(hex_digit_value(':') == -1 && hex_digit_value('@') == -1);
    CHECK(hex_digit_value('`') == -1 && hex_digit_value('G') == -1);
}

static void type_string(AppState *app, const char *s)
{
    for (; *s; s++) {
        entry_add_char(app, (unsigned char)*s);
    }
}

/* Step 2: color entry mode (13: "#00FF00" -> green, "#12345G" -> invalid,
 * Esc cancels, Backspace works, buffer bounded). */
static void test_color_entry(void)
{
    AppState app;
    app_init(&app);
    CHECK(app.entry_mode == 0 && app.status[0] == '\0');

    entry_start(&app);
    CHECK(app.entry_mode == 1 && app.entry_len == 0);
    type_string(&app, "#00FF00");
    CHECK(strcmp(app.entry_buf, "#00FF00") == 0);
    CHECK(entry_confirm(&app) == 1);
    CHECK(app.color == 0x0000FF00u && app.entry_mode == 0);

    entry_start(&app);
    type_string(&app, "#12345G");
    CHECK(entry_confirm(&app) == 0);
    CHECK(app.color == 0x0000FF00u);              /* unchanged */
    CHECK(strcmp(app.status, "Invalid color") == 0);
    CHECK(app.entry_mode == 0);

    entry_start(&app);                            /* clears the message */
    CHECK(app.status[0] == '\0');
    type_string(&app, "ff0000");
    entry_cancel(&app);
    CHECK(app.color == 0x0000FF00u && app.entry_mode == 0);

    entry_start(&app);
    type_string(&app, "#FF00001");                /* 8th char dropped */
    CHECK(app.entry_len == ENTRY_MAX_LEN);
    CHECK(strcmp(app.entry_buf, "#FF0000") == 0);
    entry_backspace(&app);
    entry_backspace(&app);
    CHECK(strcmp(app.entry_buf, "#FF00") == 0);
    type_string(&app, "FF");
    CHECK(entry_confirm(&app) == 1 && app.color == 0x00FF00FFu);

    entry_start(&app);
    entry_backspace(&app);                        /* on empty: no-op */
    CHECK(app.entry_len == 0);
    entry_add_char(&app, 0x1B);                   /* control chars ignored */
    entry_add_char(&app, '\n');
    entry_add_char(&app, 0x7F);
    CHECK(app.entry_len == 0);
    CHECK(entry_confirm(&app) == 0);              /* empty is invalid */
    CHECK(app.color == 0x00FF00FFu);
}

int main(void)
{
    test_config();
    test_grid_address();
    test_cell_layout();
    test_luminance();
    test_paint();
    test_parse_hex();
    test_color_entry();

    printf("%d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
