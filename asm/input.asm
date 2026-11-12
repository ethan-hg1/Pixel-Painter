# input.asm - keyboard MMIO polling and key dispatch.
# Key bytes measured in docs/keyboard_findings.md.

.include "constants.asm"

        .text
        .globl  read_key, handle_key

# read_key() -> a0 = key byte (0..255), or -1 if no key is waiting
# Polls KBD_CTRL bit 0; if set, reads KBD_DATA (which clears the bit).
# Uses: t0.  Calls: none (leaf).  Saves: nothing.
read_key:
        li      t0, KBD_CTRL
        lw      a0, 0(t0)
        andi    a0, a0, 1
        beqz    a0, read_key_none
        li      t0, KBD_DATA
        lw      a0, 0(t0)
        andi    a0, a0, 0xFF
        ret
read_key_none:
        li      a0, -1
        ret

# handle_key(a0 = key byte) -> a0 = 1 to quit, else 0
# Folds A-Z to lowercase, then dispatches:
#   w  paint the cursor cell, redraw it and the cursor
#   q  quit
# Other keys are ignored. More keys are added in later build steps.
# Uses: a0, a1, t0.  Calls: do_paint, draw_cell, draw_cursor.  Saves: ra.
# Reads globals: s3, s4 (and the callees' globals).
handle_key:
        addi    sp, sp, -16
        sw      ra, 12(sp)

        li      t0, 'A'
        blt     a0, t0, handle_key_dispatch
        li      t0, 'Z'
        bgt     a0, t0, handle_key_dispatch
        addi    a0, a0, 32              # 'A'..'Z' -> 'a'..'z'
handle_key_dispatch:
        li      t0, 'w'
        beq     a0, t0, handle_key_paint
        li      t0, 'q'
        beq     a0, t0, handle_key_quit
        j       handle_key_done

handle_key_paint:
        jal     do_paint
        beqz    a0, handle_key_done     # no change, nothing to redraw
        mv      a0, s3
        mv      a1, s4
        jal     draw_cell
        jal     draw_cursor
        j       handle_key_done

handle_key_quit:
        li      a0, 1
        j       handle_key_ret
handle_key_done:
        li      a0, 0
handle_key_ret:
        lw      ra, 12(sp)
        addi    sp, sp, 16
        ret
