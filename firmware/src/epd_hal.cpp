#include <Arduino.h>
#include <SPI.h>

#include "epd_config.h"
#include "epd_hal.h"

namespace epd {
namespace {

SPISettings s_bus(EPD_SPI_HZ, MSBFIRST, SPI_MODE0);

inline void selectChip(bool active) {
    digitalWrite(pins::kCs, active ? LOW : HIGH);
}

inline void csSetupDelay() { delayMicroseconds(EPD_CS_SETUP_US); }

inline void setCommandMode() { digitalWrite(pins::kDc, LOW); }
inline void setDataMode() { digitalWrite(pins::kDc, HIGH); }

}  // namespace

void halBegin() {
    if (pins::kPower >= 0) {
        // Hold the panel unpowered while the ESP32 boots.
        pinMode(pins::kPower, OUTPUT);
        digitalWrite(pins::kPower, pins::kPowerActiveLow ? HIGH : LOW);
        delay(100);
    }

    pinMode(pins::kCs, OUTPUT);
    digitalWrite(pins::kCs, HIGH);
    pinMode(pins::kDc, OUTPUT);
    digitalWrite(pins::kDc, HIGH);
    pinMode(pins::kRst, OUTPUT);
    digitalWrite(pins::kRst, HIGH);
    pinMode(pins::kBusy, INPUT_PULLUP);

    // Explicit pins: never call SPI.begin() without arguments, the ESP32-C3
    // defaults (CLK 6 / MOSI 7) do not match this harness.
    SPI.begin(pins::kSck, pins::kMiso, pins::kMosi, -1);

    if (pins::kPower >= 0) {
        // Bus is idle-high and the ESP32 is up: safe to power the panel.
        digitalWrite(pins::kPower, pins::kPowerActiveLow ? LOW : HIGH);
        delay(300);
    }
}

void reset() {
    // iTC application note: 5 ms settle, 10 ms low pulse, 5 ms settle.
    delay(5);
    digitalWrite(pins::kRst, HIGH);
    delay(5);
    digitalWrite(pins::kRst, LOW);
    delay(10);
    digitalWrite(pins::kRst, HIGH);
    delay(5);
}

void cmd(uint8_t command) {
    SPI.beginTransaction(s_bus);
    setCommandMode();
    selectChip(true);
    csSetupDelay();
    SPI.transfer(command);
    csSetupDelay();
    selectChip(false);
    SPI.endTransaction();
}

void data(const uint8_t* buffer, size_t length) {
    SPI.beginTransaction(s_bus);
    setDataMode();
    selectChip(true);
    csSetupDelay();
    SPI.writeBytes(buffer, length);
    csSetupDelay();
    selectChip(false);
    SPI.endTransaction();
}

void cmdWithData(uint8_t command, const uint8_t* buffer, size_t length) {
    SPI.beginTransaction(s_bus);
    setCommandMode();
    selectChip(true);
    csSetupDelay();
    SPI.transfer(command);
    csSetupDelay();
    setDataMode();
    csSetupDelay();
    SPI.writeBytes(buffer, length);
    csSetupDelay();
    selectChip(false);
    SPI.endTransaction();
}

bool busy() { return digitalRead(pins::kBusy) == LOW; }

bool waitReady(uint32_t timeoutMs) {
    const uint32_t start = millis();
    while (busy()) {
        if (millis() - start > timeoutMs) {
            Serial.printf("[epd] BUSY timeout after %lu ms\n",
                          (unsigned long)timeoutMs);
            return false;
        }
        delay(5);
    }
    return true;
}

}  // namespace epd
