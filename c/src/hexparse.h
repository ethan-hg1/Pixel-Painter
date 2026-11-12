/* hexparse.h - parse_hex(): optional '#' + exactly 6 hex digits.
 * BUILD_INSTRUCTIONS.txt 2.4. Maps onto asm/hexparse.asm. No raylib. */
#ifndef PP_HEXPARSE_H
#define PP_HEXPARSE_H

#include <stdint.h>

/* Parses a NUL-terminated string. Valid: optional '#', then exactly 6 hex
 * digits (0-9, a-f, A-F), then the end of the string. Nothing else is
 * allowed (no spaces, no "0x").
 * Returns 1 and stores 0x00RRGGBB in *out if valid; returns 0 and leaves
 * *out unchanged if not. */
int parse_hex(const char *s, uint32_t *out);

/* Value of one hex digit (0..15), or -1 if c is not a hex digit. */
int hex_digit_value(char c);

#endif /* PP_HEXPARSE_H */
