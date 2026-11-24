#pragma once

#include "epd_itc.h"

// Minimal drawing primitives on top of the panel framebuffer. Everything here
// assumes a top-left origin, 1 bpp, and colours from epd::Colour.
namespace gfx {

void drawChar(int x, int y, char c, epd::Colour colour, int scale);
void drawText(int x, int y, const char* text, epd::Colour colour, int scale);
void drawDisc(int cx, int cy, int radius, epd::Colour colour);
void drawRing(int cx, int cy, int radius, int thickness, epd::Colour colour);
void drawSmiley(int cx, int cy, int radius);

// Identification card shown at boot.
void hello();

// Geometry and colour reference: border, checkerboard, red band, stripes.
void testPattern();

// Build number in the top-right corner.
void buildTag(int value);

}  // namespace gfx
