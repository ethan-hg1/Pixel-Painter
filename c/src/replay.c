/* replay.c - headless replay of a key script.
 * No raylib: every token goes through the same app.c actions the window
 * uses, so the PNG it writes is the reference for the asm version. */
#include "replay.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "export_png.h"
#include "grid.h"

static int is_blank(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

/* "t #RRGGBB": the rest of the line is typed into the entry buffer, then
 * Enter confirms it. A bad or too long color is an error (the script is
 * wrong), so the color stays as it was. */
static int apply_color_token(AppState *app, const char *p, char *err, size_t err_len)
{
    entry_start(app);
    while (is_blank(*p)) {
        p++;
    }
    while (*p != '\0' && !is_blank(*p)) {
        if (!entry_add_char(app, (unsigned char)*p)) {
            entry_cancel(app);
            snprintf(err, err_len, "bad or too long color");
            return 1;
        }
        p++;
    }
    if (!entry_confirm(app)) {
        snprintf(err, err_len, "invalid color");
        return 1;
    }
    return 0;
}

int replay_apply_line(AppState *app, const char *line, const char *out_path,
                      char *err, size_t err_len)
{
    const char *p = line;
    int key;

    while (is_blank(*p)) {
        p++;
    }
    if (*p == '\0' || *p == '#') {
        return 0;
    }
    key = (unsigned char)*p;
    if (key >= 'A' && key <= 'Z') {
        key += 'a' - 'A';
    }
    p++;

    if (key == 't') {
        return apply_color_token(app, p, err, err_len);
    }
    while (is_blank(*p)) {
        p++;
    }
    if (*p != '\0' && *p != '#') {
        snprintf(err, err_len, "unexpected text after '%c'", key);
        return 1;
    }

    switch (key) {
    case 'q': return 2;                     /* quit: the rest is not replayed */
    case 'w': do_paint(app); break;
    case 'e': do_erase(app); break;
    case 'i': move_cursor(app, -1, 0); break;
    case 'k': move_cursor(app, 1, 0); break;
    case 'j': move_cursor(app, 0, -1); break;
    case 'l': move_cursor(app, 0, 1); break;
    case 'z': do_undo(app); break;
    case 'y': do_redo(app); break;
    case '+':
    case '=': do_resize(app, RESIZE_STEP); break;
    case '-': do_resize(app, -RESIZE_STEP); break;
    case 'c': do_clear(app); break;
    case 'p': palette_add(app); break;
    case 's':
        if (out_path == NULL) {
            snprintf(err, err_len, "s needs --out");
            return 1;
        }
        if (!do_save(app, out_path)) {
            snprintf(err, err_len, "cannot write %s", out_path);
            return 1;
        }
        break;
    default:
        if (key >= '1' && key <= '8') {
            palette_pick(app, key - '1');
            break;
        }
        snprintf(err, err_len, "unknown token '%c'", key);
        return 1;
    }
    return 0;
}

int replay_run(const char *script_path, const char *out_path)
{
    static AppState app;   /* static: the history snapshots are 64 KB */
    char line[256];
    char err[96];
    int lineno = 0;
    FILE *f = fopen(script_path, "r");

    if (f == NULL) {
        fprintf(stderr, "replay: cannot open %s\n", script_path);
        return 1;
    }
    app_init(&app);
    while (fgets(line, sizeof line, f) != NULL) {
        lineno++;
        int rc = replay_apply_line(&app, line, out_path, err, sizeof err);
        if (rc == 2) {
            break;
        }
        if (rc != 0) {
            fprintf(stderr, "%s:%d: %s\n", script_path, lineno, err);
            fclose(f);
            return 1;
        }
    }
    fclose(f);

    if (!export_png_write(app.grid, app.n, out_path)) {
        fprintf(stderr, "replay: cannot write %s\n", out_path);
        return 1;
    }
    return 0;
}
