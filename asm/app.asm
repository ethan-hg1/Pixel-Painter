# app.asm - app state setup + one function per action.
# Translation of c/src/app.c (same function names). The C AppState fields
# live in the global registers s0-s7 (section 8) and in .data.

.include "constants.asm"

# State (entry_mode, entry_len, entry_buf, n) is declared in main.asm.

        .text
        .globl  app_init, do_paint, do_erase, do_clear, do_resize, move_cursor
        .globl  set_color
        .globl  entry_start, entry_add_char, entry_backspace
        .globl  entry_cancel, entry_confirm

# app_init()
# Sets every global: empty grid, n = START_N, cursor (0, 0), color black,
# cell size / offset for n, display base.
# WRITES GLOBALS: s0-s7 (this is its job).
# Uses: a0, a1, t0 (+ callees' t0-t2).  Calls: grid_clear, grid_cell_size, grid_offset.
# Saves: ra.
app_init:
        addi    sp, sp, -16
        sw      ra, 12(sp)

        la      s0, grid                # s0 = GRID_BASE
        li      s7, DISPLAY_BASE        # s7 = DISPLAY_BASE
        jal     grid_clear
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

# set_cursor_cell(a0 = color) -> a0 = 1 if the cell changed, 0 if it already
# had that color. Shared by do_paint and do_erase (like the C version).
# Uses: a0-a2, t0 (+ callees').  Calls: grid_get, grid_set.  Saves: ra, s8.
# Reads globals: s0, s3, s4.
set_cursor_cell:
        addi    sp, sp, -16
        sw      ra, 12(sp)
        sw      s8, 8(sp)
        mv      s8, a0                  # s8 = wanted color

        mv      a0, s3
        mv      a1, s4
        jal     grid_get
        beq     a0, s8, set_cursor_same
        mv      a0, s3
        mv      a1, s4
        mv      a2, s8
        jal     grid_set
        li      a0, 1
        j       set_cursor_done
set_cursor_same:
        li      a0, 0
set_cursor_done:
        lw      s8, 8(sp)
        lw      ra, 12(sp)
        addi    sp, sp, 16
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

# do_clear() -> a0 = 1 if any cell of the n x n grid changed, else 0 (c key)
# Sets every cell of the current n x n grid to EMPTY (section 2.7).
# Uses: a0-a2, t0 (+ callees').  Calls: grid_get, grid_set.
# Saves: ra, s8-s10.  Reads globals: s0, s1.
do_clear:
        addi    sp, sp, -16
        sw      ra, 12(sp)
        sw      s8, 8(sp)
        sw      s9, 4(sp)
        sw      s10, 0(sp)
        li      s10, 0                  # s10 = changed flag
        li      s8, 0                   # s8 = row
do_clear_row:
        bge     s8, s1, do_clear_done
        li      s9, 0                   # s9 = col
do_clear_col:
        bge     s9, s1, do_clear_next_row
        mv      a0, s8
        mv      a1, s9
        jal     grid_get
        li      t0, EMPTY
        beq     a0, t0, do_clear_skip
        mv      a0, s8
        mv      a1, s9
        li      a2, EMPTY
        jal     grid_set
        li      s10, 1
do_clear_skip:
        addi    s9, s9, 1
        j       do_clear_col
do_clear_next_row:
        addi    s8, s8, 1
        j       do_clear_row
do_clear_done:
        mv      a0, s10
        lw      s10, 0(sp)
        lw      s9, 4(sp)
        lw      s8, 8(sp)
        lw      ra, 12(sp)
        addi    sp, sp, 16
        ret

# do_resize(a0 = delta) -> a0 = 1 if n changed, 0 if it did nothing
# Changes n by delta (+RESIZE_STEP grow, -RESIZE_STEP shrink). Returns 0 for
# a zero delta or when the new size leaves [MIN_N, MAX_N]. Shrinking sets the
# cells outside the new size to EMPTY. Then it recomputes the cell size and
# offset (s5, s6), and clamps the cursor into the new grid.
# WRITES GLOBALS: s1 and n (.data), s5, s6, and s3/s4 (through move_cursor).
# Uses: a0-a3, t0.  Calls: grid_set, grid_cell_size, grid_offset, move_cursor.
# Saves: ra, s8-s11.  Reads globals: s0, s1.
do_resize:
        addi    sp, sp, -32
        sw      ra, 28(sp)
        sw      s8, 24(sp)
        sw      s9, 20(sp)
        sw      s10, 16(sp)
        sw      s11, 12(sp)

        add     s8, s1, a0              # s8 = next n
        beq     s8, s1, do_resize_fail  # zero delta
        li      t0, MIN_N
        blt     s8, t0, do_resize_fail
        li      t0, MAX_N
        bgt     s8, t0, do_resize_fail
        bge     s8, s1, do_resize_set   # growing: nothing to clear

        # shrinking: clear cells with row >= next or col >= next
        mv      s11, s1                 # s11 = old n
        li      s9, 0                   # s9 = row
do_resize_row:
        bge     s9, s11, do_resize_set
        li      s10, 0                  # s10 = col
do_resize_col:
        bge     s10, s11, do_resize_next_row
        bge     s9, s8, do_resize_clear
        bge     s10, s8, do_resize_clear
        j       do_resize_col_next
do_resize_clear:
        mv      a0, s9
        mv      a1, s10
        li      a2, EMPTY
        jal     grid_set
do_resize_col_next:
        addi    s10, s10, 1
        j       do_resize_col
do_resize_next_row:
        addi    s9, s9, 1
        j       do_resize_row

do_resize_set:
        mv      s1, s8
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
        li      a0, 1
        j       do_resize_ret
do_resize_fail:
        li      a0, 0
do_resize_ret:
        lw      s11, 12(sp)
        lw      s10, 16(sp)
        lw      s9, 20(sp)
        lw      s8, 24(sp)
        lw      ra, 28(sp)
        addi    sp, sp, 32
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
