/* export_png.h - hand-written PNG writer, crc32, adler32.
 * No raylib. The image is n * EXPORT_SCALE pixels square, RGB, one zlib
 * "stored" deflate block (no compression), so no zlib is needed. */
#ifndef PP_EXPORT_PNG_H
#define PP_EXPORT_PNG_H

#include <stddef.h>
#include <stdint.h>

#include "config.h"
#include "grid.h"

/* Largest file the encoder can make: a MAX_N grid at EXPORT_SCALE.
 * 8 signature + IHDR (12 + 13) + IDAT (12 + 2 + 5 + raw + 4) + IEND (12),
 * where raw = w * (1 + 3w) and w = MAX_N * EXPORT_SCALE = 128. */
#define PNG_W_MAX    (MAX_N * EXPORT_SCALE)
#define PNG_RAW_MAX  (PNG_W_MAX * (1 + PNG_W_MAX * 3))
#define PNG_MAX_BYTES (8 + 25 + (12 + 2 + 5 + PNG_RAW_MAX + 4) + 12)

/* CRC-32 (reflected 0xEDB88320, bitwise). crc32_bytes("IEND") == 0xAE426082. */
uint32_t crc32_bytes(const unsigned char *data, size_t len);

/* Adler-32. adler32_bytes("Wikipedia") == 0x11E60398. */
uint32_t adler32_bytes(const unsigned char *data, size_t len);

/* Writes the whole PNG file for the n x n grid into out (cap bytes). Returns
 * the number of bytes written, or 0 if n is out of range or out is too small.
 * EMPTY cells are white. */
size_t export_png_encode(const uint32_t grid[GRID_CELLS], int n,
                         unsigned char *out, size_t cap);

/* Encodes and writes the file at path. Returns 1 on success, 0 on failure
 * (bad n, file not writable, e.g. its folder does not exist). */
int export_png_write(const uint32_t grid[GRID_CELLS], int n, const char *path);

#endif /* PP_EXPORT_PNG_H */
