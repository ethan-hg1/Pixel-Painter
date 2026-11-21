/* export_bmp.c - 24-bit BMP writer, fallback (section 11.3).
 * No raylib in this file (BUILD_INSTRUCTIONS.txt 4.2). */
#include "export_bmp.h"

#include <stdio.h>
#include <string.h>

static void put_le32(unsigned char *p, uint32_t v)
{
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
    p[2] = (unsigned char)(v >> 16);
    p[3] = (unsigned char)(v >> 24);
}

static void put_le16(unsigned char *p, uint16_t v)
{
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
}

size_t export_bmp_encode(const uint32_t grid[GRID_CELLS], int n,
                         unsigned char *out, size_t cap)
{
    uint32_t w, row, pixels, file_size;
    size_t q = 54;

    if (n < MIN_N || n > MAX_N) {
        return 0;
    }
    w = (uint32_t)(n * EXPORT_SCALE);
    row = w * 3;                            /* multiple of 4: no padding */
    pixels = row * w;
    file_size = 54 + pixels;
    if (file_size > cap) {
        return 0;
    }

    memset(out, 0, 54);
    out[0] = 'B';
    out[1] = 'M';
    put_le32(out + 2, file_size);
    put_le32(out + 6, 0);                   /* reserved */
    put_le32(out + 10, 54);                 /* pixel data offset */
    put_le32(out + 14, 40);                 /* BITMAPINFOHEADER size */
    put_le32(out + 18, w);
    put_le32(out + 22, w);                  /* positive height: bottom-up */
    put_le16(out + 26, 1);                  /* planes */
    put_le16(out + 28, 24);                 /* bits per pixel */
    put_le32(out + 30, 0);                  /* BI_RGB, no compression */
    put_le32(out + 34, pixels);
    put_le32(out + 38, 2835);               /* ~72 dpi, horizontal */
    put_le32(out + 42, 2835);               /* ~72 dpi, vertical */
    put_le32(out + 46, 0);
    put_le32(out + 50, 0);

    /* rows from the bottom of the image (y = w - 1) up to the top */
    for (int r = 0; r < (int)w; r++) {
        int y = (int)w - 1 - r;
        for (int x = 0; x < (int)w; x++) {
            uint32_t c = grid_get(grid, y / EXPORT_SCALE, x / EXPORT_SCALE);
            if (c == EMPTY) {
                c = 0x00FFFFFFu;
            }
            out[q++] = (unsigned char)c;          /* B */
            out[q++] = (unsigned char)(c >> 8);   /* G */
            out[q++] = (unsigned char)(c >> 16);  /* R */
        }
    }
    return q;
}

int export_bmp_write(const uint32_t grid[GRID_CELLS], int n, const char *path)
{
    static unsigned char buf[BMP_MAX_BYTES];
    size_t len = export_bmp_encode(grid, n, buf, sizeof buf);
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
