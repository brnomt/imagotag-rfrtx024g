#pragma once

#include <stddef.h>
#include <stdint.h>

// Low-level transport: GPIO, hardware SPI and the BUSY handshake. Nothing in
// this module knows about panel commands.
namespace epd {

// Configures the GPIOs, starts SPI and (optionally) switches panel power on.
void halBegin();

// Hardware reset. Timing follows the iTC application note.
void reset();

void cmd(uint8_t command);
void data(const uint8_t* buffer, size_t length);

// Command byte followed by its payload in a single CS frame. The iTC
// controller keys on this framing: splitting it into cmd() + data() makes the
// panel silently ignore the write.
void cmdWithData(uint8_t command, const uint8_t* buffer, size_t length);

// True while the panel is busy. iTC BUSY is open-drain and active low.
bool busy();

// Blocks until BUSY releases. Returns false on timeout.
bool waitReady(uint32_t timeoutMs);

}  // namespace epd
