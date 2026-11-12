# main.asm - entry point `main`, init, main loop.
# Assemble with "Assemble all files in directory" on (see README.md).
#
# Global registers (section 8), set by app_init and never repurposed:
#   s0 = GRID_BASE        s4 = cursor col
#   s1 = n                s5 = cell size in units
#   s2 = current color    s6 = canvas offset in units
#   s3 = cursor row       s7 = DISPLAY_BASE

.include "constants.asm"

        .globl  main
        .text

# main()
# Sets up the state, draws everything once, then polls the keyboard.
# Sleeps 10 ms whenever no key is waiting. Exits on q.
# Uses: a0, a7.  Calls: app_init, draw_background, draw_canvas,
# draw_cursor, read_key, handle_key.  Saves: nothing (never returns).
main:
        jal     app_init
        jal     draw_background
        jal     draw_canvas
        jal     draw_cursor
main_loop:
        jal     read_key
        bltz    a0, main_idle
        jal     handle_key
        bnez    a0, main_exit
        j       main_loop
main_idle:
        li      a0, 10
        li      a7, SYS_SLEEP
        ecall
        j       main_loop
main_exit:
        li      a0, 0
        li      a7, SYS_EXIT
        ecall
