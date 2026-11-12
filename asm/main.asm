# main.asm - entry point `main`, init, main loop.
# Assemble with "Assemble all files in directory" on (see README.md).
# Global registers s0-s7 are listed in BUILD_INSTRUCTIONS.txt section 8.

.include "constants.asm"

        .globl main
        .text

# main()
# Placeholder until build step 1: exits cleanly.
main:
        li      a0, 0
        li      a7, SYS_EXIT
        ecall
