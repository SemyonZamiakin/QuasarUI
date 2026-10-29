// QuasarUI - transport abstraction.
// The whole library talks to the display through this tiny interface, so the
// core is portable (Arduino, ESP-IDF, host simulator) and unit-testable.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace qs {

class Bus {
public:
    virtual ~Bus() {}
    virtual bool begin() = 0;                              // configure pins / SPI
    virtual void reset() = 0;                              // hardware reset pulse (no-op if no RST pin)
    virtual void command(uint8_t cmd) = 0;                 // DC low
    virtual void data(const uint8_t* p, size_t n) = 0;     // DC high
    virtual void delayMs(uint32_t ms) = 0;

    void data8(uint8_t v) { data(&v, 1); }
};

}  // namespace qs
