# Design (Week 2)

## Pseudocode per action

Same logic in C (`c/src/app.c`) and asm (`asm/app.asm`).

```
do_paint():                          # key w
    if grid[cur_row][cur_col] == color: return 0   # no change, no undo record
    grid[cur_row][cur_col] = color
    return 1                         # caller redraws the cell and the cursor
```

```
do_erase():                          # key e
    if grid[cur_row][cur_col] == EMPTY: return 0
    grid[cur_row][cur_col] = EMPTY
    return 1                         # same shared code path as do_paint

move_cursor(drow, dcol):             # keys i (up) j (left) k (down) l (right)
    cur_row = clamp(cur_row + drow, 0, n-1)   # no wrap, no out-of-bounds
    cur_col = clamp(cur_col + dcol, 0, n-1)
    return 1 if the cursor moved else 0
    # handle_key redraws the old cell first (erases the old border), then
    # moves, then draws the cursor in its new cell
```

```
grid_cell_at(px, py, ox, oy, canvas, n):   # mouse hit test (C only in use)
    row = axis_index(py, oy, canvas, n) ; col = axis_index(px, ox, canvas, n)
    return 0 if either is -1, else 1 with (row, col)
axis_index(p, origin, canvas, n):
    cell = canvas / n ; off = (canvas - n*cell) / 2
    rel = p - origin - off
    return -1 if rel < 0 or rel >= n*cell else rel / cell
    # asm: row and col come back in a1/a2 (the C pointers become returns)
```

```
entry_start():                       # key t
    entry_mode = 1 ; entry_len = 0 ; entry_buf = "" ; status = ""
entry_add_char(c):                   # any typed key while entry_mode
    if 0x20 <= c <= 0x7E and entry_len < ENTRY_MAX_LEN (7):
        entry_buf[entry_len++] = c ; entry_buf[entry_len] = 0
entry_backspace():                   # Backspace (asm: 0x08 or 0x7F)
    if entry_len > 0: entry_buf[--entry_len] = 0
entry_cancel():                      # Esc (asm: 0x1B)
    entry_mode = 0                   # color unchanged
entry_confirm():                     # Enter (asm: 0x0A or 0x0D)
    entry_mode = 0
    if parse_hex(entry_buf) ok: set_color(value)
    else: status = "Invalid color"   # asm: print to the console

parse_hex(s):
    if s[0] == '#': skip it
    value = 0 ; digits = 0
    for each remaining char ch:
        d = hex_digit_value(ch)      # 0-9, a-f, A-F -> 0..15, else -1
        if d < 0 or digits == 6: invalid
        value = (value << 4) | d ; digits += 1
    valid only if digits == 6
```

palette_insert(c):                   # p, and the first paint with a color
    if c == EMPTY or c already in palette: return 0      # no duplicates
    palette[palette_next] = c ; palette_next = (palette_next + 1) & 7
    return 1                         # FIFO: the 9th color overwrites the oldest
palette_pick(slot):                  # keys 1..8
    if slot out of 0..7 or palette[slot] == EMPTY: return 0
    if palette[slot] == s2: return 0
    set_color(palette[slot]) ; return 1      # not part of undo

TODO (later steps): do_save.

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
| `app_init` | - | - | a0, a1, t0-t2 | grid_clear, palette_init, history_clear, grid_cell_size, grid_offset | **writes s0-s7** |
| `apply_paint` | a0 = row, a1 = col, a2 = color | a0 = 1 changed / 0 same | a0-a2 (saves s8-s10) | grid_get, grid_set | reads s0 |
| `set_cursor_cell` | a0 = color | a0 = 1 changed / 0 same | a0-a2, t0 (saves s8) | grid_get, history_push, apply_paint | reads s0, s1, s3, s4 |
| `do_paint` | - | a0 = 1 changed / 0 same | a0-a2, t0 (saves ra, s8) | set_cursor_cell, palette_insert | reads s2, s3, s4 (+ set_cursor_cell's) |
| `do_erase` | - | a0 = 1 changed / 0 same | a0-a2, t0 | set_cursor_cell (tail jump) | reads s3, s4 (+ set_cursor_cell's) |
| `canvas_is_empty` | - | a0 = 1 if the n x n grid is all EMPTY | a0-a1, t0 (saves s8, s9) | grid_get | reads s0, s1 |
| `apply_clear` | - | - | a0-a2 (saves s8, s9) | grid_set | reads s1 |
| `do_clear` | - | a0 = 1 changed / 0 | a0-a2, t0 (saves ra) | canvas_is_empty, history_push, apply_clear | reads s0, s1 |
| `set_size` | a0 = n | - | a0-a1, t0 (saves ra) | grid_cell_size, grid_offset, move_cursor | **writes s1 (and n), s5, s6**, s3/s4 (via move_cursor) |
| `apply_resize` | a0 = n | - | a0-a2, t0 (saves s8-s11) | grid_set, set_size | reads s0, s1; writes as set_size |
| `do_resize` | a0 = delta | a0 = 1 changed / 0 | a0-a2, t0 (saves s8) | history_push, apply_resize | reads s1 |
| `do_undo` | - | a0 = 1 undone / 0 | a0-a5, t0-t5 (saves s8, s9) | history_undo_index, apply_paint, set_size | reads s0; writes grid, and n/s5/s6/s3/s4 for RESIZE |
| `do_redo` | - | a0 = 1 redone / 0 | a0-a2, t0, t1 (saves s9) | history_redo_index, apply_paint, apply_clear, apply_resize | writes grid, and n/s5/s6/s3/s4 for RESIZE |
| `history_clear` | - | - | t0 | - | writes hist_start, hist_len, hist_pos |
| `palette_init` | - | - | t0-t2 | - | writes palette, palette_next |
| `palette_find` | a0 = color | a0 = slot / -1 | a0, t0-t2 | - | reads palette |
| `palette_insert` | a0 = color | a0 = 1 saved / 0 | a0-a1, t0-t3 (saves ra, s8) | palette_find | writes palette, palette_next (when saved) |
| `palette_add` | - | a0 = 1 saved / 0 | a0 (tail jump) | palette_insert | reads s2 |
| `palette_pick` | a0 = slot | a0 = 1 changed / 0 | a0-a1, t0-t3 (saves ra) | set_color | reads palette, s2; writes s2 (through set_color) |
| `palette_slot_at` | a0 = px, a1 = py, a2 = x0, a3 = y0, a4 = size, a5 = pitch_x, a6 = pitch_y | a0 = 1 hit (slot in a1) / 0 (a1 unchanged) | a0, a1, t0-t6 | - | - (hit test; not called by the RARS program) |
| `history_slot` | a0 = i | a0 = (hist_start + i) & 63 | a0, t0 | - | reads hist_start |
| `history_push` | a0 = record address | - | a0-a2, t0-t6 (saves s8-s10) | history_slot | reads s0 (snapshot copy); writes hist_* and the record/snapshot |
| `history_undo_index` | - | a0 = slot / -1 | a0, t0, t1 | history_slot (tail jump) | writes hist_pos |
| `history_redo_index` | - | a0 = slot / -1 | a0-a1, t0-t3 | history_slot (tail jump) | writes hist_pos |
| `redraw_all` (input.asm) | - | - | a0-a4, t0-t4 | draw_background, draw_canvas, draw_cursor, draw_preview, draw_palette | reads s1-s7 |
| `move_cursor` | a0 = drow, a1 = dcol | a0 = 1 moved / 0 | a0, a1, t0-t2 | - | reads s1; **writes s3, s4** |
| `axis_index` | a0 = p, a1 = origin, a2 = canvas, a3 = n | a0 = index / -1 | t0-t3 | - | - |
| `grid_cell_at` | a0 = px, a1 = py, a2 = ox, a3 = oy, a4 = canvas, a5 = n | a0 = 1 inside (a1 = row, a2 = col) / 0 | a0-a3 (saves s8-s11) | axis_index | - |
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
| `draw_preview` | - | - | a0-a4, t0-t4 | fill_rect | reads s2, s7 |
| `draw_palette` | - | - | a0-a4, t0-t4 (saves ra, s8-s11) | fill_rect | reads s2, s7, palette |
| `set_color` | a0 = color | - | t0 | - | **writes s2** |
| `entry_start` | - | - | t0, t1 | - | entry_mode, entry_len, entry_buf |
| `entry_add_char` | a0 = char | a0 = 1 added / 0 ignored | t0-t2 | - | entry_len, entry_buf |
| `entry_backspace` | - | - | t0-t2 | - | entry_len, entry_buf |
| `entry_cancel` | - | - | t0 | - | entry_mode |
| `entry_confirm` | - | a0 = 1 valid / 0 | a0, a1, t0-t2 | parse_hex, set_color | entry_mode; **writes s2** if valid |
| `hex_digit_value` | a0 = char | a0 = 0..15 or -1 | t0 | - | - |
| `parse_hex` | a0 = string address | a0 = 1 ok / 0, a1 = color | a0, a1, t0 | hex_digit_value | - |
| `read_key` | - | a0 = byte, or -1 if none | t0 | - | - |
| `handle_key` | a0 = key byte | a0 = 1 quit / 0 | a0-a7, t0-t4 (saves s8, s9) | handle_entry_key, do_paint, do_erase, do_clear, do_resize, do_undo, do_redo, redraw_all, move_cursor, draw_cell, draw_cursor, entry_start, palette_add, palette_pick, draw_palette, draw_preview, print_current_color | reads s3, s4, entry_mode |
| `handle_entry_key` | a0 = key byte | - | a0-a7, t0-t4 | entry_confirm, entry_cancel, entry_backspace, entry_add_char, draw_preview, print_current_color | - |
| `print_current_color` | - | - | a0, a7, t0 | print_color | reads s2 |
| `print_color` | a0 = color | - | a0, a7, t0 | - | - |

Every non-leaf function saves `ra`; any function that uses s8-s11 saves and
restores them (16-byte stack frames).

## Memory map

All global `.data` is declared in `asm/main.asm` in this order (RARS puts
the main file's `.data` first; other files' `.data` follows in alphabetical
file order). Measured with RARS 1.6 in project mode:

| Address | Label | Size | Contents |
|---------|-------|------|----------|
| 0x10010000 | `grid` | 1024 B | 256 words, stride 16, EMPTY = 0xFFFFFFFF |
| 0x10010400 | `n` | 4 B | mirror of s1 |
| 0x10010404 | `entry_mode` | 4 B | 1 while typing a hex code |
| 0x10010408 | `entry_len` | 4 B | chars in `entry_buf` |
| 0x1001040C | `palette_next` | 4 B | slot the next save overwrites (FIFO) |
| 0x10010410 | `palette` | 32 B | 8 words, EMPTY = unused slot |
| 0x10010430 | `hist_records` | 2 KB | 64 records x 32 bytes (section 10) |
| 0x10010C30 | `hist_snaps` | 64 KB | 64 snapshots x 1 KB (256 words each) |
| 0x10020C30 | `hist_start` | 4 B | ring index of the oldest record |
| 0x10020C34 | `hist_len` | 4 B | valid records |
| 0x10020C38 | `hist_pos` | 4 B | records currently applied |
| 0x10020C3C | `entry_buf` | 8 B | typed chars + 0 byte |
| 0x10040000 | (display) | 64 KB | Bitmap Display, 128 x 128 words |
| 0xFFFF0000 / 0xFFFF0004 | (MMIO) | - | keyboard control / data |

`.data` total so far: 0x20C44 - 0x10000 = 68,676 bytes (grid to the end of
entry_buf, plus the palette and history), plus the console message strings (limit 0x30000 = 196,608).
Addresses after `palette_next` moved by 36 bytes at step 6 (from the step 5 values).
TODO: add file buffer (`file_buf`, 52 KB) as it lands (step 7).

## Undo record layout

Section 10 layout, the same in C (`history.h`) and asm (`history.asm`).
Each record is 32 bytes; each one has a 1 KB snapshot slot at the same index.

```
+0 type  (1 PAINT, 2 CLEAR, 3 RESIZE)   +4 row   +8 col
+12 old_color   +16 new_color   +20 old_n   +24 new_n   +28 unused
hist_start, hist_len, hist_pos       slot(i) = (hist_start + i) & 63
```

Each do_* action records BEFORE it changes the grid, and it records nothing
when it is a no-op (see `docs/decisions.md` #16):

```
do_paint / do_erase(): if cell already holds the color: return 0
    push(PAINT row, col, old, new); apply_paint(row, col, new); return 1

do_clear():            if canvas (n x n) is all EMPTY: return 0
    push(CLEAR, old_n = new_n = n)   # push copies the grid into the snapshot
    apply_clear(); return 1

do_resize(delta):      if next == n or next outside [MIN_N, MAX_N]: return 0
    push(RESIZE, old_n = n, new_n = next)   # snapshot before the change
    apply_resize(next); return 1

push(r, grid):         # drops the redo tail; a full ring drops the oldest
    len = pos
    if len == 64: start = (start+1) & 63; len--; pos--
    s = slot(pos); rec[s] = r
    if r is CLEAR or RESIZE: snap[s] = grid       # 256 words
    pos++; len = pos

do_undo():             if pos == 0: return 0
    pos--; s = slot(pos)
    PAINT:  grid[row][col] = old_color
    CLEAR, RESIZE: grid = snap[s]                 # whole 256-word grid
    RESIZE: n = old_n; move_cursor(0, 0)          # clamp
    return 1

do_redo():             if pos == len: return 0
    s = slot(pos); pos++
    PAINT:  grid[row][col] = new_color            # apply_* only, no push
    CLEAR:  apply_clear(); RESIZE: apply_resize(new_n)
    return 1
```

Key bindings: C `ctrl+z` undo, `ctrl+shift+z` redo (shift checked first).
asm `z` undo, `y` redo (decision in section 2.9 of the spec).
