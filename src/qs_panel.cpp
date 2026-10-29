#include "qs_panel.h"

namespace qs {

namespace {
enum : uint8_t {
    SWRESET = 0x01, SLPIN = 0x10, SLPOUT = 0x11, NORON = 0x13, INVOFF = 0x20, INVON = 0x21,
    DISPON = 0x29, CASET = 0x2A, RASET = 0x2B, RAMWR = 0x2C, MADCTL = 0x36, COLMOD = 0x3A
};

struct Step { uint8_t cmd; uint8_t n; uint8_t d[15]; };

// ILI9341 init (well-known vendor sequence), 16-bit colour.
const Step kIli[] = {
    {0xEF, 3, {0x03, 0x80, 0x02}},
    {0xCF, 3, {0x00, 0xC1, 0x30}},
    {0xED, 4, {0x64, 0x03, 0x12, 0x81}},
    {0xE8, 3, {0x85, 0x00, 0x78}},
    {0xCB, 5, {0x39, 0x2C, 0x00, 0x34, 0x02}},
    {0xF7, 1, {0x20}},
    {0xEA, 2, {0x00, 0x00}},
    {0xC0, 1, {0x23}},
    {0xC1, 1, {0x10}},
    {0xC5, 2, {0x3E, 0x28}},
    {0xC7, 1, {0x86}},
    {0x37, 1, {0x00}},
    {COLMOD, 1, {0x55}},
    {0xB1, 2, {0x00, 0x18}},
    {0xB6, 3, {0x08, 0x82, 0x27}},
    {0xF2, 1, {0x00}},
    {0x26, 1, {0x01}},
    {0xE0, 15, {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00}},
    {0xE1, 15, {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F}},
};
}  // namespace

bool Panel::begin() {
    if (!bus_.begin()) return false;
    bus_.reset();
    cmd(SWRESET);
    bus_.delayMs(150);

    if (cfg_.controller == Controller::ILI9341) {
        for (const Step& s : kIli) cmdData(s.cmd, s.d, s.n);
    } else {  // ST7789
        const uint8_t colmod = 0x55;  // 16 bit/pixel
        cmdData(COLMOD, &colmod, 1);
        const uint8_t porch[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
        cmdData(0xB2, porch, sizeof porch);
    }
    cmd(cfg_.invert ? INVON : INVOFF);
    cmd(SLPOUT);
    bus_.delayMs(120);
    cmd(NORON);
    cmd(DISPON);
    bus_.delayMs(20);
    setRotation(0);
    return true;
}

void Panel::setRotation(uint8_t r) {
    r &= 3;
    static const uint8_t st[4]  = {0x00, 0x60, 0xC0, 0xA0};
    static const uint8_t ili[4] = {0x48, 0x28, 0x88, 0xE8};
    const uint8_t m = cfg_.controller == Controller::ILI9341 ? ili[r] : st[r];
    cmdData(MADCTL, &m, 1);

    const bool swap = r & 1;
    w_ = swap ? cfg_.height : cfg_.width;
    h_ = swap ? cfg_.width : cfg_.height;
    xOff_ = yOff_ = 0;
    if (cfg_.offset240) {  // 240x240 glass sits at the edge of the 240x320 RAM
        if (r == 2) yOff_ = 80;
        if (r == 3) xOff_ = 80;
    }
}

void Panel::sleep(bool enter) {
    cmd(enter ? SLPIN : SLPOUT);
    bus_.delayMs(120);
}

void Panel::setWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    const uint16_t x0 = x + xOff_, x1 = x0 + w - 1;
    const uint16_t y0 = y + yOff_, y1 = y0 + h - 1;
    const uint8_t c[4] = {uint8_t(x0 >> 8), uint8_t(x0), uint8_t(x1 >> 8), uint8_t(x1)};
    const uint8_t r[4] = {uint8_t(y0 >> 8), uint8_t(y0), uint8_t(y1 >> 8), uint8_t(y1)};
    cmdData(CASET, c, 4);
    cmdData(RASET, r, 4);
    cmd(RAMWR);
}

}  // namespace qs
