# grid.asm - grid get/set, clear, layout and cursor color.
# Translation of c/src/grid.c (same function names, same formulas).
#
# Grid cell address (section 7.2):
#   addr = GRID_BASE + ((row << 4) + col) << 2      (GRID_STRIDE = 16)

.include "constants.asm"

# grid and n are declared in main.asm (memory map, section 9).

        .text
        .globl  grid_get, grid_set, grid_clear
        .globl  grid_cell_size, grid_offset, luminance_q8, cursor_color

# grid_get(a0 = row, a1 = col) -> a0 = color
# Reads one cell: s0 + ((row << 4) + col) << 2.
# Uses: t0.  Calls: none (leaf).  Saves: nothing.
# Reads globals: s0.
grid_get:
        slli    t0, a0, 4               # row * GRID_STRIDE
        add     t0, t0, a1              # + col
        slli    t0, t0, 2               # * 4 bytes
        add     t0, t0, s0
        lw      a0, 0(t0)
        ret

# grid_set(a0 = row, a1 = col, a2 = color)
# Writes one cell: s0 + ((row << 4) + col) << 2.
# Uses: t0.  Calls: none (leaf).  Saves: nothing.
# Reads globals: s0.
grid_set:
        slli    t0, a0, 4
        add     t0, t0, a1
        slli    t0, t0, 2
        add     t0, t0, s0
        sw      a2, 0(t0)
        ret

# grid_clear()
# Sets all 256 words to EMPTY (not just the current n x n).
# Uses: t0-t2.  Calls: none (leaf).  Saves: nothing.
# Reads globals: s0.
grid_clear:
        mv      t0, s0                  # t0 = current word
        addi    t1, s0, 1024            # t1 = end (256 words)
        li      t2, EMPTY
grid_clear_loop:
        sw      t2, 0(t0)
        addi    t0, t0, 4
        bltu    t0, t1, grid_clear_loop
        ret

# grid_cell_size(a0 = canvas, a1 = n) -> a0 = canvas / n (floor)
# Section 7.4 a. canvas is CANVAS_UNITS (96) in the asm.
# Uses: nothing.  Calls: none (leaf).  Saves: nothing.
grid_cell_size:
        divu    a0, a0, a1
        ret

# grid_offset(a0 = canvas, a1 = n) -> a0 = (canvas - n * cell) / 2
# Section 7.4 a: centers the canvas.
# Uses: t0, t1.  Calls: none (leaf).  Saves: nothing.
grid_offset:
        divu    t0, a0, a1              # t0 = cell
        mul     t1, a1, t0              # t1 = n * cell
        sub     t0, a0, t1              # t0 = canvas - n * cell
        srli    a0, t0, 1               # / 2
        ret

# luminance_q8(a0 = color) -> a0 = Y in 0..255
# Section 7.4 b, Q8 fixed point: Y = (77*R + 150*G + 29*B) >> 8.
# Uses: t0-t2.  Calls: none (leaf).  Saves: nothing.
luminance_q8:
        srli    t0, a0, 16
        andi    t0, t0, 0xFF            # R
        li      t1, 77
        mul     t0, t0, t1              # 77 * R
        srli    t1, a0, 8
        andi    t1, t1, 0xFF            # G
        li      t2, 150
        mul     t1, t1, t2              # 150 * G
        add     t0, t0, t1
        andi    t1, a0, 0xFF            # B
        li      t2, 29
        mul     t1, t1, t2              # 29 * B
        add     t0, t0, t1
        srli    a0, t0, 8               # >> 8 (Q8 -> integer)
        ret

# cursor_color(a0 = cell color) -> a0 = 0x000000 or 0xFFFFFF
# Black on light cells (Y >= 128) and on EMPTY, white on dark cells.
# Uses: t0 (+ luminance_q8's t0-t2).  Calls: luminance_q8.  Saves: ra.
cursor_color:
        li      t0, EMPTY
        beq     a0, t0, cursor_color_black
        addi    sp, sp, -16
        sw      ra, 12(sp)
        jal     luminance_q8
        lw      ra, 12(sp)
        addi    sp, sp, 16
        li      t0, 128
        bgeu    a0, t0, cursor_color_black
        li      a0, 0x00FFFFFF
        ret
cursor_color_black:
        li      a0, 0x00000000
        ret
