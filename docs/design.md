# Design (Week 2)

## Pseudocode per action

Same logic in C (`c/src/app.c`) and asm (`asm/app.asm`).

```
do_paint():                          # key w
    if grid[cur_row][cur_col] == color: return 0   # no change, no undo record
    grid[cur_row][cur_col] = color
    return 1                         # caller redraws the cell and the cursor
```

TODO (later steps): do_erase, set_color, do_clear, do_resize, do_undo,
do_redo, palette_add, palette_pick, do_save.

## Register allocation

Globals (set once in `main`, never repurposed):

| Register | Holds |
|----------|-------|
| s0 | GRID_BASE |
| s1 | n (mirrored in `.data n`) |
| s2 | current color |
| s3 | cursor row |
| s4 | cursor col |
| s5 | cell size in units |
| s6 | canvas offset in units |
| s7 | DISPLAY_BASE |
| s8-s11 | free for local use (save/restore) |

### Function table

| Function | Args | Returns | Clobbers | Calls | Globals read / written |
|----------|------|---------|----------|-------|------------------------|
| `main` | - | never returns | all a, t (never returns) | app_init, draw_background, draw_canvas, draw_cursor, read_key, handle_key | - |
| `app_init` | - | - | a0, a1, t0-t2 | grid_clear, grid_cell_size, grid_offset | **writes s0-s7** |
| `do_paint` | - | a0 = 1 changed / 0 same | a0-a2, t0 | grid_get, grid_set | reads s0, s2, s3, s4 |
| `grid_get` | a0 = row, a1 = col | a0 = color | t0 | - | reads s0 |
| `grid_set` | a0 = row, a1 = col, a2 = color | - | t0 | - | reads s0 |
| `grid_clear` | - | - | t0-t2 | - | reads s0 |
| `grid_cell_size` | a0 = canvas, a1 = n | a0 = canvas / n | - | - | - |
| `grid_offset` | a0 = canvas, a1 = n | a0 = (canvas - n*cell) / 2 | t0, t1 | - | - |
| `luminance_q8` | a0 = color | a0 = Y (0..255) | t0-t2 | - | - |
| `cursor_color` | a0 = cell color | a0 = 0x000000 / 0xFFFFFF | t0-t2 | luminance_q8 | - |
| `fill_rect` | a0 = x, a1 = y, a2 = w, a3 = h, a4 = color | - | t0-t4 | - | reads s7 |
| `draw_background` | - | - | a0-a4, t0-t4 | fill_rect | reads s7 |
| `draw_cell` | a0 = row, a1 = col | - | a0-a4, t0-t4 | grid_get, fill_rect | reads s0, s5, s6, s7 |
| `draw_canvas` | - | - | a0-a4, t0-t4 | draw_cell | reads s1 (+ draw_cell's) |
| `draw_cursor` | - | - | a0-a4, t0-t4 | grid_get, cursor_color, fill_rect | reads s0, s3-s7 |
| `read_key` | - | a0 = byte, or -1 if none | t0 | - | - |
| `handle_key` | a0 = key byte | a0 = 1 quit / 0 | a0-a4, t0-t4 | do_paint, draw_cell, draw_cursor | reads s3, s4 |

Every non-leaf function saves `ra`; any function that uses s8-s11 saves and
restores them (16-byte stack frames).

## Memory map

Measured with RARS 1.6 in project mode (asm build step 1):

| Address | Label | Size | Contents |
|---------|-------|------|----------|
| 0x10010000 | `grid` | 1024 B | 256 words, stride 16, EMPTY = 0xFFFFFFFF |
| 0x10010400 | `n` | 4 B | mirror of s1 |
| 0x10040000 | (display) | 64 KB | Bitmap Display, 128 x 128 words |
| 0xFFFF0000 / 0xFFFF0004 | (MMIO) | - | keyboard control / data |

`.data` total so far: 1028 bytes (limit 0x30000 = 196,608). TODO: add
palette, history and file buffer as they land.

## Undo record layout

TODO: section 10.
