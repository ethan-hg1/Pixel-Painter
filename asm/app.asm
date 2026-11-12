# app.asm - app state setup + one function per action.
# Translation of c/src/app.c (same function names). The C AppState fields
# live in the global registers s0-s7 (section 8) and in .data.

.include "constants.asm"

        .text
        .globl  app_init, do_paint

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

# do_paint() -> a0 = 1 if the cell changed, 0 if it already had the color
# Paints the cursor cell (s3, s4) with the current color s2.
# Uses: a0-a2.  Calls: grid_get, grid_set.  Saves: ra.
# Reads globals: s0, s2, s3, s4.
do_paint:
        addi    sp, sp, -16
        sw      ra, 12(sp)

        mv      a0, s3
        mv      a1, s4
        jal     grid_get
        beq     a0, s2, do_paint_same
        mv      a0, s3
        mv      a1, s4
        mv      a2, s2
        jal     grid_set
        li      a0, 1
        j       do_paint_done
do_paint_same:
        li      a0, 0
do_paint_done:
        lw      ra, 12(sp)
        addi    sp, sp, 16
        ret
