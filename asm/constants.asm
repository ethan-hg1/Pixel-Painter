# constants.asm - every constant, BUILD_INSTRUCTIONS.txt section 7.1.
# Must match c/src/config.h value for value.
# Only .eqv lines here: every other file pulls this in with
#   .include "constants.asm"

.eqv MIN_N          8
.eqv MAX_N          16
.eqv START_N        8
.eqv RESIZE_STEP    1
.eqv GRID_STRIDE    16          # always MAX_N, see 7.2
.eqv EMPTY          0xFFFFFFFF  # black 0x00000000 is a real color
.eqv HISTORY_DEPTH  64          # power of two: ring index = i & 63
.eqv PALETTE_SLOTS  8
.eqv ENTRY_MAX_LEN  7           # '#' + 6 hex digits; buffer is 8 bytes
.eqv EXPORT_SCALE   8           # image pixels per cell
.eqv CANVAS_UNITS   96          # canvas area, 96 x 96 units
.eqv DISPLAY_W      128         # display width in units
.eqv DISPLAY_H      128
.eqv DISPLAY_ROW_BYTES 512     # DISPLAY_W * 4 (RARS .eqv has no arithmetic)
.eqv BG_COLOR       0x00303030  # background (7.5)
.eqv EMPTY_LIGHT    0x00DCDCDC  # EMPTY cells: checker by (row + col) & 1
.eqv EMPTY_DARK     0x00C4C4C4
.eqv DISPLAY_BASE   0x10040000
.eqv KBD_CTRL       0xFFFF0000  # receiver control, bit 0 = ready
.eqv KBD_DATA       0xFFFF0004  # received byte

# asm display layout (section 7.5)
.eqv PREVIEW_X      100         # preview swatch: 24 x 24 units incl. border
.eqv PREVIEW_Y      2
.eqv PREVIEW_SIZE   24
.eqv PREVIEW_INNER  22          # PREVIEW_SIZE - 2 (1-unit border each side)
.eqv FRAME_COLOR    0x00E0E0E0  # swatch border (same as the C frame)

# Syscall numbers (section 8)
.eqv SYS_PRINT_INT      1
.eqv SYS_PRINT_STRING   4
.eqv SYS_EXIT           10
.eqv SYS_EXIT2          93          # exit with code a0
.eqv SYS_PRINT_CHAR     11
.eqv SYS_SLEEP          32
.eqv SYS_PRINT_INT_HEX  34
.eqv SYS_CLOSE          57
.eqv SYS_WRITE          64
.eqv SYS_OPEN           1024
