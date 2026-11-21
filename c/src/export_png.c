/* export_png.c - hand-written PNG writer, crc32, adler32 (section 11.2).
 * No raylib in this file. The asm version
 * (asm/export.asm) must produce the same bytes. */
#include "export_png.h"

#include <stdio.h>
#include <string.h>

#define ADLER_MOD 65521u

uint32_t crc32_bytes(const unsigned char *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFFu;

    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int k = 0; k < 8; k++) {
            crc = (crc & 1u) ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
        }
    }
    return crc ^ 0xFFFFFFFFu;
}

uint32_t adler32_bytes(const unsigned char *data, size_t len)
{
    uint32_t a = 1, b = 0;

    for (size_t i = 0; i < len; i++) {
        a = (a + data[i]) % ADLER_MOD;
        b = (b + a) % ADLER_MOD;
    }
    return (b << 16) | a;
}

static void put_be32(unsigned char *p, uint32_t v)
{
    p[0] = (unsigned char)(v >> 24);
    p[1] = (unsigned char)(v >> 16);
    p[2] = (unsigned char)(v >> 8);
    p[3] = (unsigned char)v;
}

/* Finishes a chunk whose "type" starts at type_pos and whose data ends at
 * data_end: the CRC covers type + data, written big-endian after it.
 * Returns the position after the CRC. */
static size_t end_chunk(unsigned char *out, size_t type_pos, size_t data_end)
{
    put_be32(out + data_end, crc32_bytes(out + type_pos, data_end - type_pos));
    return data_end + 4;
}

/* Color of image pixel (x, y): cell (y / EXPORT_SCALE, x / EXPORT_SCALE). */
static uint32_t pixel_color(const uint32_t grid[GRID_CELLS], int x, int y)
{
    uint32_t c = grid_get(grid, y / EXPORT_SCALE, x / EXPORT_SCALE);

    return c == EMPTY ? 0x00FFFFFFu : c;
}

size_t export_png_encode(const uint32_t grid[GRID_CELLS], int n,
                         unsigned char *out, size_t cap)
{
    static const unsigned char signature[8] =
        { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
    int w;
    size_t row_bytes, raw_len, idat_len, total, p, type_pos, raw_start, q;

    if (n < MIN_N || n > MAX_N) {
        return 0;
    }
    w = n * EXPORT_SCALE;
    row_bytes = 1 + (size_t)w * 3;          /* filter byte + RGB pixels */
    raw_len = row_bytes * (size_t)w;
    idat_len = 2 + 5 + raw_len + 4;         /* zlib header, stored block, adler */
    total = 8 + (12 + 13) + (12 + idat_len) + 12;
    if (raw_len > 65535u || total > cap) {  /* one stored block holds 65535 */
        return 0;
    }

    memcpy(out, signature, 8);
    p = 8;

    /* IHDR: width, height, bit depth 8, color type 2 (RGB), 0, 0, 0 */
    put_be32(out + p, 13);
    p += 4;
    type_pos = p;
    memcpy(out + p, "IHDR", 4);
    p += 4;
    put_be32(out + p, (uint32_t)w);
    p += 4;
    put_be32(out + p, (uint32_t)w);
    p += 4;
    out[p++] = 8;
    out[p++] = 2;
    out[p++] = 0;
    out[p++] = 0;
    out[p++] = 0;
    p = end_chunk(out, type_pos, p);

    /* IDAT: zlib header 78 01, one final stored block, raw data, adler-32 */
    put_be32(out + p, (uint32_t)idat_len);
    p += 4;
    type_pos = p;
    memcpy(out + p, "IDAT", 4);
    p += 4;
    out[p++] = 0x78;
    out[p++] = 0x01;
    out[p++] = 0x01;                        /* BFINAL = 1, BTYPE = 00 */
    out[p++] = (unsigned char)(raw_len & 0xFFu);          /* LEN, little-endian */
    out[p++] = (unsigned char)((raw_len >> 8) & 0xFFu);
    out[p++] = (unsigned char)(~raw_len & 0xFFu);         /* NLEN */
    out[p++] = (unsigned char)((~raw_len >> 8) & 0xFFu);

    raw_start = p;
    q = p;
    for (int y = 0; y < w; y++) {
        out[q++] = 0;                       /* filter type 0 (None) */
        for (int x = 0; x < w; x++) {
            uint32_t c = pixel_color(grid, x, y);
            out[q++] = (unsigned char)(c >> 16);
            out[q++] = (unsigned char)(c >> 8);
            out[q++] = (unsigned char)c;
        }
    }
    p = q;
    put_be32(out + p, adler32_bytes(out + raw_start, raw_len));
    p += 4;
    p = end_chunk(out, type_pos, p);

    /* IEND */
    put_be32(out + p, 0);
    p += 4;
    type_pos = p;
    memcpy(out + p, "IEND", 4);
    p += 4;
    p = end_chunk(out, type_pos, p);

    return p == total ? p : 0;
}

int export_png_write(const uint32_t grid[GRID_CELLS], int n, const char *path)
{
    static unsigned char buf[PNG_MAX_BYTES];
    size_t len = export_png_encode(grid, n, buf, sizeof buf);
    FILE *f;
    size_t wrote;

    if (len == 0) {
        return 0;
    }
    f = fopen(path, "wb");
    if (f == NULL) {
        return 0;
    }
    wrote = fwrite(buf, 1, len, f);
    if (fclose(f) != 0 || wrote != len) {
        return 0;
    }
    return 1;
}
