/* test_main.c - tiny assert-based test runner. No raylib.
 * Exits 0 when every check passes. Add one test function per module and
 * call it from main(). Required list: BUILD_INSTRUCTIONS.txt 12.5. */
#include <stdio.h>
#include <string.h>

#include "app.h"
#include "config.h"
#include "export_bmp.h"
#include "export_png.h"
#include "grid.h"
#include "hexparse.h"
#include "palette.h"
#include "replay.h"

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
    CHECK(entry_add_char(&app, 0x1B) == 0);      /* control chars ignored */
    CHECK(entry_add_char(&app, '\n') == 0);
    CHECK(entry_add_char(&app, 0x7F) == 0);
    CHECK(entry_add_char(&app, 'a') == 1);
    entry_backspace(&app);
    CHECK(app.entry_len == 0);
    CHECK(entry_confirm(&app) == 0);              /* empty is invalid */
    CHECK(app.color == 0x00FF00FFu);
}

/* Step 3: erase, cursor clamping at all four edges, mouse hit testing. */
static void test_erase_and_move(void)
{
    static const int sizes[] = { MIN_N, MAX_N };
    for (int s = 0; s < 2; s++) {
        AppState app;
        int n = sizes[s];
        app_init(&app);
        app.n = n;
        int last = n - 1;

        /* clamp at each edge; no wrap */
        CHECK(move_cursor(&app, -1, 0) == 0 && app.cur_row == 0);
        CHECK(move_cursor(&app, 0, -1) == 0 && app.cur_col == 0);
        CHECK(move_cursor(&app, 1, 0) == 1 && app.cur_row == 1);
        CHECK(move_cursor(&app, 0, 1) == 1 && app.cur_col == 1);
        CHECK(move_cursor(&app, 100, 100) == 1 && app.cur_row == last);
        CHECK(app.cur_col == last);
        CHECK(move_cursor(&app, 1, 0) == 0 && app.cur_row == last);
        CHECK(move_cursor(&app, 0, 1) == 0 && app.cur_col == last);
        CHECK(move_cursor(&app, -100, -100) == 1);
        CHECK(app.cur_row == 0 && app.cur_col == 0);
    }

    AppState app;
    app_init(&app);
    app.color = 0x00ABCDEFu;
    CHECK(do_erase(&app) == 0 && grid_get(app.grid, 0, 0) == EMPTY);  /* no-op */
    CHECK(do_paint(&app) == 1 && grid_get(app.grid, 0, 0) == 0x00ABCDEFu);
    CHECK(do_erase(&app) == 1 && grid_get(app.grid, 0, 0) == EMPTY);
    CHECK(do_erase(&app) == 0);                       /* already EMPTY */
    CHECK(app.color == 0x00ABCDEFu);                  /* erase keeps color */
}

/* Step 3: pixel -> cell. Canvas at (30, 30), 640 px square. */
static void test_cell_hit(void)
{
    int row = -7, col = -7;

    /* n = 8: cell 80, no margin */
    CHECK(grid_cell_at(30, 30, 30, 30, 640, 8, &row, &col) == 1);
    CHECK(row == 0 && col == 0);
    CHECK(grid_cell_at(109, 109, 30, 30, 640, 8, &row, &col) == 1);
    CHECK(row == 0 && col == 0);                      /* last px of cell */
    CHECK(grid_cell_at(110, 110, 30, 30, 640, 8, &row, &col) == 1);
    CHECK(row == 1 && col == 1);
    CHECK(grid_cell_at(669, 669, 30, 30, 640, 8, &row, &col) == 1);
    CHECK(row == 7 && col == 7);                      /* bottom-right px */
    CHECK(grid_cell_at(670, 500, 30, 30, 640, 8, &row, &col) == 0);
    CHECK(grid_cell_at(500, 670, 30, 30, 640, 8, &row, &col) == 0);
    CHECK(grid_cell_at(29, 500, 30, 30, 640, 8, &row, &col) == 0);
    CHECK(grid_cell_at(500, 29, 30, 30, 640, 8, &row, &col) == 0);

    /* n = 16: cell 40 */
    CHECK(grid_cell_at(30 + 40 * 15 + 39, 30 + 40 * 15, 30, 30, 640, 16,
                       &row, &col) == 1);
    CHECK(row == 15 && col == 15);
    CHECK(grid_cell_at(30 + 40 * 16, 40, 30, 30, 640, 16, &row, &col) == 0);

    /* n = 11: cell 58, 638 px used, offset 1 -> margin on the left and top */
    row = col = -7;
    CHECK(grid_cell_at(30, 30, 30, 30, 640, 11, &row, &col) == 0);  /* margin */
    CHECK(grid_cell_at(30, 500, 30, 30, 640, 11, &row, &col) == 0);
    CHECK(row == -7 && col == -7);                    /* untouched */
    CHECK(grid_cell_at(31, 31, 30, 30, 640, 11, &row, &col) == 1);
    CHECK(row == 0 && col == 0);
    CHECK(grid_cell_at(31 + 58, 31 + 58 * 2, 30, 30, 640, 11, &row, &col) == 1);
    CHECK(row == 2 && col == 1);
    CHECK(grid_cell_at(31 + 11 * 58, 300, 30, 30, 640, 11, &row, &col) == 0);
    CHECK(grid_cell_at(31 + 11 * 58 - 1, 300, 30, 30, 640, 11, &row, &col) == 1);
    CHECK(col == 10);                                 /* last used px */
}

/* Step 4: clear, and resize with its limits, shrink clearing and cursor clamp. */
static void test_clear_and_resize(void)
{
    AppState app;
    app_init(&app);
    CHECK(do_clear(&app) == 0);                  /* already empty: no change */

    app.color = 0x00112233u;
    do_paint(&app);
    app.cur_row = 3;
    app.cur_col = 4;
    do_paint(&app);
    CHECK(do_clear(&app) == 1);
    CHECK(grid_get(app.grid, 0, 0) == EMPTY && grid_get(app.grid, 3, 4) == EMPTY);
    CHECK(do_clear(&app) == 0);

    /* limits: 8 can't shrink, 16 can't grow, steps stay within 8..16 */
    app_init(&app);
    CHECK(do_resize(&app, -RESIZE_STEP) == 0 && app.n == MIN_N);
    CHECK(do_resize(&app, RESIZE_STEP) == 1 && app.n == MIN_N + RESIZE_STEP);
    while (app.n < MAX_N) {
        CHECK(do_resize(&app, RESIZE_STEP) == 1);
    }
    CHECK(app.n == MAX_N);
    CHECK(do_resize(&app, RESIZE_STEP) == 0 && app.n == MAX_N);
    while (app.n > MIN_N) {
        CHECK(do_resize(&app, -RESIZE_STEP) == 1);
    }
    CHECK(app.n == MIN_N);

    /* growing keeps colors; shrinking clears what falls outside */
    app_init(&app);
    app.n = MAX_N;                               /* via the limit, as in main */
    app.color = 0x00ABCDEFu;
    app.cur_row = 15; app.cur_col = 15; do_paint(&app);
    app.cur_row = 0;  app.cur_col = 0;  do_paint(&app);
    app.cur_row = 15; app.cur_col = 15;
    CHECK(do_resize(&app, -RESIZE_STEP) == 1);   /* 16 -> 15 */
    CHECK(app.n == 15);
    CHECK(grid_get(app.grid, 15, 15) == EMPTY);  /* outside: cleared */
    CHECK(grid_get(app.grid, 0, 0) == 0x00ABCDEFu);  /* inside: kept */
    CHECK(app.cur_row == 14 && app.cur_col == 14);   /* cursor clamped */
    CHECK(do_resize(&app, RESIZE_STEP) == 1);    /* grow back: 15 -> 16 */
    CHECK(grid_get(app.grid, 0, 0) == 0x00ABCDEFu);
    CHECK(grid_get(app.grid, 15, 15) == EMPTY);  /* cleared stays cleared */
    CHECK(app.cur_row == 14 && app.cur_col == 14);

    /* the cursor is only clamped, not moved, when it is already inside */
    app_init(&app);
    app.cur_row = 2; app.cur_col = 5;
    CHECK(do_resize(&app, RESIZE_STEP) == 1);
    CHECK(app.cur_row == 2 && app.cur_col == 5);
}

/* 12.5 history: paint/undo/redo, redo cleared by a new action, the ring
 * dropping the oldest record, and undo of clear and resize. */
static void test_undo_redo(void)
{
    static AppState app;   /* static: AppState now holds a 66 KB history */
    int n;

    /* paint 3 cells, undo 3 -> all empty; redo 3 -> back */
    app_init(&app);
    set_color(&app, 0x00112233u);
    CHECK(do_paint(&app) == 1);                 /* (0,0) */
    move_cursor(&app, 0, 1);
    CHECK(do_paint(&app) == 1);                 /* (0,1) */
    app.cur_row = 1; app.cur_col = 0;
    CHECK(do_paint(&app) == 1);                 /* (1,0) */
    CHECK(do_undo(&app) == 1);
    CHECK(do_undo(&app) == 1);
    CHECK(do_undo(&app) == 1);
    CHECK(do_undo(&app) == 0);                  /* nothing left */
    CHECK(grid_get(app.grid, 0, 0) == EMPTY);
    CHECK(grid_get(app.grid, 0, 1) == EMPTY);
    CHECK(grid_get(app.grid, 1, 0) == EMPTY);
    CHECK(do_redo(&app) == 1);
    CHECK(do_redo(&app) == 1);
    CHECK(do_redo(&app) == 1);
    CHECK(do_redo(&app) == 0);                  /* nothing left */
    CHECK(grid_get(app.grid, 0, 0) == 0x00112233u);
    CHECK(grid_get(app.grid, 0, 1) == 0x00112233u);
    CHECK(grid_get(app.grid, 1, 0) == 0x00112233u);

    /* a no-op paint or erase pushes nothing */
    app_init(&app);
    set_color(&app, 0x00112233u);
    do_paint(&app);
    CHECK(do_paint(&app) == 0);
    CHECK(do_erase(&app) == 1);
    CHECK(do_erase(&app) == 0);
    CHECK(do_undo(&app) == 1);                  /* undoes the erase */
    CHECK(grid_get(app.grid, 0, 0) == 0x00112233u);
    CHECK(do_undo(&app) == 1);                  /* undoes the paint */
    CHECK(grid_get(app.grid, 0, 0) == EMPTY);
    CHECK(do_undo(&app) == 0);

    /* a new action after an undo discards the redo history */
    app_init(&app);
    set_color(&app, 0x00112233u);
    do_paint(&app);                             /* (0,0) */
    do_undo(&app);
    move_cursor(&app, 2, 2);
    CHECK(do_paint(&app) == 1);                 /* (2,2) */
    CHECK(do_redo(&app) == 0);                  /* redo is gone */
    CHECK(grid_get(app.grid, 0, 0) == EMPTY);
    CHECK(grid_get(app.grid, 2, 2) == 0x00112233u);

    /* undo after clear restores the cells; redo clears them again */
    app_init(&app);
    set_color(&app, 0x00ABCDEFu);
    do_paint(&app);                             /* (0,0) */
    move_cursor(&app, 5, 7);
    do_paint(&app);                             /* (5,7) */
    CHECK(do_clear(&app) == 1);
    CHECK(grid_get(app.grid, 0, 0) == EMPTY);
    CHECK(grid_get(app.grid, 5, 7) == EMPTY);
    CHECK(do_clear(&app) == 0);                 /* already empty: no record */
    CHECK(do_undo(&app) == 1);                  /* undoes the clear, not a no-op */
    CHECK(grid_get(app.grid, 0, 0) == 0x00ABCDEFu);
    CHECK(grid_get(app.grid, 5, 7) == 0x00ABCDEFu);
    CHECK(do_redo(&app) == 1);
    CHECK(grid_get(app.grid, 0, 0) == EMPTY);
    CHECK(grid_get(app.grid, 5, 7) == EMPTY);

    /* undo after shrink restores the cleared cells and n */
    app_init(&app);
    set_color(&app, 0x00ABCDEFu);
    do_resize(&app, RESIZE_STEP);               /* 8 -> 9 */
    CHECK(app.n == 9);
    do_paint(&app);                             /* (0,0) */
    app.cur_row = 8; app.cur_col = 8;
    do_paint(&app);                             /* (8,8), the corner */
    CHECK(do_resize(&app, -RESIZE_STEP) == 1);  /* 9 -> 8 clears (8,8) */
    CHECK(app.n == 8);
    CHECK(grid_get(app.grid, 8, 8) == EMPTY);
    CHECK(app.cur_row == 7 && app.cur_col == 7);
    CHECK(do_undo(&app) == 1);                  /* back to 9 */
    CHECK(app.n == 9);
    CHECK(grid_get(app.grid, 8, 8) == 0x00ABCDEFu);
    CHECK(app.cur_row == 7 && app.cur_col == 7);
    CHECK(do_redo(&app) == 1);                  /* 9 -> 8 again */
    CHECK(app.n == 8);
    CHECK(grid_get(app.grid, 8, 8) == EMPTY);
    CHECK(grid_get(app.grid, 0, 0) == 0x00ABCDEFu);
    /* a resize refused at the limit (8 is the minimum) records nothing */
    app_init(&app);
    CHECK(do_resize(&app, -RESIZE_STEP) == 0);
    CHECK(do_undo(&app) == 0);

    /* 65 actions: the oldest record is dropped, so 64 undos are possible.
     * Painting cell i with color i+1 for i = 0..64 (65 actions). */
    app_init(&app);
    app.n = MAX_N;
    for (int i = 0; i < 65; i++) {
        set_color(&app, (uint32_t)(i + 1));
        app.cur_row = i / MAX_N; app.cur_col = i % MAX_N;
        CHECK(do_paint(&app) == 1);
    }
    n = 0;
    while (do_undo(&app)) {
        n++;
    }
    CHECK(n == HISTORY_DEPTH);                  /* 64, not 65 */
    CHECK(grid_get(app.grid, 0, 0) == 1u);      /* oldest action can't undo */
    CHECK(grid_get(app.grid, 0, 1) == EMPTY);   /* action 2 was undone */
    CHECK(grid_get(app.grid, 4, 0) == EMPTY);   /* action 65 undone */
    CHECK(do_undo(&app) == 0);

    /* 70 actions in a row, then undo 70 times: exactly 64 undo, no crash */
    app_init(&app);
    app.n = MAX_N;
    for (int i = 0; i < 70; i++) {
        set_color(&app, (uint32_t)(i + 1));
        app.cur_row = i / MAX_N; app.cur_col = i % MAX_N;
        do_paint(&app);
    }
    n = 0;
    for (int i = 0; i < 70; i++) {
        n += do_undo(&app);
    }
    CHECK(n == HISTORY_DEPTH);
    CHECK(do_undo(&app) == 0);
    /* and every one of the 64 comes back on redo */
    n = 0;
    for (int i = 0; i < 70; i++) {
        n += do_redo(&app);
    }
    CHECK(n == HISTORY_DEPTH);
    CHECK(grid_get(app.grid, 4, 5) == 70u);     /* action 70 is cell 69 = (4, 5) */
    CHECK(grid_get(app.grid, 0, 0) == 1u);      /* oldest was dropped, so it stayed */
}

/* 12.5 palette: no duplicates, FIFO replace when full, empty slot pick is a
 * no-op. Also: paint auto-saves, p saves, and palette changes skip undo. */
static void test_palette(void)
{
    uint32_t slots[PALETTE_SLOTS];
    int next;
    AppState app;

    palette_init(slots, &next);
    CHECK(next == 0);
    for (int i = 0; i < PALETTE_SLOTS; i++) {
        CHECK(slots[i] == EMPTY);
    }

    /* saves go to slots 0, 1, 2 in order */
    CHECK(palette_insert(slots, &next, 0x00AA0000u) == 1);
    CHECK(palette_insert(slots, &next, 0x0000BB00u) == 1);
    CHECK(palette_insert(slots, &next, 0x000000CCu) == 1);
    CHECK(next == 3);
    CHECK(palette_find(slots, 0x0000BB00u) == 1);
    CHECK(palette_find(slots, 0x00123456u) == -1);

    /* no duplicates, EMPTY never saved, and next does not move */
    CHECK(palette_insert(slots, &next, 0x00AA0000u) == 0);
    CHECK(palette_insert(slots, &next, EMPTY) == 0);
    CHECK(next == 3);

    /* fill the last five slots with distinct colors (black counts) */
    CHECK(palette_insert(slots, &next, 0x00000000u) == 1);
    for (uint32_t c = 0x00010101u; c <= 0x00040404u; c += 0x00010101u) {
        CHECK(palette_insert(slots, &next, c) == 1);
    }
    CHECK(next == 0);                            /* 8 saves wrap the ring */

    /* 9th distinct color overwrites the oldest (slot 0), then slot 1 next */
    CHECK(palette_insert(slots, &next, 0x00FFFFFFu) == 1);
    CHECK(slots[0] == 0x00FFFFFFu);
    CHECK(palette_find(slots, 0x00AA0000u) == -1);
    CHECK(next == 1);
    CHECK(palette_insert(slots, &next, 0x00FE0000u) == 1);
    CHECK(slots[1] == 0x00FE0000u);
    CHECK(palette_find(slots, 0x0000BB00u) == -1);
    CHECK(palette_find(slots, 0x000000CCu) == 2);   /* still there */

    /* swatch hit test: 2 columns at x 710 and 790, rows 60 apart */
    {
        int slot = -1;
        CHECK(palette_slot_at(710, 390, 710, 390, 50, 80, 60, &slot) == 1);
        CHECK(slot == 0);
        CHECK(palette_slot_at(800, 400, 710, 390, 50, 80, 60, &slot) == 1);
        CHECK(slot == 1);
        CHECK(palette_slot_at(800, 580, 710, 390, 50, 80, 60, &slot) == 1);
        CHECK(slot == 7);
        CHECK(palette_slot_at(760, 400, 710, 390, 50, 80, 60, &slot) == 0); /* gap */
        CHECK(palette_slot_at(720, 440, 710, 390, 50, 80, 60, &slot) == 0); /* gap */
        CHECK(palette_slot_at(709, 400, 710, 390, 50, 80, 60, &slot) == 0); /* left */
        CHECK(palette_slot_at(720, 620, 710, 390, 50, 80, 60, &slot) == 0); /* below */
        CHECK(palette_slot_at(870, 400, 710, 390, 50, 80, 60, &slot) == 0); /* right */
    }

    /* app level: paint auto-saves, p saves, empty pick no-op, pick sets color */
    app_init(&app);
    CHECK(palette_pick(&app, 0) == 0);          /* empty slot does nothing */
    CHECK(app.color == 0x00000000u);
    set_color(&app, 0x00112233u);
    CHECK(palette_add(&app) == 1);              /* p saves */
    CHECK(palette_add(&app) == 0);              /* already saved */
    set_color(&app, 0x00445566u);
    CHECK(do_paint(&app) == 1);                 /* first paint auto-saves */
    CHECK(palette_find(app.palette, 0x00445566u) == 1);
    CHECK(do_paint(&app) == 0);                 /* same cell, same color */
    CHECK(palette_find(app.palette, 0x00445566u) == 1);  /* no duplicate */
    CHECK(palette_pick(&app, 0) == 1);
    CHECK(app.color == 0x00112233u);
    CHECK(palette_pick(&app, 0) == 0);          /* already current */
    CHECK(palette_pick(&app, 5) == 0);          /* empty slot */
    CHECK(app.color == 0x00112233u);
    CHECK(palette_pick(&app, 8) == 0);          /* out of range */
    CHECK(app.color == 0x00112233u);

    /* palette is not part of undo: undo the paint, the swatch stays */
    CHECK(do_undo(&app) == 1);
    CHECK(grid_get(app.grid, 0, 0) == EMPTY);
    CHECK(palette_find(app.palette, 0x00445566u) == 1);
    CHECK(do_redo(&app) == 1);
    CHECK(palette_find(app.palette, 0x00445566u) == 1);
}

/* 12.5 export: crc32("IEND"), adler32("Wikipedia"), the PNG layout and
 * every chunk CRC, the stored block LEN/NLEN, the adler-32 trailer, EMPTY
 * as white and the 8x scale. The 8x8 and 16x16 files are compared with the
 * golden images in CI (replay job). */
static uint32_t be32(const unsigned char *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void test_export(void)
{
    static const unsigned char wiki[] = "Wikipedia";
    static unsigned char png[PNG_MAX_BYTES];
    static const unsigned char sig[8] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
    uint32_t grid[GRID_CELLS];
    size_t len;

    CHECK(crc32_bytes((const unsigned char *)"IEND", 4) == 0xAE426082u);
    CHECK(adler32_bytes(wiki, 9) == 0x11E60398u);

    grid_clear(grid);
    len = export_png_encode(grid, 8, png, sizeof png);
    /* n = 8 at 8x scale: 64 x 64 px, raw = 64 * (1 + 192) = 12352 bytes.
     * File = 8 + IHDR 25 + IDAT (12 + 2 + 5 + 12352 + 4) + IEND 12. */
    CHECK(len == 12420);
    CHECK(memcmp(png, sig, 8) == 0);
    CHECK(be32(png + 8) == 13);
    CHECK(memcmp(png + 12, "IHDR", 4) == 0);
    CHECK(be32(png + 16) == 64);                 /* width */
    CHECK(be32(png + 20) == 64);                 /* height */
    CHECK(png[24] == 8 && png[25] == 2);         /* 8-bit RGB */
    CHECK(png[26] == 0 && png[27] == 0 && png[28] == 0);
    CHECK(be32(png + 29) == crc32_bytes(png + 12, 17));  /* IHDR CRC */

    /* the IDAT header: zlib 78 01, final stored block, LEN and NLEN (LE) */
    CHECK(be32(png + 33) == 12363);              /* IDAT data length */
    CHECK(memcmp(png + 37, "IDAT", 4) == 0);
    CHECK(png[41] == 0x78 && png[42] == 0x01);   /* zlib header, 0x7801 % 31 == 0 */
    CHECK(png[43] == 0x01);                      /* BFINAL = 1, BTYPE = 00 */
    CHECK(png[44] == 0x40 && png[45] == 0x30);   /* LEN = 12352 */
    CHECK(png[46] == 0xBF && png[47] == 0xCF);   /* NLEN = ~12352 */
    /* EMPTY is white: row 0 filter byte 0, then white pixels */
    CHECK(png[48] == 0 && png[49] == 0xFF && png[50] == 0xFF && png[51] == 0xFF);
    CHECK(png[240] == 0xFF);                     /* last byte of row 0 */
    /* adler-32 trailer over the 12352 raw bytes */
    CHECK(be32(png + 48 + 12352) == adler32_bytes(png + 48, 12352));
    /* IDAT CRC over "IDAT" + its data (4 + 12363 - 4 + 4 bytes: type+data) */
    CHECK(be32(png + 41 + 12363) == crc32_bytes(png + 37, 4 + 12363));
    CHECK(be32(png + len - 4) == 0xAE426082u);   /* IEND CRC */
    CHECK(be32(png + len - 12) == 0);            /* IEND length */

    /* a painted cell: cell (0, 0) red is pixels (0..7, 0..7) of the image */
    grid_set(grid, 0, 0, 0x00FF0000u);
    len = export_png_encode(grid, 8, png, sizeof png);
    CHECK(png[49] == 0xFF && png[50] == 0x00 && png[51] == 0x00);
    CHECK(png[49 + 7 * 3] == 0xFF);              /* pixel x = 7 is still red */
    CHECK(png[49 + 8 * 3] == 0xFF && png[50 + 8 * 3] == 0xFF); /* x = 8 white */
    CHECK(png[49 + 8 * 3 + 2] == 0xFF);

    /* 16x16 at 8x scale is 128 x 128 and the largest file (49348 bytes) */
    grid_clear(grid);
    len = export_png_encode(grid, 16, png, sizeof png);
    CHECK(len == PNG_MAX_BYTES);
    CHECK(be32(png + 16) == 128 && be32(png + 20) == 128);
    CHECK(be32(png + len - 4) == 0xAE426082u);

    /* too small a buffer, and out-of-range n, write nothing */
    CHECK(export_png_encode(grid, 8, png, 100) == 0);
    CHECK(export_png_encode(grid, 7, png, sizeof png) == 0);
    CHECK(export_png_encode(grid, 17, png, sizeof png) == 0);

    /* BMP: 54-byte header, rows bottom-up, B G R order */
    {
        static unsigned char bmp[BMP_MAX_BYTES];
        size_t bmp_len;
        grid_set(grid, 7, 0, 0x00112233u);       /* bottom-left cell */
        bmp_len = export_bmp_encode(grid, 8, bmp, sizeof bmp);
        CHECK(bmp_len == 54 + 64 * 64 * 3);
        CHECK(bmp[0] == 'B' && bmp[1] == 'M');
        CHECK(bmp[2] + (bmp[3] << 8) == (int)((54 + 64 * 64 * 3) & 0xFFFF));
        CHECK(bmp[10] == 54 && bmp[11] == 0);     /* pixel offset */
        CHECK(bmp[18] == 64 && bmp[22] == 64);    /* width, height */
        CHECK(bmp[28] == 24);                     /* 24 bits per pixel */
        /* first stored row = image row 63 = cells row 7: x 0..7 are 33 22 11 */
        CHECK(bmp[54] == 0x33 && bmp[55] == 0x22 && bmp[56] == 0x11);
        CHECK(export_bmp_encode(grid, 8, bmp, 100) == 0);
    }
}

/* Replay tokens (decision 3): asm key names, uppercase folds, "t #RRGGBB",
 * unknown token and bad color are errors. */
static void test_replay_tokens(void)
{
    AppState app;
    char err[96];

    app_init(&app);
    CHECK(replay_apply_line(&app, "w\n", NULL, err, sizeof err) == 0);
    CHECK(grid_get(app.grid, 0, 0) == 0x00000000u);   /* black painted */
    CHECK(replay_apply_line(&app, "# a comment\n", NULL, err, sizeof err) == 0);
    CHECK(replay_apply_line(&app, "\n", NULL, err, sizeof err) == 0);
    CHECK(replay_apply_line(&app, "t #FF5733\n", NULL, err, sizeof err) == 0);
    CHECK(app.color == 0x00FF5733u);
    CHECK(replay_apply_line(&app, "L\n", NULL, err, sizeof err) == 0);  /* folds to l */
    CHECK(app.cur_col == 1);
    CHECK(replay_apply_line(&app, "w\n", NULL, err, sizeof err) == 0);
    CHECK(grid_get(app.grid, 0, 1) == 0x00FF5733u);
    CHECK(replay_apply_line(&app, "I\n", NULL, err, sizeof err) == 0);  /* up, clamped */
    CHECK(app.cur_row == 0);
    CHECK(replay_apply_line(&app, "1\n", NULL, err, sizeof err) == 0);  /* slot 0 = black */
    CHECK(app.color == 0x00000000u);
    CHECK(replay_apply_line(&app, "=\n", NULL, err, sizeof err) == 0);  /* grow */
    CHECK(app.n == 9);
    CHECK(replay_apply_line(&app, "-\n", NULL, err, sizeof err) == 0);  /* shrink */
    CHECK(app.n == 8);
    CHECK(replay_apply_line(&app, "z\n", NULL, err, sizeof err) == 0);  /* undo */
    CHECK(replay_apply_line(&app, "y\n", NULL, err, sizeof err) == 0);  /* redo */
    CHECK(replay_apply_line(&app, "c\n", NULL, err, sizeof err) == 0);  /* clear */
    CHECK(grid_get(app.grid, 0, 1) == EMPTY);
    CHECK(replay_apply_line(&app, "p\n", NULL, err, sizeof err) == 0);

    /* errors: each is reported, the state is unchanged */
    CHECK(replay_apply_line(&app, "q\n", NULL, err, sizeof err) == 2);  /* quit */
    CHECK(replay_apply_line(&app, "x\n", NULL, err, sizeof err) == 1);
    CHECK(replay_apply_line(&app, "t #12345G\n", NULL, err, sizeof err) == 1);
    CHECK(app.color == 0x00000000u);
    CHECK(replay_apply_line(&app, "t #FF57331\n", NULL, err, sizeof err) == 1);
    CHECK(app.color == 0x00000000u);
    CHECK(replay_apply_line(&app, "w extra\n", NULL, err, sizeof err) == 1);
    CHECK(replay_apply_line(&app, "s\n", NULL, err, sizeof err) == 1); /* no --out */
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
    test_erase_and_move();
    test_cell_hit();
    test_clear_and_resize();
    test_undo_redo();
    test_palette();
    test_export();
    test_replay_tokens();

    printf("%d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
