# input.asm - keyboard MMIO polling, key dispatch, color entry mode,
# console messages. Key bytes measured in docs/keyboard_findings.md.
# The RARS console plays the role of the C status line and hex text
# (BUILD_INSTRUCTIONS.txt 2.4, 2.5).

.include "constants.asm"

        .data
msg_prompt:     .asciz "\nColor: "
msg_color_set:  .asciz "\nColor set: "
msg_invalid:    .asciz "\nInvalid color\n"
msg_cancelled:  .asciz "\nCancelled\n"
msg_newline:    .asciz "\n"

        .text
        .globl  read_key, handle_key, print_color, print_current_color

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
# In color entry mode every key goes to handle_entry_key (no shortcuts).
# Otherwise folds A-Z to lowercase and dispatches:
#   i / j / k / l  move up / left / down / right (clamped, section 3)
#   w  paint the cursor cell, redraw it and the cursor
#   e  erase the cursor cell, redraw it and the cursor
#   c  clear the canvas, redraw everything
#   + / = grow, - shrink (RESIZE_STEP, within 8..16), redraw everything
#   z  undo (also 0x1A, ctrl+z if it arrives), redraw everything
#   y  redo, redraw everything
#   p  save the current color in the palette, redraw the swatches
#   1..8  pick that saved color (empty slot does nothing), print it
#   t  start color entry, print the prompt
#   q  quit
# Other keys are ignored.
# Uses: a0-a7, t0-t4.  Calls: handle_entry_key, do_paint, do_erase, do_clear,
# do_resize, do_undo, do_redo, move_cursor, draw_cell, draw_cursor,
# redraw_all, entry_start, palette_add, palette_pick, draw_palette,
# draw_preview, print_current_color.
# Saves: ra, s8, s9.
# Reads globals: s3, s4 (and the callees' globals).
handle_key:
        addi    sp, sp, -16
        sw      ra, 12(sp)
        sw      s8, 8(sp)
        sw      s9, 4(sp)

        la      t0, entry_mode
        lw      t0, 0(t0)
        beqz    t0, handle_key_fold
        jal     handle_entry_key
        j       handle_key_done

handle_key_fold:
        li      t0, 'A'
        blt     a0, t0, handle_key_dispatch
        li      t0, 'Z'
        bgt     a0, t0, handle_key_dispatch
        addi    a0, a0, 32              # 'A'..'Z' -> 'a'..'z'
handle_key_dispatch:
        li      t0, 'w'
        beq     a0, t0, handle_key_paint
        li      t0, 'e'
        beq     a0, t0, handle_key_erase
        li      t0, 'c'
        beq     a0, t0, handle_key_clear
        li      t0, '+'
        beq     a0, t0, handle_key_grow
        li      t0, '='
        beq     a0, t0, handle_key_grow
        li      t0, '-'
        beq     a0, t0, handle_key_shrink
        li      t0, 'z'
        beq     a0, t0, handle_key_undo
        li      t0, 0x1A                # ctrl+z, when the terminal sends it
        beq     a0, t0, handle_key_undo
        li      t0, 'y'
        beq     a0, t0, handle_key_redo
        li      t0, 'i'
        beq     a0, t0, handle_key_up
        li      t0, 'k'
        beq     a0, t0, handle_key_down
        li      t0, 'j'
        beq     a0, t0, handle_key_left
        li      t0, 'l'
        beq     a0, t0, handle_key_right
        li      t0, 't'
        beq     a0, t0, handle_key_entry
        li      t0, 'q'
        beq     a0, t0, handle_key_quit
        li      t0, 'p'
        beq     a0, t0, handle_key_palette_add
        li      t0, '1'                 # keys 1..8 pick a saved color
        blt     a0, t0, handle_key_done
        li      t0, '8'
        bgt     a0, t0, handle_key_done
        addi    a0, a0, -49             # '1' -> slot 0
        j       handle_key_pick

handle_key_paint:
        jal     do_paint
        mv      s8, a0                  # s8 = cell changed
        jal     draw_palette            # the first paint may save a swatch
        beqz    s8, handle_key_done     # no change, nothing else to redraw
        mv      a0, s3
        mv      a1, s4
        jal     draw_cell
        jal     draw_cursor
        j       handle_key_done

handle_key_palette_add:
        jal     palette_add
        jal     draw_palette
        j       handle_key_done

handle_key_pick:                        # a0 = slot 0..7
        jal     palette_pick
        beqz    a0, handle_key_done
        jal     draw_preview
        jal     draw_palette
        jal     print_current_color
        j       handle_key_done

handle_key_erase:
        jal     do_erase
        beqz    a0, handle_key_done
        mv      a0, s3
        mv      a1, s4
        jal     draw_cell
        jal     draw_cursor
        j       handle_key_done

handle_key_clear:
        jal     do_clear
        beqz    a0, handle_key_done
        jal     redraw_all
        j       handle_key_done

handle_key_grow:
        li      a0, RESIZE_STEP
        jal     do_resize
        beqz    a0, handle_key_done
        jal     redraw_all
        j       handle_key_done

handle_key_shrink:
        li      a0, -RESIZE_STEP
        jal     do_resize
        beqz    a0, handle_key_done
        jal     redraw_all
        j       handle_key_done

handle_key_undo:
        jal     do_undo
        beqz    a0, handle_key_done
        jal     redraw_all
        j       handle_key_done

handle_key_redo:
        jal     do_redo
        beqz    a0, handle_key_done
        jal     redraw_all
        j       handle_key_done

handle_key_up:
        li      s8, -1
        li      s9, 0
        j       handle_key_move
handle_key_down:
        li      s8, 1
        li      s9, 0
        j       handle_key_move
handle_key_left:
        li      s8, 0
        li      s9, -1
        j       handle_key_move
handle_key_right:
        li      s8, 0
        li      s9, 1

# Redraw the old cell first (that erases the old cursor border), then move,
# then draw the cursor in its new cell. Draws even when clamped: harmless.
handle_key_move:
        mv      a0, s3
        mv      a1, s4
        jal     draw_cell
        mv      a0, s8
        mv      a1, s9
        jal     move_cursor
        jal     draw_cursor
        j       handle_key_done

handle_key_entry:
        jal     entry_start
        la      a0, msg_prompt
        li      a7, SYS_PRINT_STRING
        ecall
        j       handle_key_done

handle_key_quit:
        li      a0, 1
        j       handle_key_ret
handle_key_done:
        li      a0, 0
handle_key_ret:
        lw      s9, 4(sp)
        lw      s8, 8(sp)
        lw      ra, 12(sp)
        addi    sp, sp, 16
        ret

# redraw_all()
# Full redraw after a change of n or of the whole grid (c, +, -, z, y). The
# background covers the whole display, so the preview swatch and the
# palette are drawn again too. Same order as main's first draw.
# Uses: a0-a4, t0-t4.  Calls: draw_background, draw_canvas, draw_cursor,
# draw_preview, draw_palette.  Saves: ra.
redraw_all:
        addi    sp, sp, -16
        sw      ra, 12(sp)
        jal     draw_background
        jal     draw_canvas
        jal     draw_cursor
        jal     draw_preview
        jal     draw_palette
        lw      ra, 12(sp)
        addi    sp, sp, 16
        ret

# handle_entry_key(a0 = key byte)
# Color entry mode (2.4):
#   0x0A / 0x0D  Enter: confirm. Valid -> redraw preview, print the color;
#                invalid -> print "Invalid color", color unchanged.
#   0x1B         Esc: cancel.
#   0x08 / 0x7F  Backspace: delete one char, reprint the typed text.
#   other        append if printable and room left; echo it.
# Uses: a0-a7, t0-t4.  Calls: entry_confirm, entry_cancel,
# entry_backspace, entry_add_char, draw_preview, print_current_color.
# Saves: ra, s8.
handle_entry_key:
        addi    sp, sp, -16
        sw      ra, 12(sp)
        sw      s8, 8(sp)
        mv      s8, a0                  # s8 = key

        li      t0, 0x0A
        beq     s8, t0, entry_key_enter
        li      t0, 0x0D
        beq     s8, t0, entry_key_enter
        li      t0, 0x1B
        beq     s8, t0, entry_key_esc
        li      t0, 0x08
        beq     s8, t0, entry_key_back
        li      t0, 0x7F
        beq     s8, t0, entry_key_back

        jal     entry_add_char          # a0 = key
        beqz    a0, entry_key_done      # ignored: no echo
        mv      a0, s8
        li      a7, SYS_PRINT_CHAR
        ecall
        j       entry_key_done

entry_key_enter:
        jal     entry_confirm
        beqz    a0, entry_key_invalid
        jal     draw_preview
        jal     print_current_color
        j       entry_key_done
entry_key_invalid:
        la      a0, msg_invalid
        li      a7, SYS_PRINT_STRING
        ecall
        j       entry_key_done

entry_key_esc:
        jal     entry_cancel
        la      a0, msg_cancelled
        li      a7, SYS_PRINT_STRING
        ecall
        j       entry_key_done

entry_key_back:
        jal     entry_backspace
        la      a0, msg_prompt          # the console cannot erase, so
        li      a7, SYS_PRINT_STRING    # reprint the prompt and the text
        ecall
        la      a0, entry_buf
        li      a7, SYS_PRINT_STRING
        ecall

entry_key_done:
        lw      s8, 8(sp)
        lw      ra, 12(sp)
        addi    sp, sp, 16
        ret

# print_current_color()
# Prints "\nColor set: #RRGGBB\n" for the current color (s2).
# Uses: a0, a7.  Calls: print_color.  Saves: ra.
# Reads globals: s2.
print_current_color:
        addi    sp, sp, -16
        sw      ra, 12(sp)
        la      a0, msg_color_set
        li      a7, SYS_PRINT_STRING
        ecall
        mv      a0, s2
        jal     print_color
        la      a0, msg_newline
        li      a7, SYS_PRINT_STRING
        ecall
        lw      ra, 12(sp)
        addi    sp, sp, 16
        ret

# print_color(a0 = color)
# Prints '#' and 6 uppercase hex digits, high nibble first.
# Uses: a0, a7, t0.  Calls: none (syscalls only).  Saves: s8, s9.
print_color:
        addi    sp, sp, -16
        sw      s8, 8(sp)
        sw      s9, 4(sp)
        mv      s8, a0                  # s8 = color
        li      a0, '#'
        li      a7, SYS_PRINT_CHAR
        ecall
        li      s9, 20                  # s9 = shift: 20, 16, ..., 0
print_color_loop:
        srl     t0, s8, s9
        andi    t0, t0, 0xF             # nibble
        li      a0, 10
        blt     t0, a0, print_color_num
        addi    a0, t0, 55              # 10..15 -> 'A'..'F'
        j       print_color_put
print_color_num:
        addi    a0, t0, 48              # 0..9 -> '0'..'9'
print_color_put:
        li      a7, SYS_PRINT_CHAR
        ecall
        addi    s9, s9, -4
        bgez    s9, print_color_loop
        lw      s9, 4(sp)
        lw      s8, 8(sp)
        addi    sp, sp, 16
        ret
