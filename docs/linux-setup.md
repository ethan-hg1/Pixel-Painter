# Linux setup (C version)

How to install the tools, build Pixel Painter, and run it on Linux. Commands
are for Debian and Ubuntu; other distributions need the equivalent packages.
For Windows, see [windows-setup.md](windows-setup.md). Controls and the demo
are in the [README](../README.md#controls).

## 1. Install the tools and raylib's dependencies

```sh
sudo apt update
sudo apt install build-essential cmake git libx11-dev libxrandr-dev \
    libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev
```

- `build-essential` provides the C compiler.
- `cmake` needs version 3.16 or newer. Check with `cmake --version`.
- `git` is needed because CMake downloads raylib with it during the build.
- The `lib*-dev` packages are the X11 and OpenGL headers raylib needs to open a window.

## 2. Get the code

```sh
git clone https://github.com/ethan-hg1/Pixel-Painter.git
cd Pixel-Painter/c
```

## 3. Build

From the `c` folder:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The first configure takes about a minute because it downloads raylib.

## 4. Run the tests

```sh
./build/pp_tests
```

Expected output: `735 checks, 0 failed`.

## 5. Run the app

```sh
./build/pixel_painter
```

Close the window to quit. Esc does not quit the app.

Save PNG (Ctrl+S) writes `pixel_painter_YYYYMMDD_HHMMSS.png` to `~/Downloads`.
If that folder does not exist, the file goes to the folder you ran the app from.

## Headless replay (no window)

Runs a key script and writes a PNG. Run it from the `c` folder:

```sh
./build/pixel_painter --replay ../tests/scripts/export_16x16.keys --out demo.png
```

The output matches `tests/golden/export_16x16.png`.

## Troubleshooting

- **`cmake` is too old:** install a newer CMake from https://cmake.org/download/
  or use `pip install cmake`.
- **The window does not open on a remote or headless machine:** the app needs
  a display. Use the replay mode above, or run it under a virtual display such
  as `xvfb-run ./build/pixel_painter` (install with `sudo apt install xvfb`).
- **`fatal: unable to access ... github.com`:** check your network connection
  or proxy settings. CMake needs GitHub access to fetch raylib.
- **Linker errors about X11 or GL:** make sure the `lib*-dev` packages in
  step 1 are installed, then delete `build` and configure again.
