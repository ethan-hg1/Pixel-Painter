# hexparse.asm - parse_hex. Translation of c/src/hexparse.c.
# Valid input: optional '#', then exactly 6 hex digits (0-9, a-f, A-F),
# then the terminating 0 byte. Nothing else (BUILD_INSTRUCTIONS.txt 2.4).

.include "constants.asm"

        .text
        .globl  hex_digit_value, parse_hex

# hex_digit_value(a0 = char) -> a0 = 0..15, or -1 if not a hex digit
# Uses: t0.  Calls: none (leaf).  Saves: nothing.
hex_digit_value:
        li      t0, '0'
        blt     a0, t0, hex_digit_bad
        li      t0, '9'
        ble     a0, t0, hex_digit_num
        li      t0, 'A'
        blt     a0, t0, hex_digit_bad
        li      t0, 'F'
        ble     a0, t0, hex_digit_upper
        li      t0, 'a'
        blt     a0, t0, hex_digit_bad
        li      t0, 'f'
        bgt     a0, t0, hex_digit_bad
        addi    a0, a0, -87             # 'a' (97) -> 10
        ret
hex_digit_upper:
        addi    a0, a0, -55             # 'A' (65) -> 10
        ret
hex_digit_num:
        addi    a0, a0, -48             # '0' (48) -> 0
        ret
hex_digit_bad:
        li      a0, -1
        ret

# parse_hex(a0 = address of 0-terminated string)
#   -> a0 = 1 if valid, 0 if not; a1 = color 0x00RRGGBB (only if valid)
# Uses: a0, a1, t0.  Calls: hex_digit_value.  Saves: ra, s8, s9, s10.
parse_hex:
        addi    sp, sp, -16
        sw      ra, 12(sp)
        sw      s8, 8(sp)
        sw      s9, 4(sp)
        sw      s10, 0(sp)
        mv      s8, a0                  # s8 = current char address
        li      s9, 0                   # s9 = value
        li      s10, 0                  # s10 = digits so far

        lbu     t0, 0(s8)
        li      a1, '#'
        bne     t0, a1, parse_hex_loop
        addi    s8, s8, 1               # skip the optional '#'
parse_hex_loop:
        lbu     a0, 0(s8)
        beqz    a0, parse_hex_end
        jal     hex_digit_value
        bltz    a0, parse_hex_bad       # not a hex digit
        li      t0, 6
        beq     s10, t0, parse_hex_bad  # a 7th digit
        slli    s9, s9, 4
        or      s9, s9, a0              # value = (value << 4) | digit
        addi    s10, s10, 1
        addi    s8, s8, 1
        j       parse_hex_loop
parse_hex_end:
        li      t0, 6
        bne     s10, t0, parse_hex_bad  # fewer than 6 digits
        mv      a1, s9
        li      a0, 1
        j       parse_hex_ret
parse_hex_bad:
        li      a0, 0
parse_hex_ret:
        lw      s10, 0(sp)
        lw      s9, 4(sp)
        lw      s8, 8(sp)
        lw      ra, 12(sp)
        addi    sp, sp, 16
        ret
