# app.asm - app state setup + one function per action.
# Translation of c/src/app.c (same function names). The C AppState fields
# live in the global registers s0-s7 (section 8) and in .data.
#
# Every do_* action records itself in the history (history.asm) BEFORE it
# changes anything, and records nothing when it is a no-op. The apply_*
# helpers change the state without touching the history; do_undo and
# do_redo use them too (same split as the C version).

.include "constants.asm"

# State (grid, n, entry_*, hist_*) is declared in main.asm.

        .text
        .globl  app_init, do_paint, do_erase, do_clear, do_resize
        .globl  do_undo, do_redo, move_cursor
        .globl  set_color
        .globl  entry_start, entry_add_char, entry_backspace
        .globl  entry_cancel, entry_confirm

# app_init()
# Sets every global: empty grid, n = START_N, cursor (0, 0), color black,
# cell size / offset for n, empty history, display base.
# WRITES GLOBALS: s0-s7 (this is its job).
# Uses: a0, a1, t0 (+ callees' t0-t2).  Calls: grid_clear, history_clear,
# grid_cell_size, grid_offset.  Saves: ra.
app_init:
        addi    sp, sp, -16
        sw      ra, 12(sp)

        la      s0, grid                # s0 = GRID_BASE
        li      s7, DISPLAY_BASE        # s7 = DISPLAY_BASE
        jal     grid_clear
        jal     history_clear
        li      s1, START_N             # s1 = n
        la      t0, n
        sw      s1, 0(t0)               # mirror n in .data
        li      s2, 0x00000000          # s2 = color (black)
        la      t0, entry_mode
        sw      zero, 0(t0)             # not in entry mode
        la      t0, entry_len
        sw      zero, 0(t0)
        la      t0, entry_buf
        sb      zero, 0(t0)
        li      s3, 0                   # s3 = cursor row
        li      s4, 0                   # s4 = cursor col

        li      a0, CANVAS_UNITS
        mv      a1, s1
        jal     grid_cell_size
        mv      s5, a0                  # s5 = cell size
        li      a0, CANVAS_UNITS
        mv      a1, s1
        jal     grid_offset
        mv      s6, a0                  # s6 = canvas offset

        lw      ra, 12(sp)
        addi    sp, sp, 16
        ret

# ---- apply_*: change the state, never touch the history -------------------

# apply_paint(a0 = row, a1 = col, a2 = color) -> a0 = 1 if the cell changed
# Writes color to one cell unless it already holds it.
# Uses: a0-a2.  Calls: grid_get, grid_set.  Saves: ra, s8-s10.
apply_paint:
        addi    sp, sp, -32
        sw      ra, 28(sp)
        sw      s8, 24(sp)
        sw      s9, 20(sp)
        sw      s10, 16(sp)
        mv      s8, a0                  # s8 = row
        mv      s9, a1                  # s9 = col
        mv      s10, a2                 # s10 = color
        mv      a0, s8
        mv      a1, s9
        jal     grid_get
        beq     a0, s10, apply_paint_same
        mv      a0, s8
        mv      a1, s9
        mv      a2, s10
        jal     grid_set
        li      a0, 1
        j       apply_paint_ret
apply_paint_same:
        li      a0, 0
apply_paint_ret:
        lw      s10, 16(sp)
        lw      s9, 20(sp)
        lw      s8, 24(sp)
        lw      ra, 28(sp)
        addi    sp, sp, 32
        ret

# canvas_is_empty() -> a0 = 1 if every cell of the n x n grid is EMPTY
# Uses: a0, a1, t0.  Calls: grid_get.  Saves: ra, s8, s9.  Reads: s1, s0.
canvas_is_empty:
        addi    sp, sp, -16
        sw      ra, 12(sp)
        sw      s8, 8(sp)
        sw      s9, 4(sp)
        li      s8, 0                   # s8 = row
ce_row:
        bge     s8, s1, ce_yes
        li      s9, 0                   # s9 = col
ce_col:
        bge     s9, s1, ce_next_row
        mv      a0, s8
        mv      a1, s9
        jal     grid_get
        li      t0, EMPTY
        bne     a0, t0, ce_no
        addi    s9, s9, 1
        j       ce_col
ce_next_row:
        addi    s8, s8, 1
        j       ce_row
ce_yes:
        li      a0, 1
        j       ce_ret
ce_no:
        li      a0, 0
ce_ret:
        lw      s9, 4(sp)
        lw      s8, 8(sp)
        lw      ra, 12(sp)
        addi    sp, sp, 16
        ret

# apply_clear()
# Sets every cell of the n x n grid to EMPTY.
# Uses: a0-a2.  Calls: grid_set.  Saves: ra, s8, s9.  Reads globals: s1.
apply_clear:
        addi    sp, sp, -16
        sw      ra, 12(sp)
        sw      s8, 8(sp)
        sw      s9, 4(sp)
        li      s8, 0                   # s8 = row
apply_clear_row:
        bge     s8, s1, apply_clear_done
        li      s9, 0                   # s9 = col
apply_clear_col:
        bge     s9, s1, apply_clear_next_row
        mv      a0, s8
        mv      a1, s9
        li      a2, EMPTY
        jal     grid_set
        addi    s9, s9, 1
        j       apply_clear_col
apply_clear_next_row:
        addi    s8, s8, 1
        j       apply_clear_row
apply_clear_done:
        lw      s9, 4(sp)
        lw      s8, 8(sp)
        lw      ra, 12(sp)
        addi    sp, sp, 16
        ret

# set_size(a0 = next)
# Sets n to a0 (mirrored in .data), recomputes the cell size and offset,
# and clamps the cursor. Does not clear cells (the caller does that).
# WRITES GLOBALS: s1 and n (.data), s5, s6, and s3/s4 (through move_cursor).
# Uses: a0-a1, t0.  Calls: grid_cell_size, grid_offset, move_cursor.
# Saves: ra.
set_size:
        addi    sp, sp, -16
        sw      ra, 12(sp)
        mv      s1, a0                  # s1 = n
        la      t0, n
        sw      s1, 0(t0)               # mirror n in .data
        li      a0, CANVAS_UNITS
        mv      a1, s1
        jal     grid_cell_size
        mv      s5, a0                  # s5 = cell size
        li      a0, CANVAS_UNITS
        mv      a1, s1
        jal     grid_offset
        mv      s6, a0                  # s6 = canvas offset
        li      a0, 0                   # clamp the cursor (delta 0 = no move)
        li      a1, 0
        jal     move_cursor
        lw      ra, 12(sp)
        addi    sp, sp, 16
        ret

# apply_resize(a0 = next)
# Changes n to a0. Shrinking first sets the cells outside the new size to
# EMPTY. Then set_size.
# WRITES GLOBALS: through set_size (s1, n, s5, s6, s3, s4).
# Uses: a0-a2, t0.  Calls: grid_set, set_size.  Saves: ra, s8-s11.
apply_resize:
        addi    sp, sp, -32
        sw      ra, 28(sp)
        sw      s8, 24(sp)
        sw      s9, 20(sp)
        sw      s10, 16(sp)
        sw      s11, 12(sp)
        mv      s8, a0                  # s8 = next n
        bge     s8, s1, apply_resize_set    # growing: nothing to clear

        # shrinking: clear cells with row >= next or col >= next
        mv      s11, s1                 # s11 = old n
        li      s9, 0                   # s9 = row
apply_resize_row:
        bge     s9, s11, apply_resize_set
        li      s10, 0                  # s10 = col
apply_resize_col:
        bge     s10, s11, apply_resize_next_row
        bge     s9, s8, apply_resize_clear
        bge     s10, s8, apply_resize_clear
        j       apply_resize_col_next
apply_resize_clear:
        mv      a0, s9
        mv      a1, s10
        li      a2, EMPTY
        jal     grid_set
apply_resize_col_next:
        addi    s10, s10, 1
        j       apply_resize_col
apply_resize_next_row:
        addi    s9, s9, 1
        j       apply_resize_row

apply_resize_set:
        mv      a0, s8
        jal     set_size
        lw      s11, 12(sp)
        lw      s10, 16(sp)
        lw      s9, 20(sp)
        lw      s8, 24(sp)
        lw      ra, 28(sp)
        addi    sp, sp, 32
        ret

# ---- do_*: record the action, then apply it ------------------------------

# set_cursor_cell(a0 = color) -> a0 = 1 if the cell changed, 0 if it already
# had that color. Shared by do_paint and do_erase (like the C version).
# Records a PAINT (old, new) before the change.
# Uses: a0-a2, t0.  Calls: grid_get, history_push, apply_paint.
# Saves: ra, s8.  Reads globals: s3, s4, s1.
set_cursor_cell:
        addi    sp, sp, -48
        sw      ra, 44(sp)
        sw      s8, 40(sp)
        mv      s8, a0                  # s8 = wanted color

        mv      a0, s3
        mv      a1, s4
        jal     grid_get                # a0 = old color
        beq     a0, s8, set_cursor_same

        li      t0, 1                   # HIST_PAINT
        sw      t0, 0(sp)
        sw      s3, 4(sp)               # row
        sw      s4, 8(sp)               # col
        sw      a0, 12(sp)              # old_color
        sw      s8, 16(sp)              # new_color
        sw      zero, 20(sp)
        sw      zero, 24(sp)
        sw      zero, 28(sp)
        mv      a0, sp
        jal     history_push
        mv      a0, s3
        mv      a1, s4
        mv      a2, s8
        jal     apply_paint             # a0 = 1
        j       set_cursor_done
set_cursor_same:
        li      a0, 0
set_cursor_done:
        lw      s8, 40(sp)
        lw      ra, 44(sp)
        addi    sp, sp, 48
        ret

# do_paint() -> a0 = 1 if the cell changed, 0 if it already had the color
# Paints the cursor cell (s3, s4) with the current color s2.
# Uses: a0-a2.  Calls: set_cursor_cell (tail jump).  Saves: nothing.
# Reads globals: s2 (and set_cursor_cell's).
do_paint:
        mv      a0, s2
        j       set_cursor_cell

# do_erase() -> a0 = 1 if the cell changed, 0 if it was already EMPTY
# Sets the cursor cell (s3, s4) to EMPTY (key e).
# Uses: a0.  Calls: set_cursor_cell (tail jump).  Saves: nothing.
do_erase:
        li      a0, EMPTY
        j       set_cursor_cell

# do_clear() -> a0 = 1 if the canvas changed, 0 if it was already empty (c)
# Records a CLEAR (with a snapshot of the grid) and sets every cell of the
# n x n grid to EMPTY. An empty canvas records nothing.
# Uses: a0-a2, t0.  Calls: canvas_is_empty, history_push, apply_clear.
# Saves: ra.  Reads globals: s0, s1.
do_clear:
        addi    sp, sp, -48
        sw      ra, 44(sp)
        jal     canvas_is_empty
        bnez    a0, do_clear_none
        li      t0, 2                   # HIST_CLEAR
        sw      t0, 0(sp)
        sw      zero, 4(sp)
        sw      zero, 8(sp)
        sw      zero, 12(sp)
        sw      zero, 16(sp)
        sw      s1, 20(sp)              # old_n
        sw      s1, 24(sp)              # new_n
        sw      zero, 28(sp)
        mv      a0, sp
        jal     history_push
        jal     apply_clear
        li      a0, 1
        j       do_clear_ret
do_clear_none:
        li      a0, 0
do_clear_ret:
        lw      ra, 44(sp)
        addi    sp, sp, 48
        ret

# do_resize(a0 = delta) -> a0 = 1 if n changed, 0 if it did nothing
# Changes n by delta (+RESIZE_STEP grow, -RESIZE_STEP shrink). Returns 0 for
# a zero delta or when the new size leaves [MIN_N, MAX_N]. Otherwise records
# a RESIZE (old_n, new_n, with a snapshot) and applies it.
# WRITES GLOBALS: through apply_resize.
# Uses: a0-a2, t0, s8.  Calls: history_push, apply_resize.  Saves: ra, s8.
# Reads globals: s1.
do_resize:
        addi    sp, sp, -48
        sw      ra, 44(sp)
        sw      s8, 40(sp)

        add     s8, s1, a0              # s8 = next n
        beq     s8, s1, do_resize_fail  # zero delta
        li      t0, MIN_N
        blt     s8, t0, do_resize_fail
        li      t0, MAX_N
        bgt     s8, t0, do_resize_fail

        li      t0, 3                   # HIST_RESIZE
        sw      t0, 0(sp)
        sw      zero, 4(sp)
        sw      zero, 8(sp)
        sw      zero, 12(sp)
        sw      zero, 16(sp)
        sw      s1, 20(sp)              # old_n
        sw      s8, 24(sp)              # new_n
        sw      zero, 28(sp)
        mv      a0, sp
        jal     history_push
        mv      a0, s8
        jal     apply_resize
        li      a0, 1
        j       do_resize_ret
do_resize_fail:
        li      a0, 0
do_resize_ret:
        lw      s8, 40(sp)
        lw      ra, 44(sp)
        addi    sp, sp, 48
        ret

# do_undo() -> a0 = 1 if something was undone, 0 if there was nothing
# Reverts the newest record (key z, or 0x1A). Never pushes a record.
# PAINT: sets the cell back to its old color. CLEAR: copies the snapshot
# back. RESIZE: copies the snapshot back, then set_size(old_n).
# WRITES GLOBALS: the grid, and s1/n, s5, s6, s3, s4 for a RESIZE.
# Uses: a0-a2, t0-t5, s8, s9.  Calls: history_undo_index, apply_paint,
# set_size.  Saves: ra, s8, s9.  Reads globals: s0.
do_undo:
        addi    sp, sp, -16
        sw      ra, 12(sp)
        sw      s8, 8(sp)
        sw      s9, 4(sp)

        jal     history_undo_index
        bltz    a0, do_undo_none
        mv      s8, a0                  # s8 = slot
        la      s9, hist_records
        slli    t0, s8, 5
        add     s9, s9, t0              # s9 = record address

        lw      t0, 0(s9)
        li      t1, 1                   # HIST_PAINT
        bne     t0, t1, do_undo_snap
        lw      a0, 4(s9)               # row
        lw      a1, 8(s9)               # col
        lw      a2, 12(s9)              # old_color
        jal     apply_paint
        j       do_undo_ok

do_undo_snap:
        # CLEAR and RESIZE: copy the whole snapshot back into the grid
        la      t2, hist_snaps
        slli    t3, s8, 10
        add     t2, t2, t3              # t2 = snapshot (source)
        mv      t3, s0                  # t3 = grid (destination)
        li      t4, 256                 # MAX_N * MAX_N words
do_undo_copy:
        lw      t5, 0(t2)
        sw      t5, 0(t3)
        addi    t2, t2, 4
        addi    t3, t3, 4
        addi    t4, t4, -1
        bnez    t4, do_undo_copy

        lw      t0, 0(s9)
        li      t1, 3                   # HIST_RESIZE
        bne     t0, t1, do_undo_ok
        lw      a0, 20(s9)              # old_n
        jal     set_size
do_undo_ok:
        li      a0, 1
        j       do_undo_ret
do_undo_none:
        li      a0, 0
do_undo_ret:
        lw      s9, 4(sp)
        lw      s8, 8(sp)
        lw      ra, 12(sp)
        addi    sp, sp, 16
        ret

# do_redo() -> a0 = 1 if something was redone, 0 if there was nothing
# Re-applies the next record (key y). Never pushes a record. Uses the same
# apply_* helpers as the actions do.
# WRITES GLOBALS: the grid, and s1/n, s5, s6, s3, s4 for a RESIZE.
# Uses: a0-a2, t0, t1, s9.  Calls: history_redo_index, apply_paint,
# apply_clear, apply_resize.  Saves: ra, s9.  Reads globals: none.
do_redo:
        addi    sp, sp, -16
        sw      ra, 12(sp)
        sw      s9, 4(sp)

        jal     history_redo_index
        bltz    a0, do_redo_none
        la      s9, hist_records
        slli    t0, a0, 5
        add     s9, s9, t0              # s9 = record address

        lw      t0, 0(s9)
        li      t1, 1                   # HIST_PAINT
        bne     t0, t1, do_redo_not_paint
        lw      a0, 4(s9)               # row
        lw      a1, 8(s9)               # col
        lw      a2, 16(s9)              # new_color
        jal     apply_paint
        j       do_redo_ok
do_redo_not_paint:
        li      t1, 2                   # HIST_CLEAR
        bne     t0, t1, do_redo_resize
        jal     apply_clear
        j       do_redo_ok
do_redo_resize:
        lw      a0, 24(s9)              # new_n
        jal     apply_resize
do_redo_ok:
        li      a0, 1
        j       do_redo_ret
do_redo_none:
        li      a0, 0
do_redo_ret:
        lw      s9, 4(sp)
        lw      ra, 12(sp)
        addi    sp, sp, 16
        ret

# move_cursor(a0 = drow, a1 = dcol) -> a0 = 1 if the cursor moved, else 0
# Moves (s3, s4) by the deltas, clamped to 0..n-1 (no wrap, section 2.8 rule).
# WRITES GLOBALS: s3, s4 (this is its job).
# Uses: a0, a1, t0-t2.  Calls: none (leaf).  Saves: nothing.
# Reads globals: s1.
move_cursor:
        add     t0, s3, a0              # new row, unclamped
        bgez    t0, move_row_lo_ok
        li      t0, 0
move_row_lo_ok:
        addi    t1, s1, -1              # last index
        ble     t0, t1, move_row_hi_ok
        mv      t0, t1
move_row_hi_ok:
        li      t2, 0                   # t2 = moved flag
        beq     t0, s3, move_row_set
        li      t2, 1
move_row_set:
        mv      s3, t0

        add     t0, s4, a1              # new col, unclamped
        bgez    t0, move_col_lo_ok
        li      t0, 0
move_col_lo_ok:
        addi    t1, s1, -1
        ble     t0, t1, move_col_hi_ok
        mv      t0, t1
move_col_hi_ok:
        beq     t0, s4, move_col_set
        li      t2, 1
move_col_set:
        mv      s4, t0
        mv      a0, t2
        ret

# set_color(a0 = color)
# Sets the current color to a0 & 0x00FFFFFF.
# WRITES GLOBALS: s2 (this is its job).
# Uses: t0.  Calls: none (leaf).  Saves: nothing.
set_color:
        li      t0, 0x00FFFFFF
        and     s2, a0, t0
        ret

# entry_start()
# Enters color entry mode with an empty buffer (key t).
# Uses: t0, t1.  Calls: none (leaf).  Saves: nothing.
entry_start:
        la      t0, entry_len
        sw      zero, 0(t0)
        la      t0, entry_buf
        sb      zero, 0(t0)
        la      t0, entry_mode
        li      t1, 1
        sw      t1, 0(t0)
        ret

# entry_add_char(a0 = char) -> a0 = 1 if appended, 0 if ignored
# Appends printable ASCII (0x20..0x7E) while entry_len < ENTRY_MAX_LEN.
# Uses: t0-t2.  Calls: none (leaf).  Saves: nothing.
entry_add_char:
        li      t0, 0x20
        blt     a0, t0, entry_add_no
        li      t0, 0x7E
        bgt     a0, t0, entry_add_no
        la      t1, entry_len
        lw      t2, 0(t1)               # t2 = len
        li      t0, ENTRY_MAX_LEN
        bge     t2, t0, entry_add_no
        la      t0, entry_buf
        add     t0, t0, t2
        sb      a0, 0(t0)               # buf[len] = char
        sb      zero, 1(t0)             # buf[len + 1] = 0
        addi    t2, t2, 1
        sw      t2, 0(t1)               # len += 1
        li      a0, 1
        ret
entry_add_no:
        li      a0, 0
        ret

# entry_backspace()
# Removes the last typed char, if any.
# Uses: t0-t2.  Calls: none (leaf).  Saves: nothing.
entry_backspace:
        la      t1, entry_len
        lw      t2, 0(t1)
        beqz    t2, entry_backspace_done
        addi    t2, t2, -1
        sw      t2, 0(t1)               # len -= 1
        la      t0, entry_buf
        add     t0, t0, t2
        sb      zero, 0(t0)             # buf[len] = 0
entry_backspace_done:
        ret

# entry_cancel()
# Leaves entry mode; the color is unchanged (Esc).
# Uses: t0.  Calls: none (leaf).  Saves: nothing.
entry_cancel:
        la      t0, entry_mode
        sw      zero, 0(t0)
        ret

# entry_confirm() -> a0 = 1 if the typed color was valid, else 0
# Leaves entry mode; if parse_hex accepts entry_buf, sets the color.
# WRITES GLOBALS: s2 (through set_color, only when valid).
# Uses: a0, a1, t0.  Calls: parse_hex, set_color.  Saves: ra.
entry_confirm:
        addi    sp, sp, -16
        sw      ra, 12(sp)
        la      t0, entry_mode
        sw      zero, 0(t0)
        la      a0, entry_buf
        jal     parse_hex
        beqz    a0, entry_confirm_ret   # invalid: a0 = 0, color unchanged
        mv      a0, a1
        jal     set_color
        li      a0, 1
entry_confirm_ret:
        lw      ra, 12(sp)
        addi    sp, sp, 16
        ret
