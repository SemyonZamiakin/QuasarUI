// QuasarUI - Arduino SPI transport (AVR/ESP32/ESP8266/RP2040/STM32 cores).
#pragma once
#if defined(ARDUINO)
#include <Arduino.h>
#include <SPI.h>
#include "qs_bus.h"

namespace qs {

class ArduinoSpiBus : public Bus {
public:
    // cs/rst may be -1 when tied to GND / VCC. dc is required.
    ArduinoSpiBus(SPIClass& spi, int8_t cs, int8_t dc, int8_t rst, uint32_t hz = 40000000UL)
        : spi_(spi), cs_(cs), dc_(dc), rst_(rst), hz_(hz) {}

    bool begin() override {
        if (dc_ < 0) return false;
        pinMode(dc_, OUTPUT);
        if (cs_ >= 0) { pinMode(cs_, OUTPUT); digitalWrite(cs_, HIGH); }
        if (rst_ >= 0) pinMode(rst_, OUTPUT);
        spi_.begin();
        return true;
    }
    void reset() override {
        if (rst_ < 0) return;
        digitalWrite(rst_, HIGH); delay(10);
        digitalWrite(rst_, LOW);  delay(20);
        digitalWrite(rst_, HIGH); delay(120);
    }
    void command(uint8_t c) override { send(c, LOW); }
    void data(const uint8_t* p, size_t n) override {
        if (!n) return;
        digitalWrite(dc_, HIGH);
        begin_();
#if defined(ESP32) || defined(ESP8266)
        spi_.writeBytes(p, n);                               // hardware burst, no RX
#else
        for (size_t i = 0; i < n; ++i) spi_.transfer(p[i]);  // portable path
#endif
        end_();
    }
    void delayMs(uint32_t ms) override { delay(ms); }

private:
    void begin_() {
        spi_.beginTransaction(SPISettings(hz_, MSBFIRST, SPI_MODE0));
        if (cs_ >= 0) digitalWrite(cs_, LOW);
    }
    void end_() {
        if (cs_ >= 0) digitalWrite(cs_, HIGH);
        spi_.endTransaction();
    }
    void send(uint8_t v, uint8_t dc) {
        digitalWrite(dc_, dc);
        begin_();
        spi_.transfer(v);
        end_();
    }
    SPIClass& spi_;
    int8_t cs_, dc_, rst_;
    uint32_t hz_;
};

}  // namespace qs
#endif  // ARDUINO
