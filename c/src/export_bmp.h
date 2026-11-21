/* export_bmp.h - 24-bit BMP writer, fallback. No raylib.
 * Same image as export_png (n * EXPORT_SCALE square, EMPTY white). The
 * width is a multiple of 8, so each row is already 4-byte aligned. */
#ifndef PP_EXPORT_BMP_H
#define PP_EXPORT_BMP_H

#include <stddef.h>
#include <stdint.h>

#include "config.h"
#include "grid.h"

#define BMP_W_MAX     (MAX_N * EXPORT_SCALE)
#define BMP_MAX_BYTES (54 + BMP_W_MAX * BMP_W_MAX * 3)

/* Writes the whole BMP file into out (cap bytes). Returns the number of
 * bytes, or 0 if n is out of range or out is too small. */
size_t export_bmp_encode(const uint32_t grid[GRID_CELLS], int n,
                         unsigned char *out, size_t cap);

/* Encodes and writes the file at path. Returns 1 on success. */
int export_bmp_write(const uint32_t grid[GRID_CELLS], int n, const char *path);

#endif /* PP_EXPORT_BMP_H */
