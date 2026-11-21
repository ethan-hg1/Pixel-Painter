/* replay.h - headless mode, BUILD_INSTRUCTIONS.txt 12.2.
 * No raylib: runs a key script through app.c and writes the PNG. */
#ifndef PP_REPLAY_H
#define PP_REPLAY_H

#include <stddef.h>

#include "app.h"

/* Runs the script (one token per line, see replay_apply_line), then writes
 * the PNG to out_path. Returns a process exit code: 0 on success. An error
 * is printed as "script:line: message". */
int replay_run(const char *script_path, const char *out_path);

/* Applies one script line to app. Blank lines and lines starting with '#'
 * are ignored. Tokens are the asm key names (decision 3): w e i j k l z y
 * + = - c p 1..8 s (save PNG to out_path), uppercase folds to lowercase,
 * and "t #RRGGBB" types a color (Enter included). q ends the script (asm q
 * quits). Returns 0 when applied, 2 on q, 1 on an unknown token or a bad
 * color, with the reason in err. */
int replay_apply_line(AppState *app, const char *line, const char *out_path,
                      char *err, size_t err_len);

#endif /* PP_REPLAY_H */
