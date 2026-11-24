# Controller identification

The assumption that a 296×152 2.6" panel is an IL0373 / GDEW026Z39 is common,
because most hobbyist panels of that size use that controller. It is wrong for
this label, and starting from an IL0373 driver wastes a lot of time.

## What this panel actually is

`SE2266JS0C1` (often printed incompletely as `SE226JS0C1`) decodes using the
Pervasive Displays naming scheme:

```text
S E 2 266 J S0 C 1
│ │ │  │  │ │  │ └─ revision
│ │ │  │  │ │  └─── series
│ │ │  │  │ └────── driver: S0 = iTC
│ │ │  │  └──────── film: J = Spectra black/white/red
│ │ │  └─────────── size: 2.66"
│ │ └────────────── 2 = 2.x inch family
│ └──────────────── E = E-paper
└────────────────── S = small size
```

The commercial equivalent is `E2266JS0C1`. There is no IL0373 in this signal
path.

## iTC versus IL0373 / UC8151

| Property | Pervasive iTC (this panel) | IL0373 / UC8151 |
| --- | --- | --- |
| Command + payload framing | One CS frame | Separate transactions |
| BUSY polarity | Low = busy | High = busy |
| Framebuffer layout | Column-major | Row-major |
| Planes | Black `0x10` and red `0x13` | DTM1/DTM2 |
| Init | `0x00/0x0E`, temp, `0x00` PSR | PSR, power, booster, TRES, `0x04` |
| Resolution register | None | `0x61` |
| Typical refresh | 15–25 s (BWR) | 2–20 s depending on LUT |

Consequences:

- **GxEPD2 cannot drive this panel.** It supports SSD16xx, UC8151/IL0373 and
  similar controllers, but has no iTC backend.
- Selecting `EPD_CONTROLLER=2` (UC8151/IL0373) in an earlier revision of this
  firmware produced no refresh at all. The panel ignored the command stream and
  BUSY never moved.
- Swapping CS and D/C, lowering the SPI clock and changing bit order all failed
  for the same reason: the protocol was wrong, not the wiring.

## Other panels

If your label uses a UC8151/IL0373-class panel, this repository is not the right
starting point. Use GxEPD2 or the Pervasive `PDLS_Basic` stack for that
controller. The ESP32-C3 harness and power path in
[pinout.md](pinout.md) still apply, but the command sequence, BUSY polarity and
framebuffer layout do not.

## Label logic board

The unit documented here carries a **Silicon Labs EFR32FG22** Wireless Gecko.
It sits on the panel's SPI bus and is not used by this project. Silkscreen
markings and test-point names differ between label revisions; record them per
revision in [pinout.md](pinout.md).

Net mapping and connector tracing were done with
[BoardExplorer](https://github.com/brnomt/BoardExplorer).

## Identifying an unknown label

1. Read the flex silkscreen or the panel edge marking.
2. Decode the Pervasive model number, if present.
3. Probe BUSY at rest: low at rest suggests an open-drain, active-low line.
4. Send a soft reset and watch BUSY; no reaction suggests a wrong controller or
   an unpowered boost converter.
5. Capture CS, D/C and MOSI with a logic analyser and compare with the framing
   above.

Never apply a waveform/PSR value from a different film. The drive voltages are
panel-specific and can damage the display permanently.
