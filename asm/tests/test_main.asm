# test_main.asm - headless unit tests for the asm modules.
# Counterpart of c/tests/test_main.c: same cases, same expected values.
#
# Run (exit code = number of failed checks, 0 = all passed):
#   java -jar rars1_6.jar nc sm se2 ae1 asm/tests/test_main.asm \
#        asm/grid.asm asm/hexparse.asm asm/app.asm
# The modules are listed as extra files, not .include'd: RARS refuses to
# include constants.asm twice in one assembly. `sm` starts at this
# file's `main` (its helpers come first in .text).
#
# Needs no Bitmap Display or keyboard: only pure logic is tested here
# (grid formulas, parse_hex, color entry), using the real module files.
#
# Registers: s11 = failed checks, s10 = total checks (never touched by the
# modules under test, which save/restore every s-register they use other
# than the globals s0-s7).

.include "../constants.asm"

        .data
        .align  2
# State the modules expect (normally declared in main.asm).
        .globl  grid, n, entry_mode, entry_len, entry_buf
grid:       .space  1024
n:          .word   START_N
entry_mode: .word   0
entry_len:  .word   0
entry_buf:  .space  8

msg_fail:       .asciz "FAIL "
msg_got:        .asciz ": got "
msg_want:       .asciz " want "
msg_nl:         .asciz "\n"
msg_checks:     .asciz " checks, "
msg_failed:     .asciz " failed\n"

# --- parse_hex cases (12.5 list + edge cases): string, ok, value ---------
s_h1:   .asciz "#FF5733"
s_h2:   .asciz "ff5733"
s_h3:   .asciz "#ff5733"
s_h4:   .asciz "#00FF00"
s_h5:   .asciz "000000"
s_h6:   .asciz "#aBcDeF"
s_b1:   .asciz ""
s_b2:   .asciz "#"
s_b3:   .asciz "FF573"
s_b4:   .asciz "FF57333"
s_b5:   .asciz "#GG0000"
s_b6:   .asciz "12 456"
s_b7:   .asciz "#12345G"
s_b8:   .asciz "##FF573"
s_b9:   .asciz "FF5733#"
s_b10:  .asciz "0xFF57"
        .align  2
hex_cases:
        .word   s_h1, 1, 0x00FF5733
        .word   s_h2, 1, 0x00FF5733
        .word   s_h3, 1, 0x00FF5733
        .word   s_h4, 1, 0x0000FF00
        .word   s_h5, 1, 0x00000000
        .word   s_h6, 1, 0x00ABCDEF
        .word   s_b1, 0, 0
        .word   s_b2, 0, 0
        .word   s_b3, 0, 0
        .word   s_b4, 0, 0
        .word   s_b5, 0, 0
        .word   s_b6, 0, 0
        .word   s_b7, 0, 0
        .word   s_b8, 0, 0
        .word   s_b9, 0, 0
        .word   s_b10, 0, 0
        .word   0                       # end of table

# --- hex_digit_value: char, expected -------------------------------------
digit_cases:
        .word   '0', 0,  '9', 9,  'a', 10, 'f', 15, 'A', 10, 'F', 15
        .word   '/', -1, ':', -1, '@', -1, 'G', -1, '`', -1, 'g', -1
        .word   0

# --- cell size / offset for n = 8..16 on the 96-unit canvas (7.4 a) ------
cell_expect:    .word 12, 10, 9, 8, 8, 7, 6, 6, 6
off_expect:     .word  0,  3, 3, 4, 0, 2, 6, 3, 0

# --- labels printed on failure -------------------------------------------
l_parse_ok:     .asciz "parse_hex ok flag for "
l_parse_val:    .asciz "parse_hex value for "
l_digit:        .asciz "hex_digit_value"
l_cell:         .asciz "grid_cell_size"
l_off:          .asciz "grid_offset"
l_lum:          .asciz "luminance_q8"
l_cursor:       .asciz "cursor_color"
l_addr:         .asciz "grid_set/grid_get address"
l_paint:        .asciz "do_paint"
l_entry:        .asciz "entry flow "
t_green:        .asciz "#00FF00"
t_bad:          .asciz "#12345G"
t_cancel:       .asciz "ff0000"
t_long:         .asciz "#FF00001"
t_ff:           .asciz "FF"
l_move:         .asciz "move_cursor"
l_erase:        .asciz "do_erase"
l_hit:          .asciz "grid_cell_at"
        .align  2
hit_row:        .word   0
hit_col:        .word   0
# grid_cell_at table: px, py, n, ok, row, col. Canvas at (30, 30), 640 px.
# n = 0 ends the table. Row/col are only checked when ok = 1.
hit_cases:
        .word   30, 30, 8, 1, 0, 0      # top-left px
        .word   109, 109, 8, 1, 0, 0    # last px of cell (0,0)
        .word   110, 110, 8, 1, 1, 1
        .word   669, 669, 8, 1, 7, 7    # bottom-right px
        .word   670, 500, 8, 0, 0, 0    # right of the canvas
        .word   500, 670, 8, 0, 0, 0    # below the canvas
        .word   29, 500, 8, 0, 0, 0     # left of the canvas
        .word   669, 630, 16, 1, 15, 15
        .word   670, 40, 16, 0, 0, 0
        .word   30, 30, 11, 0, 0, 0     # n = 11: margin (offset 1)
        .word   30, 500, 11, 0, 0, 0
        .word   31, 31, 11, 1, 0, 0
        .word   89, 147, 11, 1, 2, 1
        .word   669, 300, 11, 0, 0, 0   # 638 px used: 669 is outside
        .word   668, 31, 11, 1, 0, 10
        .word   0, 0, 0, 0, 0, 0        # end

        .text
        .globl  main

# check(a0 = got, a1 = want, a2 = label, a3 = extra string or 0)
# Counts one check; on mismatch prints "FAIL <label><extra>: got X want Y".
# Uses: a0-a3, a7, t0, t1.  Calls: none (syscalls).  Saves: nothing.
# Writes: s10 (+1), s11 (+1 on failure).
check:
        addi    s10, s10, 1
        beq     a0, a1, check_ok
        addi    s11, s11, 1
        mv      t0, a0
        mv      t1, a1
        la      a0, msg_fail
        li      a7, SYS_PRINT_STRING
        ecall
        mv      a0, a2
        ecall
        beqz    a3, check_no_extra
        mv      a0, a3
        ecall
check_no_extra:
        la      a0, msg_got
        ecall
        mv      a0, t0
        li      a7, SYS_PRINT_INT_HEX
        ecall
        la      a0, msg_want
        li      a7, SYS_PRINT_STRING
        ecall
        mv      a0, t1
        li      a7, SYS_PRINT_INT_HEX
        ecall
        la      a0, msg_nl
        li      a7, SYS_PRINT_STRING
        ecall
check_ok:
        ret

# CHECK(got_reg, want_imm_or_reg via li/mv done by caller) is spelled out
# inline below: set a0 (got), a1 (want), a2 (label), a3 (extra), jal check.

# type_string(a0 = address of 0-terminated string)
# Calls entry_add_char for every char.
# Uses: a0, t0-t2.  Calls: entry_add_char.  Saves: ra, s8.
type_string:
        addi    sp, sp, -16
        sw      ra, 12(sp)
        sw      s8, 8(sp)
        mv      s8, a0
type_string_loop:
        lbu     a0, 0(s8)
        beqz    a0, type_string_done
        jal     entry_add_char
        addi    s8, s8, 1
        j       type_string_loop
type_string_done:
        lw      s8, 8(sp)
        lw      ra, 12(sp)
        addi    sp, sp, 16
        ret

main:
        li      s10, 0
        li      s11, 0
        jal     app_init                # sets s0-s7, empty grid

        # ---- parse_hex table ---------------------------------------------
        la      s8, hex_cases
hex_loop:
        lw      a0, 0(s8)
        beqz    a0, hex_done
        jal     parse_hex
        mv      s9, a1                  # value
        lw      a1, 4(s8)               # want ok
        la      a2, l_parse_ok
        lw      a3, 0(s8)
        jal     check
        lw      t0, 4(s8)
        beqz    t0, hex_next            # value only matters when valid
        mv      a0, s9
        lw      a1, 8(s8)
        la      a2, l_parse_val
        lw      a3, 0(s8)
        jal     check
hex_next:
        addi    s8, s8, 12
        j       hex_loop
hex_done:

        # ---- hex_digit_value table ---------------------------------------
        la      s8, digit_cases
digit_loop:
        lw      a0, 0(s8)
        beqz    a0, digit_done
        jal     hex_digit_value
        lw      a1, 4(s8)
        la      a2, l_digit
        li      a3, 0
        jal     check
        addi    s8, s8, 8
        j       digit_loop
digit_done:

        # ---- cell size / offset for n = 8..16 ----------------------------
        li      s8, MIN_N
cell_loop:
        li      t0, MAX_N
        bgt     s8, t0, cell_done
        li      a0, CANVAS_UNITS
        mv      a1, s8
        jal     grid_cell_size
        addi    t0, s8, -MIN_N
        slli    t0, t0, 2
        la      t1, cell_expect
        add     t1, t1, t0
        lw      a1, 0(t1)
        la      a2, l_cell
        li      a3, 0
        jal     check
        li      a0, CANVAS_UNITS
        mv      a1, s8
        jal     grid_offset
        addi    t0, s8, -MIN_N
        slli    t0, t0, 2
        la      t1, off_expect
        add     t1, t1, t0
        lw      a1, 0(t1)
        la      a2, l_off
        li      a3, 0
        jal     check
        addi    s8, s8, 1
        j       cell_loop
cell_done:

        # ---- luminance / cursor color ------------------------------------
        li      a0, 0x00FFFFFF
        jal     luminance_q8
        li      a1, 255
        la      a2, l_lum
        li      a3, 0
        jal     check
        li      a0, 0x00000000
        jal     cursor_color
        li      a1, 0x00FFFFFF          # black cell -> white cursor
        la      a2, l_cursor
        li      a3, 0
        jal     check
        li      a0, 0x00FFFFFF
        jal     cursor_color
        li      a1, 0                   # white cell -> black cursor
        la      a2, l_cursor
        li      a3, 0
        jal     check
        li      a0, EMPTY
        jal     cursor_color
        li      a1, 0                   # EMPTY counts as light
        la      a2, l_cursor
        li      a3, 0
        jal     check
        li      a0, 0x000000FF
        jal     cursor_color
        li      a1, 0x00FFFFFF          # blue: Y = 28
        la      a2, l_cursor
        li      a3, 0
        jal     check
        li      a0, 0x0000FF00
        jal     cursor_color
        li      a1, 0                   # green: Y = 149
        la      a2, l_cursor
        li      a3, 0
        jal     check

        # ---- grid address formula: (15, 0) is word 240 -------------------
        li      a0, 15
        li      a1, 0
        li      a2, 0x00123456
        jal     grid_set
        lw      a0, 960(s0)             # (15 << 4 + 0) << 2 = 960
        li      a1, 0x00123456
        la      a2, l_addr
        li      a3, 0
        jal     check
        li      a0, 15
        li      a1, 15
        li      a2, 0x00ABCDEF
        jal     grid_set
        lw      a0, 1020(s0)            # (15 << 4 + 15) << 2 = 1020
        li      a1, 0x00ABCDEF
        la      a2, l_addr
        li      a3, 0
        jal     check

        # ---- do_paint ------------------------------------------------------
        jal     do_paint                # (0,0) EMPTY -> black: changed
        li      a1, 1
        la      a2, l_paint
        li      a3, 0
        jal     check
        jal     do_paint                # same color: no change
        li      a1, 0
        la      a2, l_paint
        li      a3, 0
        jal     check

        # ---- color entry flow (same as test_color_entry in C) -------------
        jal     entry_start
        la      a0, t_green
        jal     type_string
        jal     entry_confirm
        li      a1, 1                   # "#00FF00" valid
        la      a2, l_entry
        la      a3, t_green
        jal     check
        mv      a0, s2
        li      a1, 0x0000FF00
        la      a2, l_entry
        la      a3, t_green
        jal     check
        la      t0, entry_mode
        lw      a0, 0(t0)
        li      a1, 0                   # left entry mode
        la      a2, l_entry
        li      a3, 0
        jal     check

        jal     entry_start
        la      a0, t_bad
        jal     type_string
        jal     entry_confirm
        li      a1, 0                   # "#12345G" invalid
        la      a2, l_entry
        la      a3, t_bad
        jal     check
        mv      a0, s2
        li      a1, 0x0000FF00          # color unchanged
        la      a2, l_entry
        la      a3, t_bad
        jal     check

        jal     entry_start
        la      a0, t_cancel
        jal     type_string
        jal     entry_cancel
        mv      a0, s2
        li      a1, 0x0000FF00          # Esc: unchanged
        la      a2, l_entry
        la      a3, t_cancel
        jal     check

        jal     entry_start
        la      a0, t_long
        jal     type_string
        la      t0, entry_len
        lw      a0, 0(t0)
        li      a1, ENTRY_MAX_LEN       # 8th char dropped
        la      a2, l_entry
        la      a3, t_long
        jal     check
        jal     entry_backspace
        jal     entry_backspace
        la      a0, t_ff
        jal     type_string             # "#FF00" + "FF" = "#FF00FF"
        jal     entry_confirm
        mv      a0, s2
        li      a1, 0x00FF00FF
        la      a2, l_entry
        la      a3, t_long
        jal     check

        jal     entry_start
        jal     entry_backspace         # on empty: no-op
        la      t0, entry_len
        lw      a0, 0(t0)
        li      a1, 0
        la      a2, l_entry
        li      a3, 0
        jal     check
        li      a0, 0x1B
        jal     entry_add_char
        li      a1, 0                   # control char ignored
        la      a2, l_entry
        li      a3, 0
        jal     check
        li      a0, 0x7F
        jal     entry_add_char
        li      a1, 0
        la      a2, l_entry
        li      a3, 0
        jal     check
        jal     entry_confirm
        li      a1, 0                   # empty is invalid
        la      a2, l_entry
        li      a3, 0
        jal     check

        # ---- move_cursor: clamp at every edge, n = 8 then n = 16 ---------
        li      s1, START_N
        la      t0, n
        sw      s1, 0(t0)
        li      s3, 0
        li      s4, 0
        jal     move_cursor_checks      # n = 8 (last = 7)
        li      s1, MAX_N
        la      t0, n
        sw      s1, 0(t0)
        li      s3, 0
        li      s4, 0
        jal     move_cursor_checks      # n = 16 (last = 15)
        li      s1, START_N             # back to 8 for the erase tests
        la      t0, n
        sw      s1, 0(t0)

        # ---- do_erase: no-op on EMPTY, then paint and erase --------------
        li      s3, 0
        li      s4, 0
        li      a0, 0
        li      a1, 0
        li      a2, EMPTY
        jal     grid_set                # (0,0) = EMPTY
        jal     do_erase
        li      a1, 0                   # already EMPTY: no change
        la      a2, l_erase
        li      a3, 0
        jal     check
        li      s2, 0x00ABCDEF
        jal     do_paint
        li      a1, 1
        la      a2, l_erase
        li      a3, 0
        jal     check
        jal     do_erase
        li      a1, 1                   # painted cell: erased
        la      a2, l_erase
        li      a3, 0
        jal     check
        li      a0, 0
        li      a1, 0
        jal     grid_get
        li      a1, EMPTY
        la      a2, l_erase
        li      a3, 0
        jal     check
        mv      a0, s2
        li      a1, 0x00ABCDEF          # erase keeps the current color
        la      a2, l_erase
        li      a3, 0
        jal     check

        # ---- grid_cell_at: table-driven hit test (origin 30, 30; 640 px) -
        la      s8, hit_cases
hit_loop:
        lw      t0, 8(s8)               # n = 0 ends the table
        beqz    t0, hit_done
        lw      a0, 0(s8)
        lw      a1, 4(s8)
        li      a2, 30
        li      a3, 30
        li      a4, 640
        lw      a5, 8(s8)
        jal     grid_cell_at
        la      t0, hit_row
        sw      a1, 0(t0)
        la      t0, hit_col
        sw      a2, 0(t0)
        lw      a1, 12(s8)              # want ok
        la      a2, l_hit
        li      a3, 0
        jal     check
        lw      t0, 12(s8)
        beqz    t0, hit_next            # outside: row/col not checked
        la      t0, hit_row
        lw      a0, 0(t0)
        lw      a1, 16(s8)
        la      a2, l_hit
        li      a3, 0
        jal     check
        la      t0, hit_col
        lw      a0, 0(t0)
        lw      a1, 20(s8)
        la      a2, l_hit
        li      a3, 0
        jal     check
hit_next:
        addi    s8, s8, 24
        j       hit_loop
hit_done:

        # ---- summary: "<checks> checks, <failed> failed", exit(failed) ----
        mv      a0, s10
        li      a7, SYS_PRINT_INT
        ecall
        la      a0, msg_checks
        li      a7, SYS_PRINT_STRING
        ecall
        mv      a0, s11
        li      a7, SYS_PRINT_INT
        ecall
        la      a0, msg_failed
        li      a7, SYS_PRINT_STRING
        ecall
        mv      a0, s11
        li      a7, SYS_EXIT2
        ecall

# move_cursor_checks()
# The clamp table for the current n (= s1, last = s1 - 1), starting from
# (0, 0). Each step: (drow, dcol) -> want moved, want row, want col.
# Uses: a0-a5, t0-t1.  Calls: move_cursor, check.  Saves: ra, s8.
move_cursor_checks:
        addi    sp, sp, -16
        sw      ra, 12(sp)
        sw      s8, 8(sp)
        addi    s8, s1, -1              # s8 = last index (kept across calls)
        # (drow, dcol, moved, row, col) rows, with "last" = s8
        li      a0, -1
        li      a1, 0
        li      a2, 0
        li      a3, 0
        li      a4, 0
        jal     move_step
        li      a0, 0
        li      a1, -1
        li      a2, 0
        li      a3, 0
        li      a4, 0
        jal     move_step
        li      a0, 1
        li      a1, 0
        li      a2, 1
        li      a3, 1
        li      a4, 0
        jal     move_step
        li      a0, 0
        li      a1, 1
        li      a2, 1
        li      a3, 1
        li      a4, 1
        jal     move_step
        li      a0, 100
        li      a1, 100
        li      a2, 1
        mv      a3, s8
        mv      a4, s8
        jal     move_step
        li      a0, 1
        li      a1, 0
        li      a2, 0
        mv      a3, s8
        mv      a4, s8
        jal     move_step
        li      a0, 0
        li      a1, 1
        li      a2, 0
        mv      a3, s8
        mv      a4, s8
        jal     move_step
        li      a0, -100
        li      a1, -100
        li      a2, 1
        li      a3, 0
        li      a4, 0
        jal     move_step
        lw      ra, 12(sp)
        lw      s8, 8(sp)
        addi    sp, sp, 16
        ret

# move_step(a0 = drow, a1 = dcol, a2 = want moved, a3 = want row,
#           a4 = want col): calls move_cursor and checks the result.
# Uses: a0-a5, t0-t1.  Calls: move_cursor, check.  Saves: ra (wants kept on the stack).
move_step:
        addi    sp, sp, -32
        sw      ra, 28(sp)
        sw      a2, 24(sp)              # want moved
        sw      a3, 20(sp)              # want row
        sw      a4, 16(sp)              # want col
        jal     move_cursor
        lw      a1, 24(sp)
        la      a2, l_move
        li      a3, 0
        jal     check                   # moved flag
        mv      a0, s3
        lw      a1, 20(sp)
        la      a2, l_move
        li      a3, 0
        jal     check                   # row
        mv      a0, s4
        lw      a1, 16(sp)
        la      a2, l_move
        li      a3, 0
        jal     check                   # col
        lw      ra, 28(sp)
        addi    sp, sp, 32
        ret

