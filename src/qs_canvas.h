// QuasarUI - RGB565 framebuffer with dirty-rectangle tracking.
// Draw anywhere, call flush(): only the changed region goes over SPI.
// Screen::update() flushes per widget so distant updates stay separate.
#pragma once
#include "qs_panel.h"

namespace qs {

typedef uint16_t Color;  // RGB565, native byte order in RAM

constexpr Color rgb(uint8_t r, uint8_t g, uint8_t b) {
    return Color(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

namespace colors {
constexpr Color Black = 0x0000, White = 0xFFFF;
constexpr Color Red = rgb(255, 60, 60), Green = rgb(60, 220, 110), Blue = rgb(70, 140, 255);
constexpr Color Yellow = rgb(255, 210, 60), Cyan = rgb(60, 220, 230), Orange = rgb(255, 150, 40);
constexpr Color Grey = rgb(120, 126, 138), DarkGrey = rgb(34, 38, 48), Panel = rgb(16, 19, 28);
constexpr Color Accent = rgb(124, 92, 255);
}  // namespace colors

struct Rect {
    int16_t x, y, w, h;
    bool empty() const { return w <= 0 || h <= 0; }
};

class Canvas {
public:
    // `buffer` must hold width*height Colors. Use Canvas::bytesFor() to size it.
    Canvas(Color* buffer, uint16_t width, uint16_t height)
        : fb_(buffer), w_(width), h_(height) {}
    static constexpr size_t bytesFor(uint16_t w, uint16_t h) { return size_t(w) * h * sizeof(Color); }

    uint16_t width() const { return w_; }
    uint16_t height() const { return h_; }
    const Color* pixels() const { return fb_; }

    // --- primitives (all clip; out-of-range is safe) ---
    void clear(Color c) { fillRect(0, 0, w_, h_, c); }
    void drawPixel(int16_t x, int16_t y, Color c);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, Color c);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, Color c);
    void hline(int16_t x, int16_t y, int16_t w, Color c) { fillRect(x, y, w, 1, c); }
    void vline(int16_t x, int16_t y, int16_t h, Color c) { fillRect(x, y, 1, h, c); }
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, Color c);
    void fillCircle(int16_t cx, int16_t cy, int16_t r, Color c);
    void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, Color c);
    // Ring segment between angles a0..a1 (degrees, 0 = 12 o'clock, clockwise).
    void fillArc(int16_t cx, int16_t cy, int16_t rIn, int16_t rOut, int16_t a0, int16_t a1, Color c);

    // --- text: built-in 5x7 font, integer scale ---
    int16_t drawChar(int16_t x, int16_t y, char ch, Color fg, Color bg, uint8_t scale = 1, bool transparent = false);
    int16_t drawText(int16_t x, int16_t y, const char* s, Color fg, Color bg, uint8_t scale = 1, bool transparent = false);
    static int16_t textWidth(const char* s, uint8_t scale = 1);
    static int16_t textHeight(uint8_t scale = 1) { return int16_t(8 * scale); }  // 7 rows + descender row

    // --- dirty tracking / output ---
    Rect dirty() const { return dirty_; }
    void markDirty(int16_t x, int16_t y, int16_t w, int16_t h);
    void invalidateAll() { markDirty(0, 0, w_, h_); }
    // Sends the dirty rect to the panel and clears it. Returns pixels sent.
    uint32_t flush(Panel& panel);

private:
    Color* fb_;
    uint16_t w_, h_;
    Rect dirty_ = {0, 0, 0, 0};
};

}  // namespace qs
