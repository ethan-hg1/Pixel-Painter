/* buttons.h - on-screen buttons. The hit test
 * and the actions are pure logic (no raylib); render.c draws the row and
 * main.c calls these on a left click. Each button runs the same function as
 * its shortcut, so undo history works the same way. */
#ifndef PP_BUTTONS_H
#define PP_BUTTONS_H

#include "app.h"

enum {
    BTN_UNDO,
    BTN_REDO,
    BTN_CLEAR,
    BTN_SHRINK,
    BTN_GROW,
    BTN_SAVE,
    BUTTON_COUNT
};

/* One row under the status line: BUTTON_COUNT buttons of BUTTON_W x BUTTON_H
 * px, BUTTON_GAP apart, the first at (BUTTON_X, BUTTON_Y). */
#define BUTTON_X    30
#define BUTTON_Y    712
#define BUTTON_W    120
#define BUTTON_H    36
#define BUTTON_GAP  8

/* Label shown on the button. */
const char *button_label(int which);

/* Sets *which and returns 1 if (px, py) is on a button, else returns 0. */
int button_at(int px, int py, int *which);

/* Runs the action for undo, redo, clear, shrink and grow. Returns what the
 * shortcut returns (1 if something changed). BTN_SAVE is not handled here:
 * the window picks the file path, so main.c saves (do_save). Returns 0 for
 * BTN_SAVE. */
int button_action(AppState *app, int which);

#endif /* PP_BUTTONS_H */
