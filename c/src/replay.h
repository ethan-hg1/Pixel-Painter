/* replay.h - headless mode, BUILD_INSTRUCTIONS.txt 12.2.
 * No raylib: runs a key script through app.c and writes the PNG. */
#ifndef PP_REPLAY_H
#define PP_REPLAY_H

/* Returns a process exit code: 0 on success. */
int replay_run(const char *script_path, const char *out_path);

#endif /* PP_REPLAY_H */
