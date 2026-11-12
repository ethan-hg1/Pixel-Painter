/* main.c - window, main loop, input dispatch. */
#include <stdio.h>
#include <string.h>

#include "app.h"
#include "raylib.h"
#include "render.h"
#include "replay.h"

static int usage(const char *prog)
{
    fprintf(stderr, "usage: %s [--replay SCRIPT.keys --out OUT.png]\n", prog);
    return 2;
}

int main(int argc, char **argv)
{
    const char *script = NULL;
    const char *out = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--replay") == 0 && i + 1 < argc) {
            script = argv[++i];
        } else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) {
            out = argv[++i];
        } else {
            return usage(argv[0]);
        }
    }
    if (script != NULL || out != NULL) {
        if (script == NULL || out == NULL) {
            return usage(argv[0]);
        }
        return replay_run(script, out);
    }

    static AppState app; /* static: ~1 KB grid, keep it off the stack */
    app_init(&app);

    InitWindow(WINDOW_W, WINDOW_H, "Pixel Painter");
    SetExitKey(KEY_NULL); /* Esc must not quit (docs/decisions.md #2) */
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_W)) {
            do_paint(&app);
        }

        BeginDrawing();
        render_frame(&app);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
