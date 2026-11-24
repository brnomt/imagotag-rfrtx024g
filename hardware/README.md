# Hardware notes

Reference build for powering and wiring the panel. No custom PCB is required;
the circuit fits on the label itself or on a small piece of perfboard.

## Bill of materials

| Qty | Part | Notes |
| --- | --- | --- |
| 1 | ESP32-C3 Super Mini | 4 MB flash, native USB-CDC |
| 1 | P-channel MOSFET | AO3401A, DMG2301L or similar; Vgs(th) suitable for 3.3 V |
| 1 | 10 kΩ resistor | Gate pull-up to 3V3 |
| 1 | 100 nF ceramic | Panel VDD decoupling, at the connector |
| 1 | 10 µF ceramic | Panel VDD bulk, at the connector |
| 1 | 100 µF electrolytic or tantalum | Supply bulk, near the ESP32-C3 |
| — | 30 AWG silicone wire | Short runs; keep under ~15 cm |
| 1 | USB-C data cable | Power and serial console |

Optional test equipment: multimeter, logic analyser (≥ 8 channels, ≥ 1 MS/s).

## Power switch

The panel is switched on its high side:

```text
3V3 ──┬─────────────── S  P-MOSFET  D ─────────── VDD / VDDIO (panel)
      │                 │
     10 kΩ              G ─── GPIO 3
      │
3V3 ──┘
```

- Gate high at reset → panel off while the ESP32-C3 boots.
- Firmware drives GPIO 3 low to power the panel.
- If the MOSFET is omitted, wire VDD/VDDIO directly to 3V3 and set
  `EPD_PANEL_POWER_PIN` to `-1` in `firmware/include/epd_config.h`.

The bench prototype for this project skipped the MOSFET and tied VDD/VDDIO to
GPIO 3, driving it high from firmware (`env:direct`). It works, but the GPIO
current limit is below the refresh peak. Treat it as a bring-up shortcut only.

Do not switch the ground side. The panel's boost converter references ground,
and a low-side switch will produce erratic refreshes.

## Assembly

1. Open the label and disconnect the battery before touching the PCB.
2. Identify the panel flex and the SPI nets. Record the attachment points in
   [../docs/pinout.md](../docs/pinout.md).
3. Solder the eight wires (SCL, SDA, CS, D/C, RES, BUSY, VDD, GND). Keep them
   short and strain-relieve the flex.
4. Fit the decoupling capacitors as close as possible to the panel connector.
5. Build the high-side switch and verify with a meter before connecting the
   panel.
6. Power up and check 3.3 V at the panel connector before flashing.
7. Flash the `release` environment and watch the serial console.

## Measurements to record

Fill these in after the first working build:

| Parameter | Value |
| --- | --- |
| Idle current, panel powered | TBC |
| Refresh current, peak | TBC |
| Refresh current, average | TBC |
| Current with panel switched off | TBC |
| Refresh duration | TBC |

## Notes

- The panel's charge-pump capacitors live on the label PCB. Do not cut the PCB
  down without preserving them, or transfer equivalent 1 µF capacitors.
- The ESP32-C3 3V3 regulator on the Super Mini is adequate for the panel peaks
  when powered from a solid USB port. A weak port causes brown-outs during
  refresh; the DC/DC then never completes.
- Keep the BUSY line short. It is open-drain and relatively slow; long wires
  pick up noise during the refresh.
- The image is retained after `0x02`. In a battery design, power the panel down
  completely between updates and re-initialise before the next refresh.
