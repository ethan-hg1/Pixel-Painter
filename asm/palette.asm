# palette.asm - 8-slot saved colors, FIFO replace (2.6).
# Translation of c/src/palette.c (same function names). The state
# (palette, palette_next) is declared in main.asm.
#
# palette_init, palette_insert and palette_find are pure logic and are
# unit-tested in asm/tests/test_main.asm. palette_slot_at is the hit test
# for mouse clicks: RARS has no mouse, so the program never calls it, but
# it is kept for the function-by-function mapping (same as grid_cell_at).

.include "constants.asm"

        .text
        .globl  palette_init, palette_find, palette_insert, palette_slot_at

# palette_init()
# Every slot EMPTY, palette_next = 0.
# WRITES GLOBALS: palette, palette_next (this is its job).
# Uses: t0-t2.  Calls: none (leaf).  Saves: nothing.
palette_init:
        la      t0, palette
        li      t1, EMPTY
        li      t2, PALETTE_SLOTS
palette_init_loop:
        beqz    t2, palette_init_done
        sw      t1, 0(t0)
        addi    t0, t0, 4
        addi    t2, t2, -1
        j       palette_init_loop
palette_init_done:
        la      t0, palette_next
        sw      zero, 0(t0)
        ret

# palette_find(a0 = color) -> a0 = slot index, or -1 if not saved
# Uses: t0-t2.  Calls: none (leaf).  Saves: nothing.  Reads globals: palette.
palette_find:
        la      t0, palette
        li      t1, 0                   # t1 = slot
palette_find_loop:
        li      t2, PALETTE_SLOTS
        bge     t1, t2, palette_find_none
        lw      t2, 0(t0)
        beq     t2, a0, palette_find_hit
        addi    t0, t0, 4
        addi    t1, t1, 1
        j       palette_find_loop
palette_find_hit:
        mv      a0, t1
        ret
palette_find_none:
        li      a0, -1
        ret

# palette_insert(a0 = color) -> a0 = 1 if saved, 0 if not
# Saves color at palette_next and advances it (mod 8, the FIFO ring). A
# color already saved, and EMPTY, are not saved again (returns 0).
# WRITES GLOBALS: palette, palette_next (only when saved).
# Uses: a0-a1, t0-t3, s8.  Calls: palette_find.  Saves: ra, s8.
palette_insert:
        addi    sp, sp, -16
        sw      ra, 12(sp)
        sw      s8, 8(sp)
        mv      s8, a0                  # s8 = color
        li      t0, EMPTY
        beq     s8, t0, palette_insert_no
        jal     palette_find
        bgez    a0, palette_insert_no   # already saved

        la      t0, palette_next
        lw      t1, 0(t0)               # t1 = next slot
        la      t2, palette
        slli    t3, t1, 2
        add     t2, t2, t3
        sw      s8, 0(t2)               # palette[next] = color
        addi    t1, t1, 1
        andi    t1, t1, 7               # mod PALETTE_SLOTS (8)
        la      t0, palette_next
        sw      t1, 0(t0)
        li      a0, 1
        j       palette_insert_ret
palette_insert_no:
        li      a0, 0
palette_insert_ret:
        lw      s8, 8(sp)
        lw      ra, 12(sp)
        addi    sp, sp, 16
        ret

# palette_slot_at(a0 = px, a1 = py, a2 = x0, a3 = y0, a4 = size,
#                 a5 = pitch_x, a6 = pitch_y) -> a0 = 1 if on a swatch, else 0
# Swatches form two columns (slot k in column k & 1, row k >> 1). The top-
# left swatch is at (x0, y0). On a hit, a1 = the slot (0..7); on a miss a1
# is unchanged.
# Uses: a0, a1, t0-t6.  Calls: none (leaf).  Saves: nothing.
palette_slot_at:
        sub     t0, a0, a2              # dx
        sub     t1, a1, a3              # dy
        bltz    t0, palette_slot_no
        bltz    t1, palette_slot_no
        div     t2, t0, a5              # col
        div     t3, t1, a6              # row
        rem     t4, t0, a5              # x inside the pitch
        bge     t4, a4, palette_slot_no # gap between columns
        rem     t4, t1, a6              # y inside the pitch
        bge     t4, a4, palette_slot_no # gap between rows
        li      t5, 1
        bgt     t2, t5, palette_slot_no # only two columns
        slli    t6, t3, 1
        add     t6, t6, t2              # slot = row * 2 + col
        li      t5, PALETTE_SLOTS
        bge     t6, t5, palette_slot_no
        mv      a1, t6
        li      a0, 1
        ret
palette_slot_no:
        li      a0, 0
        ret
