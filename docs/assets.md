# Asset checklist

The repository is functional without media, but a hardware reference needs
evidence. This is the exact shot list. Place every file in `docs/images/`
unless stated otherwise, then reference it from the page in the "Used in"
column.

## Capture rules

- Format: JPEG for photos, PNG for screenshots and diagrams.
- Size: 1600 px on the long edge, under 500 KB per file. Crop, do not downscale
  the whole frame.
- Remove EXIF metadata (location, device serial) before committing.
- Neutral background, diffuse light, no reflections on the flex.
- Macro shots: focus on the conductor, not the connector housing.
- No visible serial numbers or asset tags on the label.

## Photos

| File | Subject | Framing | Used in |
| --- | --- | --- | --- |
| `hero.jpg` | Finished build, label showing the identification card, ESP32-C3 visible | 3/4 view, whole assembly, screen legible | `README.md` |
| `label-front.jpg` | Original VUSION label, front and edge | Straight on, display off | `docs/pinout.md` |
| `label-back.jpg` | Rear housing opened, PCB and battery visible | Straight on, even light | `docs/pinout.md` |
| `pcb-test-points.jpg` | Label PCB with test pads / vias labelled | Macro, 1:1, labels readable | `docs/pinout.md` |
| `fpc-connector.jpg` | Display flex mated in its connector | Macro, side angle, latch visible | `docs/pinout.md` |
| `solder-joints.jpg` | Eight wires soldered on the panel side | Macro, top-down, joints in focus | `docs/pinout.md` |
| `label-wiring.jpg` | **Present.** Panel-side wiring on the label PCB | Top-down, connector and solder points visible | `README.md`, `docs/pinout.md` |
| `wiring-overview.jpg` | ESP32-C3 Super Mini to label, all wires | Top-down, wires not overlapping | `README.md`, `docs/pinout.md` |
| `panel-test-pattern.jpg` | Boot test pattern on the panel | Straight on, fill the frame | `README.md` |
| `panel-hello.jpg` | Identification card on the panel | Straight on, screen legible | `README.md` |

## Diagrams

| File | Content | Notes |
| --- | --- | --- |
| `power-switch.png` | 3V3 → P-MOSFET → panel VDD, gate to GPIO 3, 10 kΩ pull-up, bulk caps | Export at 2× from KiCad or draw with a vector tool |
| `wiring-diagram.png` | ESP32-C3 pinout and panel signals | Same table as `pinout.csv`, drawn as a diagram |

## Captures

| File | Content | How to capture |
| --- | --- | --- |
| `logic-spi-init.png` | CS, D/C, SCK, MOSI during init | 1 MS/s or faster; annotate the `0x00 / 0x0E` frame and the PSR frame |
| `logic-refresh.png` | CS and BUSY across a full refresh | 10 s window; show BUSY low during refresh and release at the end |
| `serial-console.png` | Boot log and refresh timing | 115200 baud, monospace, trim to the relevant lines |

## Text facts still needed

These must come from the physical unit and cannot be inferred:

- [ ] Exact test-point labels or via names on the label PCB, per board revision.
- [ ] Board revision and silkscreen markings.
- [ ] Panel flex contact count and pin numbering (confirm the 24-pin
      assumption).
- [ ] Measured refresh current and idle current, with the meter model.
- [ ] Confirmation of the P-MOSFET part number used in the final build.
- [ ] URL for BeatSkip's ShelfKit, if it is public.

Confirmed: the label logic board is an EFR32FG22 (Silicon Labs Wireless Gecko).

Fill the corresponding cells in [pinout.md](pinout.md) and the BOM in
[../hardware/README.md](../hardware/README.md) once measured.

## Where images are referenced

`README.md` and the pages under `docs/` use relative paths such as
`docs/images/hero.jpg`. Keep the filenames exactly as listed so the links work
without edits.
