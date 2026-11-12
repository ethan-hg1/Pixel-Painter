# draw.asm - display drawing: fill_rect, draw_background, draw_cell,
# draw_canvas, draw_cursor. Counterpart of c/src/render.c.
#
# Display unit address (section 7.2):
#   addr = DISPLAY_BASE + ((y << 7) + x) << 2      (DISPLAY_W = 128)
# Cell (row, col) covers units x = s6 + col*s5 .. + s5-1, y = s6 + row*s5 ..

.include "constants.asm"

        .text
        .globl  fill_rect, draw_background, draw_cell, draw_canvas, draw_cursor

# fill_rect(a0 = x, a1 = y, a2 = width, a3 = height, a4 = color)
# Fills units x .. x+width-1, y .. y+height-1. Computes the first unit's
# address with the display formula, then steps 4 bytes per column and
# DISPLAY_ROW_BYTES per row.
# Uses: t0-t4.  Calls: none (leaf).  Saves: nothing.
# Reads globals: s7.
fill_rect:
        slli    t0, a1, 7               # y * DISPLAY_W
        add     t0, t0, a0              # + x
        slli    t0, t0, 2               # * 4
        add     t0, t0, s7              # t0 = address of row start
        mv      t2, a3                  # t2 = rows left
fill_rect_row:
        beqz    t2, fill_rect_done
        mv      t3, t0                  # t3 = address in this row
        mv      t4, a2                  # t4 = columns left
fill_rect_col:
        beqz    t4, fill_rect_next_row
        sw      a4, 0(t3)
        addi    t3, t3, 4
        addi    t4, t4, -1
        j       fill_rect_col
fill_rect_next_row:
        addi    t0, t0, DISPLAY_ROW_BYTES
        addi    t2, t2, -1
        j       fill_rect_row
fill_rect_done:
        ret

# draw_background()
# Fills the whole 128 x 128 display with BG_COLOR.
# Uses: a0-a4.  Calls: fill_rect.  Saves: ra.
# Reads globals: s7.
draw_background:
        addi    sp, sp, -16
        sw      ra, 12(sp)
        li      a0, 0
        li      a1, 0
        li      a2, DISPLAY_W
        li      a3, DISPLAY_H
        li      a4, BG_COLOR
        jal     fill_rect
        lw      ra, 12(sp)
        addi    sp, sp, 16
        ret

# draw_cell(a0 = row, a1 = col)
# Draws one grid cell at its scaled display position. EMPTY cells use the
# checker colors: EMPTY_DARK when (row + col) is odd, else EMPTY_LIGHT.
# Uses: a0-a4, t0.  Calls: grid_get, fill_rect.  Saves: ra, s8, s9.
# Reads globals: s0, s5, s6, s7.
draw_cell:
        addi    sp, sp, -16
        sw      ra, 12(sp)
        sw      s8, 8(sp)
        sw      s9, 4(sp)
        mv      s8, a0                  # s8 = row
        mv      s9, a1                  # s9 = col

        jal     grid_get                # a0 = cell color
        li      t0, EMPTY
        bne     a0, t0, draw_cell_paint
        add     t0, s8, s9
        andi    t0, t0, 1
        li      a0, EMPTY_LIGHT
        beqz    t0, draw_cell_paint
        li      a0, EMPTY_DARK
draw_cell_paint:
        mv      a4, a0                  # color
        mul     a0, s9, s5
        add     a0, a0, s6              # x = offset + col * cell
        mul     a1, s8, s5
        add     a1, a1, s6              # y = offset + row * cell
        mv      a2, s5                  # width = cell
        mv      a3, s5                  # height = cell
        jal     fill_rect

        lw      s9, 4(sp)
        lw      s8, 8(sp)
        lw      ra, 12(sp)
        addi    sp, sp, 16
        ret

# draw_canvas()
# Draws every cell of the current n x n grid.
# Uses: a0, a1.  Calls: draw_cell.  Saves: ra, s8, s9.
# Reads globals: s1 (and draw_cell's s0, s5, s6, s7).
draw_canvas:
        addi    sp, sp, -16
        sw      ra, 12(sp)
        sw      s8, 8(sp)
        sw      s9, 4(sp)
        li      s8, 0                   # s8 = row
draw_canvas_row:
        bge     s8, s1, draw_canvas_done
        li      s9, 0                   # s9 = col
draw_canvas_col:
        bge     s9, s1, draw_canvas_next_row
        mv      a0, s8
        mv      a1, s9
        jal     draw_cell
        addi    s9, s9, 1
        j       draw_canvas_col
draw_canvas_next_row:
        addi    s8, s8, 1
        j       draw_canvas_row
draw_canvas_done:
        lw      s9, 4(sp)
        lw      s8, 8(sp)
        lw      ra, 12(sp)
        addi    sp, sp, 16
        ret

# draw_cursor()
# Draws a 1-unit border inside the cursor cell, in cursor_color(cell):
# top, bottom, left and right edges as four fill_rects.
# Uses: a0-a4.  Calls: grid_get, cursor_color, fill_rect.
# Saves: ra, s8, s9, s10.
# Reads globals: s0, s3, s4, s5, s6, s7.
draw_cursor:
        addi    sp, sp, -16
        sw      ra, 12(sp)
        sw      s8, 8(sp)
        sw      s9, 4(sp)
        sw      s10, 0(sp)

        mv      a0, s3
        mv      a1, s4
        jal     grid_get
        jal     cursor_color
        mv      s8, a0                  # s8 = border color
        mul     s9, s4, s5
        add     s9, s9, s6              # s9 = x of cell
        mul     s10, s3, s5
        add     s10, s10, s6            # s10 = y of cell

        mv      a0, s9                  # top edge
        mv      a1, s10
        mv      a2, s5
        li      a3, 1
        mv      a4, s8
        jal     fill_rect

        mv      a0, s9                  # bottom edge
        add     a1, s10, s5
        addi    a1, a1, -1
        mv      a2, s5
        li      a3, 1
        mv      a4, s8
        jal     fill_rect

        mv      a0, s9                  # left edge
        mv      a1, s10
        li      a2, 1
        mv      a3, s5
        mv      a4, s8
        jal     fill_rect

        add     a0, s9, s5              # right edge
        addi    a0, a0, -1
        mv      a1, s10
        li      a2, 1
        mv      a3, s5
        mv      a4, s8
        jal     fill_rect

        lw      s10, 0(sp)
        lw      s9, 4(sp)
        lw      s8, 8(sp)
        lw      ra, 12(sp)
        addi    sp, sp, 16
        ret
