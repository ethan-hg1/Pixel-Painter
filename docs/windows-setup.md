# Windows setup (C version)

How to install the tools, build Pixel Painter, and run it on Windows. For
Linux, see [linux-setup.md](linux-setup.md). Controls and the demo are in the
[README](../README.md#controls).

## 1. Get the code

Either clone the repository (needs Git, see step 2):

```powershell
git clone https://github.com/ethan-hg1/Pixel-Painter.git
cd Pixel-Painter\c
```

Or download the ZIP from https://github.com/ethan-hg1/Pixel-Painter/archive/refs/heads/main.zip,
unzip it, then:

```powershell
cd Pixel-Painter-main\c
```

## 2. Install the tools

Run these in PowerShell:

```powershell
winget install Kitware.CMake
winget install Git.Git
winget install Microsoft.VisualStudio.2022.BuildTools --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
```

Close and reopen PowerShell afterward, then check the tools:

```powershell
cmake --version
git --version
```

Git is needed even for the ZIP download: CMake fetches raylib with Git during
the build.

If `cmake` is still not recognized after reopening PowerShell:

1. Check whether the install finished: `winget list Kitware.CMake`.
2. If CMake is installed, add it to PATH for the current session and try again:
   ```powershell
   $env:Path += ";C:\Program Files\CMake\bin"
   cmake --version
   ```
   To make this permanent, add `C:\Program Files\CMake\bin` to your user PATH
   in "Edit the system environment variables", then reopen PowerShell.
3. If winget is not available or keeps failing, install CMake from
   https://cmake.org/download/ (the Windows `.msi`). Choose "Add CMake to the
   system PATH for all users".

## 3. Build

From the `c` folder:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

The first configure takes about a minute because it downloads raylib.

If `cmake -S . -B build` reports a missing compiler, open **Developer
PowerShell for VS 2022** from the Start menu and run the two commands there.

## 4. Run the tests

```powershell
.\build\Release\pp_tests.exe
```

Expected output: `735 checks, 0 failed`.

## 5. Run the app

```powershell
.\build\Release\pixel_painter.exe
```

Close the window to quit. Esc does not quit the app.

Save PNG writes `pixel_painter_YYYYMMDD_HHMMSS.png` to your Downloads folder.
If that folder does not exist, the file goes to the folder you ran the app from.

## Headless replay (no window)

Runs a key script and writes a PNG. Run it from the `c` folder:

```powershell
.\build\Release\pixel_painter.exe --replay ..\tests\scripts\export_16x16.keys --out demo.png
```

The output matches `tests\golden\export_16x16.png`.

## Troubleshooting

- **`cmake` or `git` is not recognized:** reopen PowerShell after installing,
  then see the PATH steps in section 2.
- **Git asks for a password during the build:** cancel and retry. The
  repository is public and should not need a login.
- **Long path errors while cloning or building:** clone into a short folder
  such as `C:\dev`, and in an admin PowerShell run
  `git config --system core.longpaths true`.
- **Compiler errors during the build:** run the build from **Developer
  PowerShell for VS 2022**.
