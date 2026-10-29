// Host simulator: a Bus that behaves like an ST7789/ILI9341 RAM window.
// Decodes CASET/RASET/RAMWR so tests can assert what really lands on "glass".
#pragma once
#include "../src/qs_bus.h"
#include <stdio.h>
#include <vector>

class SimBus : public qs::Bus {
public:
    SimBus(int w, int h) : W(w), H(h), vram(size_t(w) * h, 0) {}

    bool begin() override { return true; }
    void reset() override { ++resets; }
    void command(uint8_t c) override {
        cmd_ = c; argIdx_ = 0; ++cmdCount;
        if (c == 0x2C) { cx_ = x0_; cy_ = y0_; }
        if (c == 0x2C) ++ramwr;
    }
    void data(const uint8_t* p, size_t n) override {
        bytesSent += n;
        for (size_t i = 0; i < n; ++i) feed(p[i]);
    }
    void delayMs(uint32_t) override {}

    void feed(uint8_t b) {
        switch (cmd_) {
            case 0x2A: arg(b, x0_, x1_); break;
            case 0x2B: arg(b, y0_, y1_); break;
            case 0x36: madctl = b; break;
            case 0x2C: ramByte(b); break;
            default: break;
        }
    }
    void savePPM(const char* path, int scale = 1) const {
        FILE* f = fopen(path, "wb");
        if (!f) return;
        fprintf(f, "P6\n%d %d\n255\n", W * scale, H * scale);
        for (int y = 0; y < H * scale; ++y)
            for (int x = 0; x < W * scale; ++x) {
                const uint16_t c = vram[size_t(y / scale) * W + x / scale];
                const uint8_t rgb[3] = {uint8_t((c >> 8) & 0xF8), uint8_t((c >> 3) & 0xFC), uint8_t((c << 3) & 0xF8)};
                fwrite(rgb, 1, 3, f);
            }
        fclose(f);
    }

    int W, H;
    std::vector<uint16_t> vram;
    size_t bytesSent = 0, cmdCount = 0, ramwr = 0, resets = 0;
    uint8_t madctl = 0;
    int writesOutsideWindow = 0;

private:
    void arg(uint8_t b, int& lo, int& hi) {
        // 4 args: lo_hi lo_lo hi_hi hi_lo
        if (argIdx_ == 0) lo = b << 8;
        else if (argIdx_ == 1) lo |= b;
        else if (argIdx_ == 2) hi = b << 8;
        else if (argIdx_ == 3) hi |= b;
        ++argIdx_;
    }
    void ramByte(uint8_t b) {
        if (!half_) { hi_ = b; half_ = true; return; }
        half_ = false;
        const uint16_t px = uint16_t(hi_ << 8 | b);
        if (cx_ < W && cy_ < H) vram[size_t(cy_) * W + cx_] = px;
        else ++writesOutsideWindow;
        if (++cx_ > x1_) { cx_ = x0_; ++cy_; }
    }
    uint8_t cmd_ = 0;
    int argIdx_ = 0;
    int x0_ = 0, x1_ = 0, y0_ = 0, y1_ = 0, cx_ = 0, cy_ = 0;
    uint8_t hi_ = 0;
    bool half_ = false;
};
