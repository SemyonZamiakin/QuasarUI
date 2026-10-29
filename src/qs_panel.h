// QuasarUI - SPI TFT panel controller driver (ST7789, ILI9341).
#pragma once
#include "qs_bus.h"

namespace qs {

enum class Controller : uint8_t { ST7789, ILI9341 };

struct PanelConfig {
    Controller controller;
    uint16_t width;      // native (rotation 0) size
    uint16_t height;
    bool invert;         // IPS ST7789 modules usually need INVON
    bool offset240;      // 240x240 ST7789 glass is mapped inside a 240x320 RAM

    static PanelConfig ST7789_240x240() { return {Controller::ST7789, 240, 240, true, true}; }
    static PanelConfig ST7789_240x320() { return {Controller::ST7789, 240, 320, true, false}; }
    static PanelConfig ILI9341_240x320() { return {Controller::ILI9341, 240, 320, false, false}; }
};

class Panel {
public:
    Panel(Bus& bus, const PanelConfig& cfg) : bus_(bus), cfg_(cfg), w_(cfg.width), h_(cfg.height) {}

    bool begin();                              // reset + init sequence; false if the bus fails
    void setRotation(uint8_t r);               // 0..3 (90 degree steps)
    void sleep(bool enter);                    // controller sleep in/out (backlight is yours)
    uint16_t width() const { return w_; }      // current (rotated) size
    uint16_t height() const { return h_; }

    // Open a RAM window; follow with pushPixels() for exactly w*h pixels.
    void setWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    // Send big-endian RGB565 bytes (2 bytes per pixel).
    void pushBytes(const uint8_t* p, size_t n) { bus_.data(p, n); }

private:
    void cmd(uint8_t c) { bus_.command(c); }
    void cmdData(uint8_t c, const uint8_t* d, size_t n) { bus_.command(c); bus_.data(d, n); }

    Bus& bus_;
    PanelConfig cfg_;
    uint16_t w_, h_;
    uint16_t xOff_ = 0, yOff_ = 0;
};

}  // namespace qs
