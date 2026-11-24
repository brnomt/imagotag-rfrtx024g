#pragma once

// ---------------------------------------------------------------------------
// Panel: Pervasive Displays 2.66" 296x152 black/white/red, iTC controller
//        (sold as SE2266JS0C1 / E2266JS0C1)
// Target: ESP32-C3 Super Mini, Arduino framework
//
// The panel was recovered from a SES-imagotag VUSION electronic shelf label.
// Its controller is Pervasive's iTC, not an IL0373 or SSD16xx part: see
// docs/controller-notes.md before changing anything here.
// ---------------------------------------------------------------------------

#include <stddef.h>
#include <stdint.h>

constexpr uint16_t kPanelWidth = 296;
constexpr uint16_t kPanelHeight = 152;

// iTC stores the image column-major: one 152-bit column per source line,
// MSB first. 19 bytes per column, 5624 bytes per plane.
constexpr uint16_t kPanelStride = (kPanelHeight + 7) / 8;
constexpr size_t kPlaneBytes = (size_t)kPanelWidth * kPanelStride;

// SPI clock. 1 MHz is reliable on 10 cm jumper wires; the panel accepts more
// once the harness is short and solid. Keep the default conservative so the
// first power-up in a new build always works.
#ifndef EPD_SPI_HZ
#define EPD_SPI_HZ 1000000
#endif

// Setup time around every CS edge, in microseconds.
#ifndef EPD_CS_SETUP_US
#define EPD_CS_SETUP_US 10
#endif

// Panel temperature, in degrees Celsius, used to select the refresh waveform.
// iTC encodes it as a plain integer: 0 C = 0x00, 22 C = 0x16, 25 C = 0x19.
#ifndef EPD_TEMPERATURE_C
#define EPD_TEMPERATURE_C 25
#endif

// Frame rendered at boot: 0 = identification card, 1 = geometry test pattern.
#ifndef EPD_BOOT_DEMO
#define EPD_BOOT_DEMO 0
#endif

// Build number drawn in the top-right corner (0 disables it). A retained
// frame looks identical to a fresh one, so the tag makes a failed refresh
// obvious.
#ifndef EPD_BUILD_TAG
#define EPD_BUILD_TAG 1
#endif

// ---------------------------------------------------------------------------
// Panel supply switch
//   EPD_PANEL_POWER_PIN = -1          VDD tied to 3V3, panel always on
//   EPD_PANEL_POWER_ACTIVE_LOW = 1    P-MOSFET high-side switch (recommended)
//   EPD_PANEL_POWER_ACTIVE_LOW = 0    GPIO drives VDD directly (bench only)
// ---------------------------------------------------------------------------
#ifndef EPD_PANEL_POWER_PIN
#define EPD_PANEL_POWER_PIN 3
#endif
#ifndef EPD_PANEL_POWER_ACTIVE_LOW
#define EPD_PANEL_POWER_ACTIVE_LOW 1
#endif

// ---------------------------------------------------------------------------
// Wiring: ESP32-C3 Super Mini -> panel FPC test pads
//
// GPIO 2/8/9 are strapping pins and are avoided on purpose. GPIO 18/19 are
// USB D-/D+, GPIO 20/21 are UART0.
// ---------------------------------------------------------------------------
namespace pins {
constexpr int kSck = 4;    // SCL
constexpr int kMosi = 6;   // SDA (also used for the OTP read on 3-wire parts)
constexpr int kMiso = 10;  // unused, kept so SPI.begin() stays explicit
constexpr int kCs = 7;     // CS
constexpr int kDc = 5;     // D/C: LOW = command, HIGH = data
constexpr int kRst = 1;    // RES, active low
constexpr int kBusy = 0;   // BUSY, open-drain, LOW = busy
constexpr int kPower = EPD_PANEL_POWER_PIN;
constexpr bool kPowerActiveLow = EPD_PANEL_POWER_ACTIVE_LOW != 0;
}  // namespace pins

// ---------------------------------------------------------------------------
// Orientation. These three values are the combination verified on this panel
// with the ESP32-C3 harness. Change them only for a different film or label.
// ---------------------------------------------------------------------------
#ifndef EPD_MIRROR_X
#define EPD_MIRROR_X 0
#endif
#ifndef EPD_MIRROR_Y
#define EPD_MIRROR_Y 1
#endif
// This film presents the black plane inverted relative to the datasheet
// convention, so logical black and white are swapped on the way out.
#ifndef EPD_INVERT
#define EPD_INVERT 1
#endif
