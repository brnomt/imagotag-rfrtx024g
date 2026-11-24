#include <Arduino.h>
#include <math.h>
#include <string.h>

#include "epd_config.h"
#include "epd_itc.h"
#include "font5x7.h"
#include "gfx.h"

namespace gfx {

void drawChar(int x, int y, char c, epd::Colour colour, int scale) {
    if (c < 32 || c > 126) c = '?';
    if (scale < 1) scale = 1;

    const uint8_t* glyph = FONT5X7[c - 32];
    for (int col = 0; col < 5; col++)
        for (int row = 0; row < 7; row++)
            if (glyph[col] & (1 << row))
                for (int sy = 0; sy < scale; sy++)
                    for (int sx = 0; sx < scale; sx++)
                        epd::pixel(x + col * scale + sx, y + row * scale + sy,
                                   colour);
}

void drawText(int x, int y, const char* text, epd::Colour colour, int scale) {
    for (; *text; text++, x += 6 * scale)
        drawChar(x, y, *text, colour, scale);
}

void drawDisc(int cx, int cy, int radius, epd::Colour colour) {
    for (int y = -radius; y <= radius; y++)
        for (int x = -radius; x <= radius; x++)
            if (x * x + y * y <= radius * radius)
                epd::pixel(cx + x, cy + y, colour);
}

void drawRing(int cx, int cy, int radius, int thickness, epd::Colour colour) {
    const int inner = (radius - thickness) * (radius - thickness);
    for (int y = -radius; y <= radius; y++)
        for (int x = -radius; x <= radius; x++) {
            const int d = x * x + y * y;
            if (d <= radius * radius && d >= inner)
                epd::pixel(cx + x, cy + y, colour);
        }
}

void drawSmiley(int cx, int cy, int radius) {
    drawRing(cx, cy, radius, 4, epd::Red);
    drawDisc(cx - radius / 3, cy - radius / 3, radius / 9, epd::Black);
    drawDisc(cx + radius / 3, cy - radius / 3, radius / 9, epd::Black);
    for (int angle = 25; angle <= 155; angle++) {
        const float t = angle * PI / 180.0f;
        drawDisc(cx + static_cast<int>(0.6f * radius * cosf(t)),
                 cy + static_cast<int>(0.6f * radius * sinf(t)), 2, epd::Red);
    }
}

void hello() {
    epd::clear(epd::White);
    drawText(10, 16, "SES-imagotag", epd::Black, 2);
    drawText(10, 40, "VUSION 2.6\"", epd::Black, 2);
    drawText(10, 66, "296x152  BWR", epd::Red, 2);
    drawText(10, 92, "ESP32-C3  iTC", epd::Black, 2);
    drawSmiley(232, 76, 54);
}

void testPattern() {
    epd::clear(epd::White);
    for (int y = 0; y < kPanelHeight; y++)
        for (int x = 0; x < kPanelWidth; x++) {
            const bool border = (x < 3 || y < 3 || x >= kPanelWidth - 3 ||
                                 y >= kPanelHeight - 3);
            const bool checker =
                (((x - 3) / 16 + (y - 3) / 16) & 1) && y < kPanelHeight / 2;
            const bool redBand =
                (y >= kPanelHeight / 2 && y < kPanelHeight / 2 + 16);
            const bool stripes =
                (x >= kPanelWidth / 2) && (y >= kPanelHeight * 3 / 4) &&
                (((x - kPanelWidth / 2) / 6) & 1);

            if (border || checker || stripes) epd::pixel(x, y, epd::Black);
            if (redBand) epd::pixel(x, y, epd::Red);
        }
}

void buildTag(int value) {
    char tag[8];
    snprintf(tag, sizeof(tag), "%d", value);
    const int length = static_cast<int>(strlen(tag));
    drawText(kPanelWidth - 6 - length * 18, 2, tag, epd::Black, 3);
}

}  // namespace gfx
