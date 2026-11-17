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

    /* ctrl (Cmd on macOS) combos: clear and resize (sections 2.7, 2.8).
     * They return early so ctrl+arrow, ctrl+w etc. do nothing else. */
    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL) ||
        IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER)) {
        if (IsKeyPressed(KEY_DELETE)) {
            do_clear(app);
        }
        if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) {
            do_resize(app, RESIZE_STEP);
        }
        if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) {
            do_resize(app, -RESIZE_STEP);
        }
        return;
    }

    /* Arrows repeat while held; paint and erase do not (section 3). */
    if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) {
        move_cursor(app, -1, 0);
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) {
        move_cursor(app, 1, 0);
    }
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
        move_cursor(app, 0, -1);
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
        move_cursor(app, 0, 1);
    }
    if (IsKeyPressed(KEY_W)) {
        do_paint(app);
    }
    if (IsKeyPressed(KEY_E)) {
        do_erase(app);
    }
    if (IsKeyPressed(KEY_T)) {
        entry_start(app);
        while (GetCharPressed() != 0) {
            /* drop the 't' itself so it is not typed into the buffer */
        }
    }
}

/* Mouse (section 2.2, 2.3): a press on a cell moves the cursor there, then
 * left paints or right erases. Presses outside the cells do nothing. */
static void handle_mouse(AppState *app)
{
    int left = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    int right = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    Vector2 p;
    int row, col;

    if (!left && !right) {
        return;
    }
    p = GetMousePosition();
    if (!grid_cell_at((int)p.x, (int)p.y, CANVAS_X, CANVAS_Y, CANVAS_PX,
                      app->n, &row, &col)) {
        return;
    }
    app->cur_row = row;
    app->cur_col = col;
    if (left) {
        do_paint(app);
    } else {
        do_erase(app);
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
        if (!app.entry_mode) {
            handle_mouse(&app);
        }

        BeginDrawing();
        render_frame(&app);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
