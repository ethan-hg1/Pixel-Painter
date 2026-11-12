/* replay.c - see replay.h. Implemented once app.c and export_png.c exist. */
#include "replay.h"

#include <stdio.h>

int replay_run(const char *script_path, const char *out_path)
{
    fprintf(stderr, "replay: not implemented yet (script %s, out %s)\n",
            script_path, out_path);
    return 1;
}
