#pragma once

// ---------------------------------------------------------------------------
// Panel: Pervasive Displays 2.66" 296x152 black/white/red, iTC controller
// (SE2266JS0C1 / E2266JS0C1). Change these if you fit another panel.
// ---------------------------------------------------------------------------
#ifndef EPD_WIDTH
#define EPD_WIDTH 296
#endif
#ifndef EPD_HEIGHT
#define EPD_HEIGHT 152
#endif

// 3 = Pervasive iTC (this panel), 1 = SSD1619/SSD1680, 2 = UC8151/IL0373
#ifndef EPD_CONTROLLER
#define EPD_CONTROLLER 3
#endif

// BUSY polarity: iTC uses LOW = busy, SSD16xx/UC8151 use HIGH = busy.
#ifndef EPD_BUSY_ACTIVE_HIGH
#if EPD_CONTROLLER == 3
#define EPD_BUSY_ACTIVE_HIGH 0
#else
#define EPD_BUSY_ACTIVE_HIGH 1
#endif
#endif

// ---------------------------------------------------------------------------
// ESP32-C3 Super Mini wiring.
// GPIO 2 / 8 / 9 are the strapping pins -> avoided on purpose.
// GPIO 18/19 = USB, GPIO 20/21 = UART0.
// ---------------------------------------------------------------------------
// Boot image: 0 = greeting, 1 = geometry calibration, 2 = solid black
#ifndef EPD_BOOT_DEMO
#define EPD_BOOT_DEMO 0
#endif

// 1 = boot straight into the pin wiggle, for continuity checks
#ifndef EPD_BOOT_WIGGLE
#define EPD_BOOT_WIGGLE 0
#endif

// 1 = boot into the step-by-step BUSY diagnostics
#ifndef EPD_BOOT_DIAG
#define EPD_BOOT_DIAG 0
#endif

// Experimental: ignore BUSY and drive the refresh with fixed delays. Tells a
// lying BUSY line apart from a panel that genuinely cannot refresh
// (-D EPD_IGNORE_BUSY=1).
#ifndef EPD_IGNORE_BUSY
#define EPD_IGNORE_BUSY 0
#endif

// Refresh time used by the blind mode, in ms. BWR Spectra panels need ~20-30 s.
#ifndef EPD_BLIND_MS
#define EPD_BLIND_MS 15000
#endif

// iTC normally takes its booster configuration from OTP. Set to 1 to send the
// reference power registers (0x01 PWR, 0x06 BTST) before 0x04 as an experiment
// on panels whose DC/DC never completes (-D EPD_ITC_POWER_SETTINGS=1).
#ifndef EPD_ITC_POWER_SETTINGS
#define EPD_ITC_POWER_SETTINGS 0
#endif

// Set to 1 to swap CS and D/C in software, for A/B testing a suspected wiring
// mix-up (build with -D EPD_SWAP_CS_DC=1).
#ifndef EPD_SWAP_CS_DC
#define EPD_SWAP_CS_DC 0
#endif

// Panel image is horizontally mirrored on this 2.66" iTC panel: flip X.
#ifndef EPD_FLIP_X
#define EPD_FLIP_X 1
#endif
// Set to 1 if the image comes out upside down (bit order inside each byte).
#ifndef EPD_FLIP_Y
#define EPD_FLIP_Y 0
#endif
// Set to 1 to mirror the whole image vertically (reverse byte order and bit
// order). This is what this panel needs; EPD_FLIP_Y alone only mirrors inside
// each 8-pixel band.
#ifndef EPD_FLIP_Y_AXIS
#define EPD_FLIP_Y_AXIS 0
#endif
// Set to 1 if black and white come out swapped on the panel.
#ifndef EPD_INVERT
#define EPD_INVERT 0
#endif
// Set to 1 if the panel maps bit 0 to the leftmost pixel of each byte
// (LSB-first in X). Leaves 0x00/0xFF bytes unchanged but scrambles glyphs.
#ifndef EPD_X_LSB
#define EPD_X_LSB 0
#endif

// Big build number in the top-right corner, so a stale frame is obvious.
#ifndef EPD_BUILD_TAG
#define EPD_BUILD_TAG 0
#endif

// Optional high-side switch for the panel supply. Gate of a P-channel MOSFET
// (source = 3V3, drain = panel VDD/VDDIO, 10k gate pull-up to 3V3). The panel
// stays off while the ESP boots and is powered once the firmware drives the
// gate low. Set to -1 when the panel is wired straight to 3V3.
#ifndef EPD_PIN_PWR
#define EPD_PIN_PWR 3
#endif
#ifndef EPD_PWR_ON_LEVEL
#define EPD_PWR_ON_LEVEL LOW  // P-MOSFET: gate low = on
#endif

static constexpr int EPD_PIN_SCK = 4;   // SCL
static constexpr int EPD_PIN_MOSI = 6;  // SDA
static constexpr int EPD_PIN_MISO = 10; // unused, keeps SPI.begin() quiet
#if EPD_SWAP_CS_DC
static constexpr int EPD_PIN_CS = 5;  // physically wired to D/C
static constexpr int EPD_PIN_DC = 7;  // physically wired to CS
#else
static constexpr int EPD_PIN_CS = 7;  // CS
static constexpr int EPD_PIN_DC = 5;  // D/C
#endif
static constexpr int EPD_PIN_RST = 1;   // RES
static constexpr int EPD_PIN_BUSY = 0;  // BUSY

// Start slow (jumper wires / flex). The iTC controller accepts 10-16 MHz once
// the wiring is solid.
#ifndef EPD_SPI_HZ
#define EPD_SPI_HZ 4000000
#endif
#ifndef EPD_SPI_LSB
#define EPD_SPI_LSB 0  // 1 = LSBFIRST, as an experiment
#endif
// Setup/hold delay around CS and D/C edges, in microseconds. Long wires and
// slow level shifters need a few us of margin.
#ifndef EPD_CS_DELAY_US
#define EPD_CS_DELAY_US 0
#endif
