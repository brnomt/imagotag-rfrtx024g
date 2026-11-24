# iTC protocol notes

This is the minimum command set needed to initialise and refresh the 2.66"
296×152 black/white/red panel used in the VUSION label. It was reconstructed
from the Pervasive Displays `PDLS_Basic` / `PDLS_EXT3_Basic_Global` sources and
verified against the panel.

## Command sequence

| Step | Command | Payload | Timing |
| --- | --- | --- | --- |
| Hardware reset | RES low | — | 10 ms low pulse, 5 ms settle |
| Soft reset | `0x00` | `0x0E` | wait 5 ms |
| Temperature | `0xE5` | degrees Celsius | 0 °C = `0x00`, 22 °C = `0x16`, 25 °C = `0x19` |
| Temperature mode | `0xE0` | `0x02` | use the register above |
| Panel setting (PSR) | `0x00` | `0xCF 0x8D` | film J / small C-J parts |
| Write black plane | `0x10` | 5624 bytes | — |
| Write red plane | `0x13` | 5624 bytes | — |
| DC/DC on | `0x04` | — | wait BUSY, up to tens of seconds |
| Refresh | `0x12` | — | ~15–25 s, wait BUSY |
| DC/DC off | `0x02` | — | image is retained |

## Command framing

A command and its payload travel in **one CS frame**:

```text
CS  ‾‾\_____________________________________________/‾‾
DC  ‾‾\___________/‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾\____
      command byte      payload bytes
```

If the command byte and the payload are sent as two separate transactions, the
controller ignores the write. This is the single most common reason an iTC
panel stays blank after an otherwise correct-looking init.

## Framebuffer format

- Two planes of 5624 bytes each: black (`0x10`) and red (`0x13`).
- Column-major: 296 columns × 19 bytes, 152 bits per column, MSB = first gate
  line.
- Pixel address: `index = x * 19 + (y >> 3)`, mask `0x80 >> (y & 7)`.
- A row-major buffer produces an image rotated 90° or split into vertical
  bands.

### Colour encoding

| Colour | Black plane | Red plane |
| --- | --- | --- |
| White | 1 | 0 |
| Black | 0 | 0 |
| Red | 0 | 1 |

The film in this label presents the black plane inverted relative to the
datasheet convention. `EPD_INVERT` in `epd_config.h` swaps black and white at
the driver boundary so drawing code can stay logical.

## BUSY line

- iTC BUSY is open-drain and **active low**: low = busy.
- This is the opposite of SSD16xx and UC8151 panels, where high = busy.
- The line needs a pull-up. The firmware enables the ESP32-C3 internal pull-up
  on GPIO 0.
- If BUSY never releases after `0x04`, the boost converter is not running:
  check VDD, the 1 µF charge-pump capacitors and the ground return.

## Timing that matters

| Event | Expected duration |
| --- | --- |
| Soft reset | a few ms |
| PSR write | immediate |
| DC/DC start | up to tens of seconds on a weak supply |
| BWR refresh | 15–25 s |
| DC/DC off | a few ms |

A full refresh is slow by design. Do not loop it: every refresh consumes panel
cycles and slowly degrades the film.

## OTP

The panel stores its power settings in OTP, read over 3-wire SPI with a
bidirectional SDA line. This driver skips the read and uses the fixed PSR
values instead, which is sufficient for normal operation. Reading OTP requires
switching GPIO 6 to an input for the response phase, which is outside the scope
of this repository.

## References

- Pervasive Displays `PDLS_Basic` — iTC driver and HAL
- Pervasive Displays `PDLS_EXT3_Basic_Global` — iTC C/J sequences and buffer
  format
- Pervasive Displays application notes for small-size panels — boost converter
  capacitors and waveform tables
