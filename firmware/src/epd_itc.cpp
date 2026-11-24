#include <Arduino.h>
#include <string.h>

#include "epd_config.h"
#include "epd_hal.h"
#include "epd_itc.h"

namespace epd {
namespace {

// iTC command set used by this panel.
constexpr uint8_t kPanelSetting = 0x00;    // soft reset when sent with 0x0E
constexpr uint8_t kDcDcOff = 0x02;
constexpr uint8_t kDcDcOn = 0x04;
constexpr uint8_t kWriteBlack = 0x10;
constexpr uint8_t kRefresh = 0x12;
constexpr uint8_t kWriteRed = 0x13;
constexpr uint8_t kTemperatureMode = 0xE0;
constexpr uint8_t kTemperature = 0xE5;

// Verified against the 2.66" film J panel. These are the values Pervasive uses
// for small C/J parts; the OTP copy cannot be read without 3-wire SDA.
constexpr uint8_t kPsrLow = 0xCF;
constexpr uint8_t kPsrHigh = 0x8D;

uint8_t s_black[kPlaneBytes];
uint8_t s_red[kPlaneBytes];

struct PixelRef {
    size_t index;
    uint8_t mask;
};

PixelRef locate(int x, int y) {
#if EPD_MIRROR_X
    x = kPanelWidth - 1 - x;
#endif
    uint8_t row = y & 7;
    size_t column = y >> 3;
#if EPD_MIRROR_Y
    column = kPanelStride - 1 - column;
    row = 7 - row;
#endif
    return {static_cast<size_t>(x) * kPanelStride + column,
            static_cast<uint8_t>(0x80u >> row)};
}

inline Colour toPanelColour(Colour colour) {
#if EPD_INVERT
    if (colour == White) return Black;
    if (colour == Black) return White;
#endif
    return colour;
}

}  // namespace

void init() {
    reset();

    const uint8_t softReset = 0x0E;
    cmdWithData(kPanelSetting, &softReset, 1);
    delay(5);

    const uint8_t temperature = static_cast<uint8_t>(EPD_TEMPERATURE_C);
    cmdWithData(kTemperature, &temperature, 1);

    const uint8_t useRegister = 0x02;
    cmdWithData(kTemperatureMode, &useRegister, 1);

    const uint8_t psr[2] = {kPsrLow, kPsrHigh};
    cmdWithData(kPanelSetting, psr, sizeof(psr));
}

bool show() {
    cmdWithData(kWriteBlack, s_black, sizeof(s_black));
    cmdWithData(kWriteRed, s_red, sizeof(s_red));

    if (!waitReady(5000)) return false;

    // The booster can take tens of seconds to come up on a weak supply.
    cmd(kDcDcOn);
    if (!waitReady(90000)) {
        Serial.println("[epd] DC/DC did not start: check VDD and booster caps");
        cmd(kDcDcOff);
        return false;
    }

    cmd(kRefresh);
    delay(5);
    if (!waitReady(45000)) return false;

    cmd(kDcDcOff);
    waitReady(5000);
    return true;
}

void powerOff() {
    cmd(kDcDcOff);
    waitReady(5000);
}

void clear(Colour colour) {
    const Colour panelColour = toPanelColour(colour);
    memset(s_black, panelColour == White ? 0xFF : 0x00, sizeof(s_black));
    memset(s_red, panelColour == Red ? 0xFF : 0x00, sizeof(s_red));
}

void pixel(int x, int y, Colour colour) {
    if (x < 0 || y < 0 || x >= kPanelWidth || y >= kPanelHeight) return;

    const PixelRef p = locate(x, y);
    switch (toPanelColour(colour)) {
        case White:  // black plane 1 = white, red plane 0 = no red
            s_black[p.index] |= p.mask;
            s_red[p.index] &= static_cast<uint8_t>(~p.mask);
            break;
        case Black:  // both planes 0 = black
            s_black[p.index] &= static_cast<uint8_t>(~p.mask);
            s_red[p.index] &= static_cast<uint8_t>(~p.mask);
            break;
        case Red:  // black plane 0, red plane 1
            s_black[p.index] &= static_cast<uint8_t>(~p.mask);
            s_red[p.index] |= p.mask;
            break;
    }
}

uint8_t* blackPlane() { return s_black; }
uint8_t* redPlane() { return s_red; }

}  // namespace epd
