# key_test.asm - Week 1 keyboard test (BUILD_INSTRUCTIONS.txt section 3).
# Assembled on its own, NOT in project mode with asm/ (it has its own main).
#
# Prints every byte the Keyboard MMIO delivers, so each teammate can fill in
# docs/keyboard_findings.md. One line per byte on the RARS Run I/O console:
#   byte 0x0000001a = 26
# Runs until stopped (Run > Stop).
#
# Setup: Tools > Keyboard and Display MMIO Simulator > Connect to Program,
# then click in its lower text box and press the keys to test.

.include "../constants.asm"

        .data
msg_ready:  .asciz "key_test: type in the MMIO keyboard box. Stop with Run > Stop.\n"
msg_byte:   .asciz "byte "
msg_eq:     .asciz " = "
msg_nl:     .asciz "\n"

        .globl main
        .text

# main()
# Polls KBD_CTRL bit 0; when set, reads KBD_DATA and prints it. Never returns.
# Uses: s0-s2 (main never returns, so nothing to restore), a0, a7.
# Calls: none (syscalls only).
main:
        la      a0, msg_ready
        li      a7, SYS_PRINT_STRING
        ecall
        li      s0, KBD_CTRL
        li      s1, KBD_DATA
poll:
        lw      s2, 0(s0)
        andi    s2, s2, 1
        bnez    s2, got_key
        li      a0, 10                  # sleep 10 ms to keep CPU use low
        li      a7, SYS_SLEEP
        ecall
        j       poll
got_key:
        lw      s2, 0(s1)               # reading data clears the ready bit
        andi    s2, s2, 0xFF

        la      a0, msg_byte
        li      a7, SYS_PRINT_STRING
        ecall
        mv      a0, s2
        li      a7, SYS_PRINT_INT_HEX
        ecall
        la      a0, msg_eq
        li      a7, SYS_PRINT_STRING
        ecall
        mv      a0, s2
        li      a7, SYS_PRINT_INT
        ecall
        la      a0, msg_nl
        li      a7, SYS_PRINT_STRING
        ecall
        j       poll
