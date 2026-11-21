# main.asm - entry point `main`, init, main loop.
# Assemble with "Assemble all files in directory" on (see README.md).
#
# Global registers, set by app_init and never repurposed:
#   s0 = GRID_BASE        s4 = cursor col
#   s1 = n                s5 = cell size in units
#   s2 = current color    s6 = canvas offset in units
#   s3 = cursor row       s7 = DISPLAY_BASE

.include "constants.asm"

# ---------------------------------------------------------------------------
# Memory map. All global state is declared here, in this order.
# RARS places the main file's .data first, so grid is at 0x10010000.
# ---------------------------------------------------------------------------
        .data
        .align  2
        .globl  grid, n, entry_mode, entry_len, entry_buf
        .globl  palette_next, palette, file_buf
        .globl  hist_records, hist_snaps, hist_start, hist_len, hist_pos
grid:       .space  1024                # MAX_N * MAX_N words, stride 16
n:          .word   START_N             # mirror of s1
entry_mode: .word   0                   # 1 while typing a hex code
entry_len:  .word   0                   # chars in entry_buf
palette_next: .word 0                   # slot the next save overwrites (FIFO)
palette:    .space  32                  # PALETTE_SLOTS words, EMPTY = unused
hist_records: .space 2048               # 64 records x 32 bytes
hist_snaps:   .space 65536              # 64 snapshots x 1 KB (256 words)
hist_start: .word   0                   # ring index of the oldest record
hist_len:   .word   0                   # valid records
hist_pos:   .word   0                   # records currently applied
entry_buf:  .space  8                   # ENTRY_MAX_LEN chars + 0 byte
file_buf:   .space  53248               # PNG being written (export.asm), 52 KB

        .globl  main
        .text

# main()
# Sets up the state, draws everything once, then polls the keyboard.
# Sleeps 10 ms whenever no key is waiting. Exits on q.
# Uses: a0, a7.  Calls: app_init, draw_background, draw_canvas,
# draw_cursor, draw_preview, draw_palette, print_current_color, read_key,
# handle_key.  Saves: nothing (never returns).
main:
        jal     app_init
        jal     draw_background
        jal     draw_canvas
        jal     draw_cursor
        jal     draw_preview
        jal     draw_palette
        jal     print_current_color
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
