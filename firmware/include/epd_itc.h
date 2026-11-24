#pragma once

#include <stdint.h>

// Pervasive iTC driver for the 2.66" 296x152 black/white/red panel.
//
// The driver owns two 1 bpp planes and addresses them with a top-left origin
// and logical colours. Panel quirks (column-major layout, gate order, plane
// polarity) are applied here, so drawing code never needs to know about them.
namespace epd {

enum Colour : uint8_t { White, Black, Red };

void init();

// Pushes both planes and runs a full refresh. Returns false if BUSY never
// released. A BWR refresh takes roughly 15-25 s.
bool show();

// Turns the DC/DC converter off. The image is retained on the panel.
void powerOff();

void clear(Colour colour);
void pixel(int x, int y, Colour colour);

// Raw plane access, for advanced use. Layout: x * kPanelStride + (y / 8).
uint8_t* blackPlane();
uint8_t* redPlane();

}  // namespace epd
