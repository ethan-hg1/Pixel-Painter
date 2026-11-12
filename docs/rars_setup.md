# RARS setup

Tested with RARS 1.6 (`rars1_6.jar`), Java TODO.

1. `java -jar rars1_6.jar`
2. Settings: enable **Assemble all files in directory** and
   **Initialize Program Counter to global 'main' if defined**.
   Screenshot: TODO
3. File > Open `asm/main.asm`.
4. Tools > Bitmap Display: Unit Width 4, Unit Height 4, Display Width 512,
   Display Height 512, Base address `0x10040000 (heap)`, **Connect to Program**.
   RARS opens this tool with the wrong defaults (unit 1x1, 512x256, base
   `0x10010000 (static data)`), so all five settings must be changed every
   time the tool is opened.
   Screenshot (settings + First Pixels output): `evidence/week1_first_pixels_rars.png`
5. Tools > Keyboard and Display MMIO Simulator: **Connect to Program**. Click
   in the lower (keyboard) text box before typing.
   Screenshot: TODO
6. Assemble (F3), Run (F5), run speed at max.

## Smoke tests (`asm/smoke/`)

Each file has its own `main`, so open it on its own and turn **Assemble all
files in directory** OFF while running it.

| File | What it does |
|------|--------------|
| `first_pixels.asm` | Red unit at (4, 4), orange 8x8 block at units 16..23. Needs the Bitmap Display (step 4). |
| `key_test.asm` | Prints every byte the Keyboard MMIO delivers. Needs the MMIO simulator (step 5). See `keyboard_findings.md`. |

## Command line (assemble only, used in CI)

```sh
java -jar rars1_6.jar a nc ae1 p asm/main.asm
java -jar rars1_6.jar a nc ae1 asm/smoke/first_pixels.asm
java -jar rars1_6.jar a nc ae1 asm/smoke/key_test.asm
```

`ae1` is required: without it RARS exits 0 even when assembly fails.

## asm unit tests (headless, used in CI)

```sh
java -jar rars1_6.jar nc sm se2 ae1 asm/tests/test_main.asm \
    asm/grid.asm asm/hexparse.asm asm/app.asm
```

Prints `<n> checks, <f> failed` and exits with `<f>` (0 = all passed). The
modules are passed as extra files rather than `.include`d, because RARS
refuses to include `constants.asm` twice in one assembly. `sm` makes it
start at the test's `main`.
