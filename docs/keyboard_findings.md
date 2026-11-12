# Keyboard findings (RARS Keyboard MMIO)

What byte arrived at `KBD_DATA` (0xFFFF0004) for each key, per machine.
"none" if nothing arrived. See BUILD_INSTRUCTIONS.txt section 3.

## How to run the test (about 1 minute)

1. `java -jar rars1_6.jar`, then File > Open `asm/smoke/key_test.asm`.
   Make sure **Assemble all files in directory** is OFF for this one (it has
   its own `main`).
2. Tools > Keyboard and Display MMIO Simulator > **Connect to Program**.
3. Run > Assemble, then Run > Go.
4. Click in the lower (KEYBOARD) text box and press each key in the table, one
   at a time. The Run I/O tab prints one line per byte, e.g.
   `byte 0x0000001a = 26`.
5. Fill in your column with the hex value(s), or "none". Note your OS and
   `java -version`.

## Results

| Key | Expected (if it arrives) | Ethan (OS / Java) | Adrian (OS / Java) | Arion (OS / Java) |
|-----|--------------------------|-------------------|--------------------|-------------------|
| a | 0x61 | | | |
| shift+z | 0x5A (`Z`) | | | |
| ctrl+z | 0x1A | | | |
| ctrl+s | 0x13 | | | |
| Arrow keys | none | | | |
| Enter | 0x0A or 0x0D | | | |
| Esc | 0x1B | | | |
| Backspace | 0x08 or 0x7F | | | |
| + / = / - | 0x2B / 0x3D / 0x2D | | | |

## Reference run (not a teammate machine)

Run in the dev Codespace: Linux, OpenJDK 25, RARS on a virtual display (Xvfb),
keys sent with `xdotool`. These are **synthetic** key events, so real
keyboards may differ. Use only as a sanity check.

| Key | Byte |
|-----|------|
| a | 0x61 |
| shift+z (`Z`) | 0x5A |
| ctrl+z | 0x1A |
| ctrl+s | 0x13 |
| Up arrow | none |
| Enter | 0x0A |
| Backspace | 0x08 |
| Esc | 0x1B |

Screenshot: `docs/evidence/week1_key_test_codespace.png`.

## Consequences for `input.asm`

To be confirmed once all three columns are filled:
- Enter arrived as 0x0A here (not 0x0D). Accept both.
- Backspace arrived as 0x08. Accept 0x08 and 0x7F.
- ctrl+z / ctrl+s arrived as control codes here; still keep `z` / `s` as the
  main keys (section 3).
