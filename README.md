# Pixel Painter

A small pixel-art editor, built twice for CPE 2300:

1. **C** (C11 + [raylib](https://www.raylib.com/) 5.5) with a desktop window. This is the reference implementation.
2. **RISC-V assembly** (RV32IM) running in the [RARS](https://github.com/TheThirdOne/rars) simulator, drawing to the Bitmap Display and reading the Keyboard MMIO.

Pick a grid from 8x8 to 16x16, enter colors as hex codes, paint and erase,
keep a palette of 8 saved colors, undo/redo, and export a PNG.

Team: Ethan, Adrian, Arion.

> Status: scaffolding only. Features land step by step; see
> `BUILD_INSTRUCTIONS.txt` section 6.

## Layout

| Path | Contents |
|------|----------|
| `c/` | C version (CMake project, sources in `c/src`, tests in `c/tests`) |
| `asm/` | RISC-V version (`main.asm` + one file per module) |
| `asm/smoke/` | Stand-alone RARS smoke tests (own `main`) |
| `docs/` | Plan, design, RARS setup, findings, report, decisions |
| `tests/` | Replay key scripts, golden images, manual test plan |
| `BUILD_INSTRUCTIONS.txt` | Full spec and build plan |

## C version

### Requirements

- CMake 3.16+ and a C11 compiler
- Linux: raylib's X11/OpenGL dev packages, for example on Ubuntu:

  ```sh
  sudo apt install build-essential cmake libx11-dev libxrandr-dev \
      libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev
  ```

raylib is downloaded by CMake (FetchContent), so there is nothing else to install.

### Build and run

```sh
cd c
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/pixel_painter          # Windows: build\Release\pixel_painter.exe
./build/pp_tests               # exits 0 when all tests pass
```

Headless replay (no window): `./build/pixel_painter --replay ../tests/scripts/demo.keys --out out.png`

## Assembly version (RARS)

1. Download `rars1_6.jar` from the RARS releases page (needs Java 8+). Do not commit it.
2. `java -jar rars1_6.jar`
3. Settings: enable **Assemble all files in directory** and **Initialize Program Counter to global 'main' if defined**.
4. File > Open `asm/main.asm`.
5. Tools > Bitmap Display: unit 4x4, display 512x512, base address `0x10040000 (heap)`, then **Connect to Program**.
6. Tools > Keyboard and Display MMIO Simulator: **Connect to Program**, and click in its lower text box before typing.
7. Assemble (F3), Run (F5), run speed at max.

Full steps with screenshots: `docs/rars_setup.md`.

Assemble-only check (what CI runs):

```sh
java -jar rars1_6.jar a nc ae1 p asm/main.asm
```

## Controls

| Action | C version | asm (RARS) |
|--------|-----------|------------|
| Move cursor | Arrow keys | i / j / k / l |
| Paint / erase | w / e (or left / right click) | w / e |
| Enter hex color | t, type, Enter (Esc cancels) | t, type, Enter (Esc cancels) |
| Save to palette | p | p |
| Pick palette slot | 1..8 (or click a swatch) | 1..8 |
| Undo / redo | ctrl+z / ctrl+shift+z | z / y |
| Grow / shrink grid | ctrl+plus / ctrl+minus | + (or =) / - |
| Save PNG | ctrl+s | s |
| Clear canvas | ctrl+delete | c |
| Quit | close the window | q |

On macOS, Cmd works like Ctrl. In the C version Esc does **not** quit (see `docs/decisions.md`).
