// ---------------------------------------------------------------------------
// SES-imagotag VUSION 2.6" e-paper label driven by an ESP32-C3 Super Mini.
//
// The label's own MCU is left untouched; the panel is driven directly over
// SPI. See README.md for wiring, power and safety notes.
// ---------------------------------------------------------------------------
#include <Arduino.h>

#include "epd_config.h"
#include "epd_hal.h"
#include "epd_itc.h"
#include "gfx.h"

// Pushes the current framebuffer and waits for the refresh to finish.
static void present() {
#if EPD_BUILD_TAG > 0
    gfx::buildTag(EPD_BUILD_TAG);
#endif

    const uint32_t start = millis();
    if (epd::show())
        Serial.printf("[epd] refresh complete in %lu ms\n",
                      static_cast<unsigned long>(millis() - start));
    else
        Serial.println("[epd] refresh failed");
}

// Prints BUSY for 15 s while the booster starts. A healthy panel releases BUSY
// shortly after 0x04; one that stays low has a supply or wiring problem.
static void traceBusy() {
    Serial.printf("[epd] BUSY idle = %d\n", digitalRead(pins::kBusy));
    epd::cmd(0x04);
    const uint32_t start = millis();
    while (millis() - start < 15000) {
        Serial.printf("[epd] +%4lu ms  BUSY = %d\n",
                      static_cast<unsigned long>(millis() - start),
                      digitalRead(pins::kBusy));
        delay(500);
    }
    epd::cmd(0x02);
}

static void printHelp() {
    Serial.println("[epd] keys: w white | k black | r red | p pattern | e hello");
    Serial.println("[epd]       i init  | s power off | t busy trace | h help");
}

void setup() {
    Serial.begin(115200);
    delay(300);

    epd::halBegin();

    Serial.printf("[epd] %ux%u, iTC controller, SPI %lu Hz\n", kPanelWidth,
                  kPanelHeight, static_cast<unsigned long>(EPD_SPI_HZ));
    Serial.printf("[epd] SCK=%d MOSI=%d CS=%d DC=%d RST=%d BUSY=%d\n",
                  pins::kSck, pins::kMosi, pins::kCs, pins::kDc, pins::kRst,
                  pins::kBusy);
    Serial.printf("[epd] build tag %d, boot demo %d\n", EPD_BUILD_TAG,
                  EPD_BOOT_DEMO);

    epd::init();

#if EPD_BOOT_DEMO == 1
    gfx::testPattern();
#else
    gfx::hello();
#endif
    present();

    printHelp();
}

void loop() {
    if (!Serial.available()) {
        delay(20);
        return;
    }

    switch (Serial.read()) {
        case 'w':
            epd::clear(epd::White);
            present();
            break;
        case 'k':
            epd::clear(epd::Black);
            present();
            break;
        case 'r':
            epd::clear(epd::Red);
            present();
            break;
        case 'p':
            gfx::testPattern();
            present();
            break;
        case 'e':
            gfx::hello();
            present();
            break;
        case 'i':
            epd::init();
            Serial.println("[epd] re-initialised");
            break;
        case 's':
            epd::powerOff();
            Serial.println("[epd] DC/DC off, image retained");
            break;
        case 't':
            traceBusy();
            break;
        case 'h':
            printHelp();
            break;
        default:
            break;
    }
}
