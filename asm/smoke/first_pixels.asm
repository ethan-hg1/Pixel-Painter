# first_pixels.asm - Week 1 smoke test (BUILD_INSTRUCTIONS.txt section 5).
# Assembled on its own, NOT in project mode with asm/ (it has its own main).
#
# Draws on the RARS Bitmap Display (unit 4x4, display 512x512, base
# 0x10040000, so 128 x 128 units):
#   1. one red pixel at unit (4, 4)
#   2. a filled 8x8 orange block covering units x 16..23, y 16..23
#
# Every address comes from the display formula (section 7.2):
#   addr = DISPLAY_BASE + (y * DISPLAY_W + x) * 4
#        = DISPLAY_BASE + ((y << 7) + x) << 2

.include "../constants.asm"

.eqv PIXEL_X      4
.eqv PIXEL_Y      4
.eqv PIXEL_COLOR  0x00FF0000    # red
.eqv BLOCK_X      16
.eqv BLOCK_Y      16
.eqv BLOCK_SIZE   8
.eqv BLOCK_COLOR  0x00FF5733    # orange

        .globl main
        .text

# main()
# Draws the pixel and the block, then exits.
# Uses: a0-a4, a7.  Calls: set_pixel, fill_rect.  Saves: nothing (exits).
main:
        li      a0, PIXEL_X
        li      a1, PIXEL_Y
        li      a2, PIXEL_COLOR
        jal     set_pixel

        li      a0, BLOCK_X
        li      a1, BLOCK_Y
        li      a2, BLOCK_SIZE
        li      a3, BLOCK_SIZE
        li      a4, BLOCK_COLOR
        jal     fill_rect

        li      a0, 0
        li      a7, SYS_EXIT
        ecall

# set_pixel(a0 = x, a1 = y, a2 = color)
# Stores one display unit: DISPLAY_BASE + ((y << 7) + x) << 2.
# Uses: t0, t1.  Calls: none (leaf).  Saves: nothing.
set_pixel:
        slli    t0, a1, 7               # y * DISPLAY_W (128 = 1 << 7)
        add     t0, t0, a0              # + x
        slli    t0, t0, 2               # * 4 bytes per unit
        li      t1, DISPLAY_BASE
        add     t0, t0, t1
        sw      a2, 0(t0)
        ret

# fill_rect(a0 = x, a1 = y, a2 = width, a3 = height, a4 = color)
# Fills units x .. x+width-1, y .. y+height-1. Computes the address of the
# first unit with the display formula, then steps 4 bytes per column and
# DISPLAY_W*4 bytes per row.
# Uses: t0-t4.  Calls: none (leaf).  Saves: nothing.
fill_rect:
        slli    t0, a1, 7               # y * DISPLAY_W
        add     t0, t0, a0              # + x
        slli    t0, t0, 2               # * 4
        li      t1, DISPLAY_BASE
        add     t0, t0, t1              # t0 = address of row start
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
        addi    t0, t0, DISPLAY_ROW_BYTES  # next row: 128 units * 4 bytes
        addi    t2, t2, -1
        j       fill_rect_row
fill_rect_done:
        ret
