/* test_main.c - tiny assert-based test runner. No raylib.
 * Exits 0 when every check passes. Add one test function per module and
 * call it from main(). Required list: BUILD_INSTRUCTIONS.txt 12.5. */
#include <stdio.h>

#include "config.h"

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        g_checks++;                                                       \
        if (!(cond)) {                                                    \
            g_failures++;                                                 \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                 \
    } while (0)

static void test_config(void)
{
    CHECK(MIN_N == 8);
    CHECK(MAX_N == 16);
    CHECK(START_N >= MIN_N && START_N <= MAX_N);
    CHECK(GRID_STRIDE == MAX_N);
    CHECK((HISTORY_DEPTH & (HISTORY_DEPTH - 1)) == 0); /* power of two */
    CHECK(EMPTY != 0x00000000u);                       /* black is a color */
    CHECK(CANVAS_UNITS <= DISPLAY_W && CANVAS_UNITS <= DISPLAY_H);
}

int main(void)
{
    test_config();

    printf("%d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
