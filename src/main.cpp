// ---------------------------------------------------------------------------
// Minimal hardware-SPI driver for a rescued Imagotag / VUSION e-paper panel.
// Panel: Pervasive Displays 2.66" 296x152 black/white/red (iTC controller),
//        sold as SE2266JS0C1 / E2266JS0C1.
// Target: ESP32-C3 Super Mini.
//
// EPD_CONTROLLER selects the protocol:
//   3 = Pervasive iTC, film J BWR   (default, this panel)
//   1 = SSD1619 / SSD1680           (other rescued labels)
//   2 = UC8151 / IL0373             (other rescued labels)
// ---------------------------------------------------------------------------
#include <Arduino.h>
#include <SPI.h>

#include "epd_config.h"
#include "font5x7.h"

// iTC stores the image column-major: 296 source lines, 152 gate bits each,
// MSB = first line. SSD16xx/UC8151 store it row-major instead.
#if EPD_CONTROLLER == 3
static constexpr uint16_t EPD_STRIDE = (EPD_HEIGHT + 7) / 8;
static constexpr size_t EPD_FRAME_BYTES = (size_t)EPD_WIDTH * EPD_STRIDE;
#else
static constexpr uint16_t EPD_STRIDE = (EPD_WIDTH + 7) / 8;
static constexpr size_t EPD_FRAME_BYTES = (size_t)EPD_STRIDE * EPD_HEIGHT;
#endif

static SPISettings epdSpi(EPD_SPI_HZ, EPD_SPI_LSB ? LSBFIRST : MSBFIRST, SPI_MODE0);

// iTC keeps two 1 bpp planes: black plane (0x10) and red plane (0x13).
static uint8_t frame[EPD_FRAME_BYTES];
#if EPD_CONTROLLER == 3
static uint8_t redFrame[EPD_FRAME_BYTES];
#endif

static inline size_t fbIndex(int x, int y, uint8_t& mask) {
#if EPD_CONTROLLER == 3
#if EPD_FLIP_X
    x = EPD_WIDTH - 1 - x;
#endif
    uint8_t row = y & 7;
    size_t byte = y >> 3;
#if EPD_FLIP_Y_AXIS
    byte = EPD_STRIDE - 1 - byte;
    row = 7 - row;
#endif
#if EPD_FLIP_Y
    mask = 1 << row;
#else
    mask = 0x80 >> row;
#endif
    return (size_t)x * EPD_STRIDE + byte;
#else
#if EPD_X_LSB
    mask = 1 << (x & 7);
#else
    mask = 0x80 >> (x & 7);
#endif
    return (size_t)y * EPD_STRIDE + (x >> 3);
#endif
}

// --------------------------------------------------------------------------
// Low level: command / data
// --------------------------------------------------------------------------
static inline void epdCs(bool active) { digitalWrite(EPD_PIN_CS, active ? LOW : HIGH); }

static inline void epdCsDelay() {
#if EPD_CS_DELAY_US > 0
    delayMicroseconds(EPD_CS_DELAY_US);
#endif
}

static inline bool epdBusy() {
#if EPD_BUSY_ACTIVE_HIGH
    return digitalRead(EPD_PIN_BUSY) == HIGH;
#else
    return digitalRead(EPD_PIN_BUSY) == LOW;
#endif
}

// Returns false on timeout. A stuck BUSY line is the most common symptom of a
// wiring fault or a wrong controller selection.
static bool epdWaitBusy(uint32_t timeoutMs = 20000) {
    const uint32_t t0 = millis();
    while (epdBusy()) {
        if (millis() - t0 > timeoutMs) {
            Serial.printf("[epd] BUSY timeout (%lu ms)\n", (unsigned long)timeoutMs);
            return false;
        }
        delay(5);
    }
    return true;
}

static void epdCmd(uint8_t cmd) {
    SPI.beginTransaction(epdSpi);
    digitalWrite(EPD_PIN_DC, LOW);
    epdCs(true);
    epdCsDelay();
    SPI.transfer(cmd);
    epdCsDelay();
    epdCs(false);
    SPI.endTransaction();
}

static void epdData(uint8_t value) {
    SPI.beginTransaction(epdSpi);
    digitalWrite(EPD_PIN_DC, HIGH);
    epdCs(true);
    epdCsDelay();
    SPI.transfer(value);
    epdCsDelay();
    epdCs(false);
    SPI.endTransaction();
}

static void epdData(const uint8_t* buf, size_t len) {
    SPI.beginTransaction(epdSpi);
    digitalWrite(EPD_PIN_DC, HIGH);
    epdCs(true);
    epdCsDelay();
    SPI.writeBytes(buf, len);
    epdCsDelay();
    epdCs(false);
    SPI.endTransaction();
}

// Command byte followed by data inside one CS frame. The iTC controller keys
// on this framing, so do not split it into epdCmd() + epdData().
static void epdCmdBuf(uint8_t cmd, const uint8_t* buf, size_t len) {
    SPI.beginTransaction(epdSpi);
    digitalWrite(EPD_PIN_DC, LOW);
    epdCs(true);
    epdCsDelay();
    SPI.transfer(cmd);
    epdCsDelay();
    digitalWrite(EPD_PIN_DC, HIGH);
    epdCsDelay();
    SPI.writeBytes(buf, len);
    epdCsDelay();
    epdCs(false);
    SPI.endTransaction();
}

static void epdCmdData(uint8_t cmd, uint8_t value) { epdCmdBuf(cmd, &value, 1); }

// --------------------------------------------------------------------------
// Hardware reset
// --------------------------------------------------------------------------
static void epdReset() {
#if EPD_CONTROLLER == 3
    // iTC application note: 5 ms settle, 10 ms low pulse.
    delay(5);
    digitalWrite(EPD_PIN_RST, HIGH);
    delay(5);
    digitalWrite(EPD_PIN_RST, LOW);
    delay(10);
    digitalWrite(EPD_PIN_RST, HIGH);
    delay(5);
#else
    digitalWrite(EPD_PIN_RST, HIGH);
    delay(20);
    digitalWrite(EPD_PIN_RST, LOW);
    delay(20);
    digitalWrite(EPD_PIN_RST, HIGH);
    delay(20);
    epdWaitBusy(5000);
#endif
}

// --------------------------------------------------------------------------
// RAM window helpers (SSD16xx only)
// --------------------------------------------------------------------------
static void epdSetWindow() {
#if EPD_CONTROLLER == 1
    epdCmd(0x44);  // RAM X start/end (in bytes)
    epdData(0x00);
    epdData((EPD_WIDTH - 1) / 8);
    epdCmd(0x45);  // RAM Y start/end (in lines)
    epdData(0x00);
    epdData(0x00);
    epdData((EPD_HEIGHT - 1) & 0xFF);
    epdData(((EPD_HEIGHT - 1) >> 8) & 0x03);
    epdCmd(0x4E);  // RAM X address counter
    epdData(0x00);
    epdCmd(0x4F);  // RAM Y address counter
    epdData(0x00);
    epdData(0x00);
#endif
}

#if EPD_CONTROLLER == 2
// UC8151 has no RAM window: pixels stream in continuously, so a 122 px wide
// panel needs rows packed across byte boundaries.
static constexpr size_t EPD_CONT_BYTES = ((size_t)EPD_WIDTH * EPD_HEIGHT + 7) / 8;

static void epdPackContinuous(uint8_t* out) {
    memset(out, 0xFF, EPD_CONT_BYTES);
    size_t bit = 0;
    for (int y = 0; y < EPD_HEIGHT; y++) {
        const uint8_t* row = &frame[(size_t)y * EPD_STRIDE];
        for (int x = 0; x < EPD_WIDTH; x++, bit++)
            if (!(row[x >> 3] & (0x80 >> (x & 7)))) out[bit >> 3] &= ~(0x80 >> (bit & 7));
    }
}
#endif

// --------------------------------------------------------------------------
// Controller initialisation
// --------------------------------------------------------------------------
static void epdInitSsd16xx() {
    epdCmd(0x12);  // SW reset
    delay(10);
    if (!epdWaitBusy(5000)) return;

    epdCmd(0x01);  // driver output control: gate count
    epdData((EPD_HEIGHT - 1) & 0xFF);
    epdData(((EPD_HEIGHT - 1) >> 8) & 0x03);
    epdData(0x00);

    epdCmd(0x11);  // data entry mode: X+ / Y+ / address counter Y
    epdData(0x03);

    epdSetWindow();

    epdCmd(0x3C);  // border waveform
    epdData(0x05);
    epdCmd(0x18);  // internal temperature sensor
    epdData(0x80);
    epdCmd(0x21);  // display update control 1: use RAM content
    epdData(0x00);
    epdData(0x80);

    delay(10);
    epdWaitBusy(5000);
}

static void epdInitUc8151() {
    epdCmd(0x00);  // PSR: LUT from OTP, black/white, scan up
    epdData(0x1F);
    epdData(0x0D);
    epdData(0x02);
    epdData(0x00);

    epdCmd(0x50);  // VCOM / data interval
    epdData(0x97);
    epdData(0x97);

    epdCmd(0x01);  // power setting
    epdData(0x0F);
    epdData(0x0F);
    epdData(0x2F);
    epdData(0x2F);
    epdData(0x07);

    epdCmd(0x06);  // booster soft start
    epdData(0x07);
    epdData(0x07);
    epdData(0x17);

    epdCmd(0x61);  // resolution (TRES): HRES[9:0], VRES[9:0]
    epdData(EPD_WIDTH & 0xFF);
    epdData((EPD_WIDTH >> 8) & 0x03);
    epdData(EPD_HEIGHT & 0xFF);
    epdData((EPD_HEIGHT >> 8) & 0x03);

    epdCmd(0x04);  // power on (booster startup)
    delay(100);
    epdWaitBusy(5000);
}

static void epdInitItc() {
    // Pervasive iTC, film J (black/white/red), 2.66" small-size application note
    epdCmdData(0x00, 0x0E);  // soft reset
    delay(5);

    epdCmdData(0xE5, 25);    // temperature: 0 C = 0x00, 22 C = 0x16, 25 C = 0x19
    epdCmdData(0xE0, 0x02);  // use the temperature above

    // PSR. These are the values Pervasive uses for small C/J panels; reading
    // them from OTP requires a 3-wire (bidirectional SDA) read we skip here.
    static const uint8_t psr[2] = {0xCF, 0x8D};
    epdCmdBuf(0x00, psr, sizeof(psr));

#if EPD_ITC_POWER_SETTINGS
    static const uint8_t pwr[5] = {0x0F, 0x0F, 0x2F, 0x2F, 0x07};
    epdCmdBuf(0x01, pwr, sizeof(pwr));  // power setting
    static const uint8_t btst[3] = {0x07, 0x07, 0x17};
    epdCmdBuf(0x06, btst, sizeof(btst));  // booster soft start
    Serial.println("[epd] explicit power settings sent");
#endif
}

static void epdInit() {
    epdReset();
#if EPD_CONTROLLER == 1
    epdInitSsd16xx();
    Serial.println("[epd] SSD16xx init done");
#elif EPD_CONTROLLER == 2
    epdInitUc8151();
    Serial.println("[epd] UC8151 init done");
#else
    epdInitItc();
    Serial.println("[epd] Pervasive iTC init done");
#endif
}

// --------------------------------------------------------------------------
// Frame buffer helpers
// --------------------------------------------------------------------------
enum EpdColour { EPD_WHITE, EPD_BLACK, EPD_RED };

static void fbClear(EpdColour colour) {
#if EPD_INVERT
    if (colour == EPD_WHITE) colour = EPD_BLACK;
    else if (colour == EPD_BLACK) colour = EPD_WHITE;
#endif
#if EPD_CONTROLLER == 3
    // black plane: 1 = white, 0 = black. red plane: 1 = red, 0 = no red.
    memset(frame, colour == EPD_WHITE ? 0xFF : 0x00, sizeof(frame));
    memset(redFrame, colour == EPD_RED ? 0xFF : 0x00, sizeof(redFrame));
#else
    memset(frame, colour == EPD_WHITE ? 0xFF : 0x00, sizeof(frame));
#endif
}

static inline void fbSet(int x, int y, EpdColour colour) {
    if (x < 0 || y < 0 || x >= EPD_WIDTH || y >= EPD_HEIGHT) return;
#if EPD_INVERT
    if (colour == EPD_WHITE) colour = EPD_BLACK;
    else if (colour == EPD_BLACK) colour = EPD_WHITE;
#endif
    uint8_t mask;
    const size_t i = fbIndex(x, y, mask);
#if EPD_CONTROLLER == 3
    if (colour == EPD_WHITE) {
        frame[i] |= mask;
        redFrame[i] &= ~mask;
    } else if (colour == EPD_BLACK) {
        frame[i] &= ~mask;
        redFrame[i] &= ~mask;
    } else {
        frame[i] &= ~mask;
        redFrame[i] |= mask;
    }
#else
    if (colour == EPD_BLACK) frame[i] &= ~mask;
#endif
}

static void fbTestPattern() {
    fbClear(EPD_WHITE);
    for (int y = 0; y < EPD_HEIGHT; y++)
        for (int x = 0; x < EPD_WIDTH; x++) {
            const bool border = (x < 3 || y < 3 || x >= EPD_WIDTH - 3 || y >= EPD_HEIGHT - 3);
            const bool checker = (((x - 3) / 16 + (y - 3) / 16) & 1) && y < EPD_HEIGHT / 2;
            const bool band = (y >= EPD_HEIGHT / 2 && y < EPD_HEIGHT / 2 + 16);
            const bool stripes = (x >= EPD_WIDTH / 2) && (y >= EPD_HEIGHT * 3 / 4) &&
                                 (((x - EPD_WIDTH / 2) / 6) & 1);
            if (border || checker || stripes) fbSet(x, y, EPD_BLACK);
            if (band) fbSet(x, y, EPD_RED);
        }
}

// --------------------------------------------------------------------------
// Simple vector helpers and text, for the demo screen
// --------------------------------------------------------------------------
static void drawChar(int x, int y, char c, EpdColour colour, int scale) {
    if (c < 32 || c > 126) c = '?';
    const uint8_t* glyph = FONT5X7[c - 32];
    for (int col = 0; col < 5; col++)
        for (int row = 0; row < 7; row++)
            if (glyph[col] & (1 << row))
                for (int sy = 0; sy < scale; sy++)
                    for (int sx = 0; sx < scale; sx++)
                        fbSet(x + col * scale + sx, y + row * scale + sy, colour);
}

static void drawText(int x, int y, const char* text, EpdColour colour, int scale) {
    for (; *text; text++, x += 6 * scale) drawChar(x, y, *text, colour, scale);
}

static void drawDisc(int cx, int cy, int r, EpdColour colour) {
    for (int y = -r; y <= r; y++)
        for (int x = -r; x <= r; x++)
            if (x * x + y * y <= r * r) fbSet(cx + x, cy + y, colour);
}

static void drawRing(int cx, int cy, int r, int thickness, EpdColour colour) {
    const int inner = (r - thickness) * (r - thickness);
    for (int y = -r; y <= r; y++)
        for (int x = -r; x <= r; x++) {
            const int d = x * x + y * y;
            if (d <= r * r && d >= inner) fbSet(cx + x, cy + y, colour);
        }
}

static void drawSmiley(int cx, int cy, int r) {
    drawRing(cx, cy, r, 4, EPD_RED);
    drawDisc(cx - r / 3, cy - r / 3, r / 9, EPD_BLACK);
    drawDisc(cx + r / 3, cy - r / 3, r / 9, EPD_BLACK);
    for (int a = 25; a <= 155; a++) {
        const float t = a * PI / 180.0f;
        drawDisc(cx + (int)(0.6f * r * cosf(t)), cy + (int)(0.6f * r * sinf(t)), 2,
                 EPD_RED);
    }
}

static void fbGreeting() {
    fbClear(EPD_WHITE);
    drawText(10, 14, "Hello from", EPD_BLACK, 2);
    drawText(10, 38, "DeepSeek", EPD_BLACK, 2);
    drawText(10, 62, "@brnomt", EPD_RED, 2);
    drawSmiley(222, 76, 54);
}

// Left half black, right half white, plus a black square on the white half.
// Tells a correct column-major layout from a transposed or mirrored one.
static void fbCalibration() {
    fbClear(EPD_WHITE);
    for (int y = 0; y < EPD_HEIGHT; y++)
        for (int x = 0; x < EPD_WIDTH / 2; x++) fbSet(x, y, EPD_BLACK);
    for (int y = 10; y < 40; y++)
        for (int x = EPD_WIDTH - 40; x < EPD_WIDTH - 10; x++) fbSet(x, y, EPD_BLACK);
}

// Orientation markers: black bar along the top edge, black bar along the left
// edge, and a big "F" in the middle. The corner where the bars meet tells the
// orientation unambiguously, the F tells mirroring.
static void fbMarker() {
    fbClear(EPD_WHITE);
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < EPD_WIDTH; x++) fbSet(x, y, EPD_BLACK);
    for (int y = 0; y < EPD_HEIGHT; y++)
        for (int x = 0; x < 8; x++) fbSet(x, y, EPD_BLACK);
    drawChar(EPD_WIDTH / 2 - 24, EPD_HEIGHT / 2 - 28, 'F', EPD_BLACK, 8);
}

// Four big numerals, one per corner: 1 TL, 2 TR, 3 BL, 4 BR.
static void fbQuadrants() {
    fbClear(EPD_WHITE);
    drawChar(12, 10, '1', EPD_BLACK, 7);
    drawChar(EPD_WIDTH - 47, 10, '2', EPD_BLACK, 7);
    drawChar(12, EPD_HEIGHT - 60, '3', EPD_BLACK, 7);
    drawChar(EPD_WIDTH - 47, EPD_HEIGHT - 60, '4', EPD_BLACK, 7);
}

// Big text test: same string at two scales.
static void fbTextScales() {
    fbClear(EPD_WHITE);
    drawText(10, 8, "Hello", EPD_BLACK, 4);
    drawText(10, 50, "abcde", EPD_BLACK, 4);
    drawText(10, 100, "Hello abcde", EPD_BLACK, 2);
}

// Lowercase alphabet, to find where glyphs stop matching.
static void fbAlphabet() {
    fbClear(EPD_WHITE);
    drawText(6, 10, "abcdefghij", EPD_BLACK, 3);
    drawText(6, 55, "klmnopqrst", EPD_BLACK, 3);
    drawText(6, 100, "uvwxyz", EPD_BLACK, 3);
}

// --------------------------------------------------------------------------
// Push frame + refresh
// --------------------------------------------------------------------------
static void epdShowFrame(bool doubleRefresh = false) {
    const uint32_t t0 = millis();

#if EPD_BUILD_TAG > 0
    {
        char tag[8];
        snprintf(tag, sizeof(tag), "%d", EPD_BUILD_TAG);
        const int len = (int)strlen(tag);
        drawText(EPD_WIDTH - 6 - len * 18, 2, tag, EPD_BLACK, 3);
    }
#endif

#if EPD_CONTROLLER == 3
    for (int pass = 0; pass < (doubleRefresh ? 2 : 1); pass++) {
        epdCmdBuf(0x10, frame, sizeof(frame));        // black plane
        epdCmdBuf(0x13, redFrame, sizeof(redFrame));  // red plane

#if EPD_IGNORE_BUSY
        // Blind sequence: fire the refresh and trust the timing, not BUSY.
        epdCmd(0x04);  // DC/DC on
        delay(200);
        epdCmd(0x12);  // refresh
        for (int i = 0; i < EPD_BLIND_MS / 1000; i++) {
            delay(1000);
            if ((i & 1) == 1)
                Serial.printf("[epd] refresh %2ds BUSY=%d\n", i + 1,
                              digitalRead(EPD_PIN_BUSY));
        }
        epdCmd(0x02);  // DC/DC off
        delay(50);
#else
        epdWaitBusy(5000);
        epdCmd(0x04);  // DC/DC on, can take tens of seconds on a weak supply
        if (!epdWaitBusy(90000)) {
            Serial.println("[epd] DC/DC never came up - check VDD and the booster caps");
            epdCmd(0x02);
            return;
        }
        epdCmd(0x12);  // display refresh, BWR takes ~22 s
        delay(5);
        if (!epdWaitBusy(45000)) return;
        epdCmd(0x02);  // DC/DC off
        epdWaitBusy(5000);
#endif
    }
#elif EPD_CONTROLLER == 1
    // Full update uses the OTP LUT: both RAM banks must be written.
    for (int pass = 0; pass < (doubleRefresh ? 2 : 1); pass++) {
        epdSetWindow();
        epdCmd(0x24);  // write RAM B/W
        epdData(frame, sizeof(frame));
        epdSetWindow();
        epdCmd(0x26);  // write RAM red (unused in B/W, keeps LUT happy)
        epdData(frame, sizeof(frame));
        epdCmd(0x22);  // display update control 2: full refresh
        epdData(0xF7);
        epdCmd(0x20);  // master activation
        if (!epdWaitBusy(30000)) return;
    }
#else
    static uint8_t packed[EPD_CONT_BYTES];
    epdPackContinuous(packed);

    for (int pass = 0; pass < (doubleRefresh ? 2 : 1); pass++) {
        epdCmd(0x04);  // power on
        delay(50);
        epdWaitBusy(5000);

        epdCmd(0x13);  // DTM2 - new data
        epdData(packed, sizeof(packed));
        epdCmd(0x10);  // DTM1 - previous data
        epdData(packed, sizeof(packed));

        epdCmd(0x12);  // display refresh
        delay(50);
        if (!epdWaitBusy(30000)) return;

        epdCmd(0x02);  // power off
        epdWaitBusy(5000);
    }
#endif

    Serial.printf("[epd] refresh %lu ms\n", (unsigned long)(millis() - t0));
}

// The image stays on the panel with DC/DC off. Call epdInit() before the next
// update if you also cut power to the panel.
static void epdHibernate() {
#if EPD_CONTROLLER == 1
    epdCmd(0x10);
    epdData(0x01);
#elif EPD_CONTROLLER == 2
    epdCmd(0x07);
    epdData(0xA5);
#else
    epdCmd(0x02);
#endif
    epdWaitBusy(5000);
    Serial.println("[epd] DC/DC off, image retained");
}

// Serial 't': trace the BUSY line while the DC/DC starts up. A healthy panel
// pulses BUSY and releases it; a panel with a dead booster stays low.
static void epdPowerTrace() {
    Serial.printf("[epd] BUSY idle=%d\n", digitalRead(EPD_PIN_BUSY));
    epdCmd(0x04);
    const uint32_t t0 = millis();
    while (millis() - t0 < 20000) {
        Serial.printf("[epd] +%4lu ms BUSY=%d\n", (unsigned long)(millis() - t0),
                      digitalRead(EPD_PIN_BUSY));
        delay(500);
    }
    epdCmd(0x02);
    Serial.println("[epd] trace done, 0x02 sent");
}

#if EPD_CONTROLLER == 3
// Sample BUSY and print only transitions, so we can see whether the panel
// reacts to our commands at all, or whether something else drives it.
static void epdBusyWatch(uint32_t ms, const char* label) {
    int last = digitalRead(EPD_PIN_BUSY);
    Serial.printf("[epd] %s: BUSY=%d\n", label, last);
    const uint32_t t0 = millis();
    while (millis() - t0 < ms) {
        const int cur = digitalRead(EPD_PIN_BUSY);
        if (cur != last) {
            Serial.printf("[epd] %s: BUSY=%d at +%lu ms\n", label, cur,
                          (unsigned long)(millis() - t0));
            last = cur;
        }
        delayMicroseconds(200);
    }
}

// Boot diagnostic: one step at a time, printing what BUSY does. A working
// panel goes busy for a few ms on soft reset and for a few hundred ms when the
// DC/DC starts, then releases BUSY to HIGH.
static void epdItcDiagnostics() {
    Serial.printf("[epd] BUSY before init = %d\n", digitalRead(EPD_PIN_BUSY));
    epdInit();  // reset + soft reset + temperature + PSR (+ power settings)
    epdBusyWatch(1000, "after init");
    epdBusyWatch(3000, "idle, no commands");  // catches a foreign master

    epdCmd(0x04);  // DC/DC on
    epdBusyWatch(15000, "dc-dc");
    epdCmd(0x02);
    Serial.println("[epd] diagnostics done");
}

// Toggle one line at 1 Hz so it can be followed with a multimeter at the panel
// connector. Used to verify continuity after rework.
static void epdWigglePin(int pin, const char* name) {
    pinMode(pin, OUTPUT);
    for (int i = 0; i < 3; i++) {
        digitalWrite(pin, HIGH);
        Serial.printf("[wiggle] %-3s GPIO%d = HIGH\n", name, pin);
        delay(500);
        digitalWrite(pin, LOW);
        Serial.printf("[wiggle] %-3s GPIO%d = LOW\n", name, pin);
        delay(500);
    }
    digitalWrite(pin, HIGH);
}

static void epdWiggle() {
    Serial.println("[wiggle] start");
    SPI.end();
    epdWigglePin(EPD_PIN_CS, "CS");
    epdWigglePin(EPD_PIN_DC, "DC");
    epdWigglePin(EPD_PIN_RST, "RST");
    epdWigglePin(EPD_PIN_SCK, "SCK");
    epdWigglePin(EPD_PIN_MOSI, "SDA");

    pinMode(EPD_PIN_CS, OUTPUT);
    digitalWrite(EPD_PIN_CS, HIGH);
    pinMode(EPD_PIN_DC, OUTPUT);
    digitalWrite(EPD_PIN_DC, HIGH);
    pinMode(EPD_PIN_RST, OUTPUT);
    digitalWrite(EPD_PIN_RST, HIGH);
    SPI.begin(EPD_PIN_SCK, EPD_PIN_MISO, EPD_PIN_MOSI, -1);
    Serial.println("[wiggle] done");
}
#endif

// --------------------------------------------------------------------------
// Arduino entry points
// --------------------------------------------------------------------------
static void epdPrintConfig() {
    Serial.printf("[epd] SCK=%d MOSI=%d CS=%d DC=%d RST=%d BUSY=%d\n",
                  EPD_PIN_SCK, EPD_PIN_MOSI, EPD_PIN_CS, EPD_PIN_DC,
                  EPD_PIN_RST, EPD_PIN_BUSY);
    Serial.printf("[epd] panel %dx%d, controller %d, bus %lu Hz\n",
                  EPD_WIDTH, EPD_HEIGHT, EPD_CONTROLLER, (unsigned long)EPD_SPI_HZ);
    Serial.printf("[epd] flipX=%d powerPin=%d\n", EPD_FLIP_X, EPD_PIN_PWR);
    Serial.printf("[epd] build tag = %d\n", EPD_BUILD_TAG);
    Serial.printf("[epd] font a = %02X %02X %02X %02X %02X\n", FONT5X7['a' - 32][0],
                  FONT5X7['a' - 32][1], FONT5X7['a' - 32][2], FONT5X7['a' - 32][3],
                  FONT5X7['a' - 32][4]);
    Serial.printf("[epd] font z = %02X %02X %02X %02X %02X\n", FONT5X7['z' - 32][0],
                  FONT5X7['z' - 32][1], FONT5X7['z' - 32][2], FONT5X7['z' - 32][3],
                  FONT5X7['z' - 32][4]);
    pinMode(EPD_PIN_BUSY, INPUT);
    delay(2);
    const int floating = digitalRead(EPD_PIN_BUSY);
    pinMode(EPD_PIN_BUSY, INPUT_PULLUP);
    delay(2);
    const int pulledUp = digitalRead(EPD_PIN_BUSY);
    pinMode(EPD_PIN_BUSY, INPUT_PULLDOWN);
    delay(2);
    const int pulledDown = digitalRead(EPD_PIN_BUSY);
    Serial.printf("[epd] BUSY float=%d pullup=%d pulldown=%d\n", floating, pulledUp,
                  pulledDown);
}

void setup() {
    Serial.begin(115200);
    delay(300);

#if EPD_PIN_PWR >= 0
    // Keep the panel unpowered while the ESP boots, then switch it on.
    pinMode(EPD_PIN_PWR, OUTPUT);
    digitalWrite(EPD_PIN_PWR, EPD_PWR_ON_LEVEL == LOW ? HIGH : LOW);
    delay(100);
#endif

    pinMode(EPD_PIN_CS, OUTPUT);
    digitalWrite(EPD_PIN_CS, HIGH);
    pinMode(EPD_PIN_DC, OUTPUT);
    digitalWrite(EPD_PIN_DC, HIGH);
    pinMode(EPD_PIN_RST, OUTPUT);
    digitalWrite(EPD_PIN_RST, HIGH);

    SPI.begin(EPD_PIN_SCK, EPD_PIN_MISO, EPD_PIN_MOSI, -1);

    epdPrintConfig();
#if EPD_PIN_PWR >= 0
    // Panel supply on, only now that the ESP is up and the bus is idle-high.
    digitalWrite(EPD_PIN_PWR, EPD_PWR_ON_LEVEL);
    delay(300);
#endif
#if EPD_BOOT_WIGGLE && EPD_CONTROLLER == 3
    epdWiggle();
    return;
#endif
#if EPD_BOOT_DIAG && EPD_CONTROLLER == 3
    pinMode(EPD_PIN_BUSY, INPUT_PULLUP);
    epdItcDiagnostics();
    return;
#endif
#if EPD_CONTROLLER == 3
    // iTC BUSY is open-drain on most panels: it needs a pull-up to read ready.
    pinMode(EPD_PIN_BUSY, INPUT_PULLUP);
    epdInit();
#if EPD_BOOT_DEMO == 2
    fbClear(EPD_BLACK);
    epdShowFrame();
    Serial.println("[epd] black frame drawn");
#elif EPD_BOOT_DEMO == 7
    fbAlphabet();
    epdShowFrame();
    Serial.println("[epd] alphabet drawn");
#elif EPD_BOOT_DEMO == 6
    fbTextScales();
    epdShowFrame();
    Serial.println("[epd] text scales drawn");
#elif EPD_BOOT_DEMO == 5
    fbQuadrants();
    epdShowFrame();
    Serial.println("[epd] quadrants drawn");
#elif EPD_BOOT_DEMO == 4
    fbMarker();
    epdShowFrame();
    Serial.println("[epd] markers drawn");
#elif EPD_BOOT_DEMO == 1
    fbCalibration();
    epdShowFrame();
    Serial.println("[epd] calibration drawn");
#else
    fbGreeting();
    epdShowFrame();
    Serial.println("[epd] greeting drawn");
#endif
#else
    pinMode(EPD_PIN_BUSY, INPUT);
    epdInit();

#if EPD_BOOT_DEMO == 2
    fbClear(EPD_BLACK);
#elif EPD_BOOT_DEMO == 1
    fbCalibration();
#else
    fbTestPattern();
#endif
    epdShowFrame();
    Serial.println("[epd] frame drawn");
#endif
}

// Serial console: 'w' white, 'k' black, 'r' red, 'p' pattern, 'i' re-init,
// 't' booster trace, 's' DC/DC off
void loop() {
    static uint32_t lastBeat = 0;

    if (!Serial.available()) {
        delay(20);
        if (millis() - lastBeat > 5000) {
            lastBeat = millis();
            Serial.printf("[hb] %lu ms BUSY=%d\n", (unsigned long)millis(),
                          digitalRead(EPD_PIN_BUSY));
        }
        return;
    }

    lastBeat = millis();
    switch (Serial.read()) {
        case 'w': fbClear(EPD_WHITE); epdShowFrame(); break;
        case 'k': fbClear(EPD_BLACK); epdShowFrame(); break;
        case 'r': fbClear(EPD_RED); epdShowFrame(); break;
        case 'p': fbTestPattern(); epdShowFrame(); break;
        case 'e': fbGreeting(); epdShowFrame(); break;
        case 'i': epdInit(); break;
        case 't': epdPowerTrace(); break;
#if EPD_CONTROLLER == 3
        case 'g': epdWiggle(); break;
        case 'd': epdItcDiagnostics(); break;
#endif
        case 's': epdHibernate(); break;
        default: break;
    }
}
