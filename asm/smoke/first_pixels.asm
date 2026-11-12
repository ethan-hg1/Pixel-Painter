# first_pixels.asm - Week 1 smoke test, assembled on its own (not in
# project mode with asm/). Will draw one pixel and one 8x8 block using the
# display formula: addr = DISPLAY_BASE + ((y << 7) + x) << 2.

.include "../constants.asm"

        .globl main
        .text

# main()
# Placeholder until the First Pixels task: exits cleanly.
main:
        li      a0, 0
        li      a7, SYS_EXIT
        ecall
