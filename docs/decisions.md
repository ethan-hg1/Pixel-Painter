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
| 6 | Constants (7.1) | No row-size constant | **Added** `DISPLAY_ROW_BYTES = 512` (`DISPLAY_W * 4`) to both `config.h` and `constants.asm`. RARS `.eqv` is plain text substitution and cannot evaluate `DISPLAY_W * 4`, so the asm needs the value spelled out. |
