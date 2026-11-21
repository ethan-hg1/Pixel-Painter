/* app.c - see app.h. No raylib in this file (BUILD_INSTRUCTIONS.txt 4.2). */
#include "app.h"

#include <stdio.h>
#include <string.h>

#include "export_png.h"
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
    palette_init(app->palette, &app->palette_next);
    history_clear(&app->hist);
    app->entry_mode = 0;
    app->entry_len = 0;
    app->entry_buf[0] = '\0';
    app->status[0] = '\0';
}

/* ---- apply_*: change the state, never touch the history ------------------
 * Each returns 1 if something changed. undo and redo use them directly. */

static int apply_paint(AppState *app, int row, int col, uint32_t color)
{
    if (grid_get(app->grid, row, col) == color) {
        return 0;
    }
    grid_set(app->grid, row, col, color);
    return 1;
}

static int canvas_is_empty(const AppState *app)
{
    for (int row = 0; row < app->n; row++) {
        for (int col = 0; col < app->n; col++) {
            if (grid_get(app->grid, row, col) != EMPTY) {
                return 0;
            }
        }
    }
    return 1;
}

static void apply_clear(AppState *app)
{
    for (int row = 0; row < app->n; row++) {
        for (int col = 0; col < app->n; col++) {
            grid_set(app->grid, row, col, EMPTY);
        }
    }
}

/* Sets n to next (already checked against the limits). Shrinking empties
 * the cells outside the new size; the cursor is clamped into it. */
static void apply_resize(AppState *app, int next)
{
    if (next < app->n) {
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
}

/* ---- do_*: record the action, then apply it -------------------------------
 * A no-op returns 0 before anything is recorded (section 10). */

/* Shared by paint and erase: writes color to the cursor cell unless it
 * already holds it, recording the change first. */
static int set_cursor_cell(AppState *app, uint32_t color)
{
    HistRecord r;

    if (grid_get(app->grid, app->cur_row, app->cur_col) == color) {
        return 0;
    }
    r.type = HIST_PAINT;
    r.row = app->cur_row;
    r.col = app->cur_col;
    r.old_color = grid_get(app->grid, app->cur_row, app->cur_col);
    r.new_color = color;
    r.old_n = 0;
    r.new_n = 0;
    history_push(&app->hist, &r, app->grid);
    return apply_paint(app, app->cur_row, app->cur_col, color);
}

int do_paint(AppState *app)
{
    int changed = set_cursor_cell(app, app->color);

    /* 2.6: the first time a color paints, it is saved automatically.
     * palette_insert ignores colors that are already saved. */
    palette_insert(app->palette, &app->palette_next, app->color);
    return changed;
}

int do_erase(AppState *app)
{
    return set_cursor_cell(app, EMPTY);
}

int do_clear(AppState *app)
{
    HistRecord r;

    if (canvas_is_empty(app)) {
        return 0;
    }
    r.type = HIST_CLEAR;
    r.row = r.col = 0;
    r.old_color = r.new_color = 0;
    r.old_n = app->n;
    r.new_n = app->n;
    history_push(&app->hist, &r, app->grid);
    apply_clear(app);
    return 1;
}

int do_resize(AppState *app, int delta)
{
    HistRecord r;
    int next = app->n + delta;

    if (next == app->n || next < MIN_N || next > MAX_N) {
        return 0;
    }
    r.type = HIST_RESIZE;
    r.row = r.col = 0;
    r.old_color = r.new_color = 0;
    r.old_n = app->n;
    r.new_n = next;
    history_push(&app->hist, &r, app->grid);
    apply_resize(app, next);
    return 1;
}

int do_undo(AppState *app)
{
    int slot = history_undo_index(&app->hist);
    const HistRecord *r;

    if (slot < 0) {
        return 0;
    }
    r = &app->hist.rec[slot];
    if (r->type == HIST_PAINT) {
        grid_set(app->grid, r->row, r->col, r->old_color);
    } else {
        /* CLEAR and RESIZE: the snapshot holds the whole grid from before */
        memcpy(app->grid, app->hist.snap[slot], sizeof app->grid);
        if (r->type == HIST_RESIZE) {
            app->n = r->old_n;
            move_cursor(app, 0, 0);
        }
    }
    return 1;
}

int do_redo(AppState *app)
{
    int slot = history_redo_index(&app->hist);
    const HistRecord *r;

    if (slot < 0) {
        return 0;
    }
    r = &app->hist.rec[slot];
    if (r->type == HIST_PAINT) {
        apply_paint(app, r->row, r->col, r->new_color);
    } else if (r->type == HIST_CLEAR) {
        apply_clear(app);
    } else {
        apply_resize(app, r->new_n);
    }
    return 1;
}

int do_save(AppState *app, const char *path)
{
    char msg[STATUS_LEN];

    if (!export_png_write(app->grid, app->n, path)) {
        set_status(app, "Save failed");
        return 0;
    }
    snprintf(msg, sizeof msg, "Saved: %s", path);
    set_status(app, msg);
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

int palette_add(AppState *app)
{
    return palette_insert(app->palette, &app->palette_next, app->color);
}

int palette_pick(AppState *app, int slot)
{
    uint32_t color;

    if (slot < 0 || slot >= PALETTE_SLOTS || app->palette[slot] == EMPTY) {
        return 0;
    }
    color = app->palette[slot];
    if (color == app->color) {
        return 0;
    }
    set_color(app, color);
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
