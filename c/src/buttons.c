/* buttons.c - on-screen buttons, see buttons.h. No raylib in this file. */
#include "buttons.h"

static const char *const labels[BUTTON_COUNT] = {
    "Undo", "Redo", "Clear", "Resize -", "Resize +", "Save PNG"
};

const char *button_label(int which)
{
    return (which >= 0 && which < BUTTON_COUNT) ? labels[which] : "";
}

int button_at(int px, int py, int *which)
{
    for (int i = 0; i < BUTTON_COUNT; i++) {
        int x = BUTTON_X + i * (BUTTON_W + BUTTON_GAP);

        if (px >= x && px < x + BUTTON_W && py >= BUTTON_Y &&
            py < BUTTON_Y + BUTTON_H) {
            *which = i;
            return 1;
        }
    }
    return 0;
}

int button_action(AppState *app, int which)
{
    switch (which) {
    case BTN_UNDO:   return do_undo(app);
    case BTN_REDO:   return do_redo(app);
    case BTN_CLEAR:  return do_clear(app);
    case BTN_SHRINK: return do_resize(app, -RESIZE_STEP);
    case BTN_GROW:   return do_resize(app, RESIZE_STEP);
    default:         return 0;
    }
}
