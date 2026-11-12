/* hexparse.c - see hexparse.h. No raylib in this file. */
#include "hexparse.h"

int hex_digit_value(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

int parse_hex(const char *s, uint32_t *out)
{
    uint32_t value = 0;
    int digits = 0;

    if (*s == '#') {
        s++;
    }
    for (; *s != '\0'; s++) {
        int d = hex_digit_value(*s);
        if (d < 0 || digits == 6) {
            return 0;               /* bad character, or a 7th digit */
        }
        value = (value << 4) | (uint32_t)d;
        digits++;
    }
    if (digits != 6) {
        return 0;
    }
    *out = value;
    return 1;
}
