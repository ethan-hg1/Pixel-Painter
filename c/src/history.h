/* history.h - undo/redo ring buffer (BUILD_INSTRUCTIONS.txt section 10).
 * Records are pushed by the do_* actions in app.c; undo and redo use the
 * index helpers below and apply the change themselves. No raylib. */
#ifndef PP_HISTORY_H
#define PP_HISTORY_H

#include <stdint.h>

#include "config.h"
#include "grid.h"

/* Record types (section 10, +0 type). */
#define HIST_PAINT   1   /* also erase: new_color = EMPTY */
#define HIST_CLEAR   2   /* full grid snapshot in snap[] */
#define HIST_RESIZE  3   /* full grid snapshot in snap[] */

typedef struct {
    int      type;
    int      row, col;       /* PAINT */
    uint32_t old_color;      /* PAINT */
    uint32_t new_color;      /* PAINT */
    int      old_n;          /* CLEAR, RESIZE */
    int      new_n;          /* RESIZE */
} HistRecord;

typedef struct {
    HistRecord rec[HISTORY_DEPTH];
    uint32_t   snap[HISTORY_DEPTH][GRID_CELLS];  /* used by CLEAR, RESIZE */
    int        start;        /* ring index of the oldest record */
    int        len;          /* valid records */
    int        pos;          /* records currently applied (0..len) */
} History;

/* Empties the history: no records, nothing to undo or redo. */
void history_clear(History *h);

/* Ring index of logical record i: (start + i) & (HISTORY_DEPTH - 1). */
int  history_slot(const History *h, int i);

/* Adds r as the newest record and drops any redo tail. If the ring is full
 * the oldest record is forgotten. For CLEAR and RESIZE, `grid` is copied
 * into the record's snapshot, so call this BEFORE changing the grid. */
void history_push(History *h, const HistRecord *r, const uint32_t *grid);

/* Undo step: moves back one record and returns its ring index, or -1 when
 * there is nothing to undo. The caller reverts the change. */
int  history_undo_index(History *h);

/* Redo step: returns the ring index of the next record and moves forward,
 * or -1 when there is nothing to redo. The caller re-applies the change. */
int  history_redo_index(History *h);

#endif /* PP_HISTORY_H */
