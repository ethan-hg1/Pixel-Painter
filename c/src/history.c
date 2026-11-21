/* history.c - see history.h. No raylib in this file. */
#include "history.h"

#include <string.h>

void history_clear(History *h)
{
    h->start = 0;
    h->len = 0;
    h->pos = 0;
}

int history_slot(const History *h, int i)
{
    return (h->start + i) & (HISTORY_DEPTH - 1);
}

void history_push(History *h, const HistRecord *r, const uint32_t *grid)
{
    int slot;

    h->len = h->pos;                       /* drop the redo tail */
    if (h->len == HISTORY_DEPTH) {         /* full: forget the oldest */
        h->start = (h->start + 1) & (HISTORY_DEPTH - 1);
        h->len--;
        h->pos--;
    }
    slot = history_slot(h, h->pos);
    h->rec[slot] = *r;
    if (r->type == HIST_CLEAR || r->type == HIST_RESIZE) {
        memcpy(h->snap[slot], grid, sizeof h->snap[slot]);
    }
    h->pos++;
    h->len = h->pos;
}

int history_undo_index(History *h)
{
    if (h->pos == 0) {
        return -1;
    }
    h->pos--;
    return history_slot(h, h->pos);
}

int history_redo_index(History *h)
{
    int slot;

    if (h->pos == h->len) {
        return -1;
    }
    slot = history_slot(h, h->pos);
    h->pos++;
    return slot;
}
