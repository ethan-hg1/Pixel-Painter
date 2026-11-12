/* config.h - every constant, BUILD_INSTRUCTIONS.txt section 7.1.
 * Must match asm/constants.asm value for value. */
#ifndef PP_CONFIG_H
#define PP_CONFIG_H

#define MIN_N          8
#define MAX_N          16
#define START_N        8
#define RESIZE_STEP    1
#define GRID_STRIDE    16            /* always MAX_N, see 7.2 */
#define EMPTY          0xFFFFFFFFu   /* black 0x00000000 is a real color */
#define HISTORY_DEPTH  64            /* power of two: ring index = i & 63 */
#define PALETTE_SLOTS  8
#define EXPORT_SCALE   8             /* image pixels per cell */
#define CANVAS_UNITS   96            /* asm canvas area, 96 x 96 units */
#define DISPLAY_W      128           /* asm display width in units */
#define DISPLAY_H      128
#define DISPLAY_ROW_BYTES 512        /* DISPLAY_W * 4; asm .eqv cannot multiply */

/* asm-only addresses, kept here so the two files stay in sync. */
#define DISPLAY_BASE   0x10040000u
#define KBD_CTRL       0xFFFF0000u
#define KBD_DATA       0xFFFF0004u

#endif /* PP_CONFIG_H */
