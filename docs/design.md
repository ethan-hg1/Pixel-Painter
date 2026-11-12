# Design (Week 2)

## Pseudocode per action

TODO: do_paint, do_erase, set_color, do_clear, do_resize, do_undo, do_redo,
palette_add, palette_pick, do_save.

## Register allocation

Globals (set once in `main`, never repurposed):

| Register | Holds |
|----------|-------|
| s0 | GRID_BASE |
| s1 | n (mirrored in `.data n`) |
| s2 | current color |
| s3 | cursor row |
| s4 | cursor col |
| s5 | cell size in units |
| s6 | canvas offset in units |
| s7 | DISPLAY_BASE |
| s8-s11 | free for local use (save/restore) |

### Function table

| Function | Args | Returns | Clobbers | Calls | Globals read / written |
|----------|------|---------|----------|-------|------------------------|
| TODO | | | | | |

## Memory map

TODO: fill in from section 9 with real label addresses, and the computed
`.data` total (must stay under 0x30000 bytes).

## Undo record layout

TODO: section 10.
