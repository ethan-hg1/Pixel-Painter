# Decisions and deviations from BUILD_INSTRUCTIONS.txt

BUILD_INSTRUCTIONS.txt is the source of truth. This file records where the
team settled an ASSUMPTION or chose to deviate from it. Every PR that relies
on one of these should mention it.

| # | Topic | Instructions say | Decision |
|---|-------|------------------|----------|
| 1 | Resize step (2.8, 7.1) | ASSUMPTION: `RESIZE_STEP = 1` | Confirmed: `RESIZE_STEP = 1`, sizes 8..16. Still a single constant. |
| 2 | Quit in C (3) | Esc quits outside color entry | **Deviation:** Esc only cancels color entry. The C app quits only when the window is closed (`SetExitKey(KEY_NULL)`, since raylib quits on Esc by default). asm `q` still quits. |
| 3 | Replay tokens (12.2) | Lists w e i j k l z y + - c p 1-8 and `t #RRGGBB` | Replay accepts every asm key with the asm meaning: `=` is `+`, uppercase folds to lowercase, `c` clears, `s` writes the PNG to `--out`. The PNG is also written when the script ends. An unknown token is an error that reports the line number. |
| 4 | RARS check (12.3, 12.4) | `java -jar rars1_6.jar a nc p asm/main.asm`, optional in CI | **Fix:** RARS 1.6 exits 0 on assembly errors unless `ae<n>` is given. Use `a nc ae1 p`. The check is **required** in CI and also assembles `asm/smoke/first_pixels.asm` on its own. |
| 5 | first_pixels.asm location (4.2) | `asm/first_pixels.asm` | `asm/smoke/first_pixels.asm` (as 12.3 suggests), so project mode does not see a second `main`. Other stand-alone programs (`key_test.asm`) go there too. |
| 6 | Constants (7.1) | No row-size constant | **Added** `DISPLAY_ROW_BYTES = 512` (`DISPLAY_W * 4`) to both `config.h` and `constants.asm`. RARS `.eqv` is plain text substitution and cannot evaluate `DISPLAY_W * 4`, so the asm needs the value spelled out. Also **added** the display colors `BG_COLOR` (0x303030, from 7.5), `EMPTY_LIGHT` (0xDCDCDC) and `EMPTY_DARK` (0xC4C4C4) so the C window and the asm display draw EMPTY cells identically. |
| 7 | asm file list (4.2) | No `app.asm` | **Added** `asm/app.asm` as the counterpart of `c/src/app.c` (`app_init`, `do_paint`, and later every `do_*` action), so each C file maps onto one asm file with the same function names. |
| 8 | Luminance/cursor color location | Not specified | `luminance_q8` and `cursor_color` live in `grid.c` / `grid.asm` (pure functions, unit-tested without raylib). |
| 9 | Color entry details (2.4) | Not specified | The entry buffer takes any printable ASCII character (0x20..0x7E), up to 7 (`ENTRY_MAX_LEN`, buffer 8 bytes per 7.3); `parse_hex` decides validity on Enter, so `#12345G` can be typed and rejected. Enter always leaves entry mode, valid or not. "Invalid color" stays on screen until the next `t`. |
| 10 | Where `.data` lives (9) | "in this order" | All global state is declared in `asm/main.asm`, in section 9 order. RARS places the main file's `.data` first and every other file's after it in alphabetical order, so this is the only way to pin `grid` at 0x10010000. Modules reference the labels via `.globl`; only constant strings stay in the module that prints them. |
| 11 | asm unit tests (12) | Only manual asm testing | **Added** `asm/tests/test_main.asm`: headless checks of the pure asm functions (same cases as the C tests), run in CI. The modules are passed as extra files on the command line, because RARS refuses to `.include` the same file (`constants.asm`) twice in one assembly. Exit code = failed checks. |
