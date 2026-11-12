# Keyboard findings (RARS Keyboard MMIO)

What byte arrives at `KBD_DATA` (0xFFFF0004) for each key, and what that
means for `asm/input.asm`. See BUILD_INSTRUCTIONS.txt section 3.

## Test setup

- Program: `asm/smoke/key_test.asm` (prints one line per received byte).
- Machine: dev Codespace, Linux, OpenJDK 25.0.5, RARS 1.6, on a virtual
  display (Xvfb). Keys were sent with `xdotool`, each followed by `.` (0x2E)
  as a separator, so a key that sends nothing shows up as two separators in
  a row.
- Raw output: `evidence/week1_key_test_codespace.txt`. Screenshot of an
  earlier run: `evidence/week1_key_test_codespace.png`.

These are synthetic key events from one Linux machine. Other OS/Java
combinations may differ (macOS in particular often sends 0x7F for
Backspace), which is why the rules below accept every known variant.

## Results

| Key | Byte received | Notes |
|-----|---------------|-------|
| a | 0x61 | |
| shift+z (`Z`) | 0x5A | Shift only changes the character. |
| ctrl+z | 0x1A | |
| ctrl+shift+z | 0x1A | **Same as ctrl+z.** Shift is lost, so redo cannot use it. |
| ctrl+s | 0x13 | |
| Up / Down / Left / Right | none | Arrow keys send nothing. |
| Enter | 0x0A | Line feed, not carriage return (0x0D). |
| Keypad Enter | 0x0A | |
| Esc | 0x1B | |
| Backspace | 0x08 | |
| Delete | 0x7F | |
| ctrl+Delete | 0x7F | **Same as Delete.** Ctrl is lost. |
| + | 0x2B | |
| = | 0x3D | |
| - | 0x2D | |
| Keypad + / Keypad - | 0x2B / 0x2D | Same as the main keys. |
| ctrl+= / ctrl+- | 0x3D / 0x2D | Ctrl is lost; same as plain = / -. |
| Tab | 0x09 | |
| q | 0x71 | |

## Consequences for `input.asm`

1. **Backup keys stay the main controls** (section 3): `i j k l` move,
   `z` undo, `y` redo, `c` clear, `+`/`=` grow, `-` shrink, `s` save, `q` quit.
   Arrows, ctrl+shift+z and ctrl+delete cannot be told apart from other keys.
2. **Also accept the control codes that do arrive:** 0x1A as undo, 0x13 as
   save (as section 3 asks).
3. **Color entry mode:** Enter = 0x0A **or** 0x0D; Backspace = 0x08 **or**
   0x7F; Esc = 0x1B.
4. **Do not map 0x7F to clear** outside entry mode, even though ctrl+delete
   sends it: on machines where Backspace sends 0x7F, a stray Backspace would
   clear the canvas.
5. Fold uppercase to lowercase before dispatch (`Z` → `z` is undo).
