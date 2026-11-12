# RARS setup

Tested with RARS 1.6 (`rars1_6.jar`), Java TODO.

1. `java -jar rars1_6.jar`
2. Settings: enable **Assemble all files in directory** and
   **Initialize Program Counter to global 'main' if defined**.
   Screenshot: TODO
3. File > Open `asm/main.asm`.
4. Tools > Bitmap Display: Unit Width 4, Unit Height 4, Display Width 512,
   Display Height 512, Base address `0x10040000 (heap)`, **Connect to Program**.
   Screenshot: TODO
5. Tools > Keyboard and Display MMIO Simulator: **Connect to Program**. Click
   in the lower (keyboard) text box before typing.
   Screenshot: TODO
6. Assemble (F3), Run (F5), run speed at max.

## Command line (assemble only, used in CI)

```sh
java -jar rars1_6.jar a nc ae1 p asm/main.asm
java -jar rars1_6.jar a nc ae1 asm/smoke/first_pixels.asm
```

`ae1` is required: without it RARS exits 0 even when assembly fails.
