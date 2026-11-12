/* main.c - window, main loop, input dispatch. */
#include <stdio.h>
#include <string.h>

#include "app.h"
#include "raylib.h"
#include "render.h"
#include "replay.h"

/* Routes this frame's keys to the app actions (BUILD_INSTRUCTIONS.txt 3).
 * In color entry mode every other shortcut is ignored (2.4). */
static void handle_input(AppState *app)
{
    if (app->entry_mode) {
        int c;
        while ((c = GetCharPressed()) != 0) {
            entry_add_char(app, c);
        }
        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
            entry_backspace(app);
        }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            entry_confirm(app);
        } else if (IsKeyPressed(KEY_ESCAPE)) {
            entry_cancel(app);
        }
        return;
    }

    if (IsKeyPressed(KEY_W)) {
        do_paint(app);
    }
    if (IsKeyPressed(KEY_T)) {
        entry_start(app);
        while (GetCharPressed() != 0) {
            /* drop the 't' itself so it is not typed into the buffer */
        }
    }
}

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
        handle_input(&app);

        BeginDrawing();
        render_frame(&app);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
