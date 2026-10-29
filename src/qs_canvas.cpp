#include "qs_canvas.h"
#include "qs_font5x7.h"
#include <math.h>

namespace qs {

namespace {
inline int16_t imin(int16_t a, int16_t b) { return a < b ? a : b; }
inline int16_t imax(int16_t a, int16_t b) { return a > b ? a : b; }
inline int16_t iabs(int16_t a) { return a < 0 ? -a : a; }
// Half-width of a circle row: largest dx with dx*dx + dy*dy <= r*r.
int16_t circleSpan(int16_t r, int16_t dy) {
    const int32_t rem = int32_t(r) * r - int32_t(dy) * dy;
    if (rem < 0) return -1;
    int16_t dx = int16_t(sqrtf(float(rem)) + 0.5f);
    while (int32_t(dx) * dx > rem) --dx;
    return dx;
}
}  // namespace

void Canvas::markDirty(int16_t x, int16_t y, int16_t w, int16_t h) {
    int16_t x1 = imin(x + w, int16_t(w_)), y1 = imin(y + h, int16_t(h_));
    x = imax(x, 0); y = imax(y, 0);
    if (x1 <= x || y1 <= y) return;
    if (dirty_.empty()) { dirty_ = {x, y, int16_t(x1 - x), int16_t(y1 - y)}; return; }
    const int16_t nx0 = imin(x, dirty_.x), ny0 = imin(y, dirty_.y);
    const int16_t nx1 = imax(x1, int16_t(dirty_.x + dirty_.w)), ny1 = imax(y1, int16_t(dirty_.y + dirty_.h));
    dirty_ = {nx0, ny0, int16_t(nx1 - nx0), int16_t(ny1 - ny0)};
}

void Canvas::drawPixel(int16_t x, int16_t y, Color c) {
    if (x < 0 || y < 0 || x >= int16_t(w_) || y >= int16_t(h_)) return;
    fb_[size_t(y) * w_ + x] = c;
    markDirty(x, y, 1, 1);
}

void Canvas::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, Color c) {
    int16_t x1 = imin(x + w, int16_t(w_)), y1 = imin(y + h, int16_t(h_));
    x = imax(x, 0); y = imax(y, 0);
    if (x1 <= x || y1 <= y) return;
    for (int16_t j = y; j < y1; ++j) {
        Color* row = fb_ + size_t(j) * w_;
        for (int16_t i = x; i < x1; ++i) row[i] = c;
    }
    markDirty(x, y, x1 - x, y1 - y);
}

void Canvas::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, Color c) {
    if (w <= 0 || h <= 0) return;
    hline(x, y, w, c);
    hline(x, y + h - 1, w, c);
    vline(x, y, h, c);
    vline(x + w - 1, y, h, c);
}

void Canvas::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, Color c) {
    const int16_t dx = iabs(x1 - x0), dy = -iabs(y1 - y0);
    const int16_t sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int32_t err = dx + dy;
    for (;;) {
        drawPixel(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        const int32_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void Canvas::fillCircle(int16_t cx, int16_t cy, int16_t r, Color c) {
    if (r < 0) return;
    for (int16_t dy = -r; dy <= r; ++dy) {
        const int16_t dx = circleSpan(r, dy);
        fillRect(cx - dx, cy + dy, 2 * dx + 1, 1, c);
    }
}

void Canvas::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, Color c) {
    if (w <= 0 || h <= 0) return;
    r = imin(r, imin(w / 2, h / 2));
    if (r <= 0) { fillRect(x, y, w, h, c); return; }
    fillRect(x, y + r, w, h - 2 * r, c);
    for (int16_t i = 0; i < r; ++i) {
        const int16_t dx = circleSpan(r, r - 1 - i);  // inset shrinks towards the corner
        const int16_t inset = r - dx;
        fillRect(x + inset, y + i, w - 2 * inset, 1, c);
        fillRect(x + inset, y + h - 1 - i, w - 2 * inset, 1, c);
    }
}

void Canvas::fillArc(int16_t cx, int16_t cy, int16_t rIn, int16_t rOut, int16_t a0, int16_t a1, Color c) {
    if (rOut <= 0 || rIn >= rOut) return;
    const bool full = (a1 - a0) >= 360 || (a0 - a1) >= 360;
    int16_t s = int16_t(((a0 % 360) + 360) % 360);
    int16_t sweep = int16_t((((a1 - a0) % 360) + 360) % 360);
    if (!full && sweep == 0) return;
    const int32_t in2 = int32_t(rIn) * rIn, out2 = int32_t(rOut) * rOut;
    const int16_t ya = imax(cy - rOut, 0), yb = imin(cy + rOut + 1, int16_t(h_));
    const int16_t xa = imax(cx - rOut, 0), xb = imin(cx + rOut + 1, int16_t(w_));
    for (int16_t y = ya; y < yb; ++y) {
        for (int16_t x = xa; x < xb; ++x) {
            const int32_t dx = x - cx, dy = y - cy, d2 = dx * dx + dy * dy;
            if (d2 < in2 || d2 > out2) continue;
            if (!full) {
                float a = atan2f(float(dx), float(-dy)) * 57.2957795f;  // 0 = up, clockwise
                if (a < 0) a += 360.0f;
                float rel = a - s;
                if (rel < 0) rel += 360.0f;
                if (rel > sweep) continue;
            }
            fb_[size_t(y) * w_ + x] = c;
        }
    }
    markDirty(xa, ya, xb - xa, yb - ya);
}

int16_t Canvas::drawChar(int16_t x, int16_t y, char ch, Color fg, Color bg, uint8_t scale, bool transparent) {
    const int16_t adv = int16_t(6 * scale);
    if (scale == 0 || x >= int16_t(w_) || y >= int16_t(h_) || x + adv <= 0 || y + 8 * scale <= 0) return adv;
    const uint8_t code = uint8_t(ch);
    const uint8_t* g = (code >= 0x20 && code <= 0x7E) ? kFont5x7[code - 0x20] : kFont5x7['?' - 0x20];
    // Write straight into the framebuffer, mark the cell dirty once.
    for (uint8_t col = 0; col < 6; ++col) {
        const uint8_t bits = col < 5 ? g[col] : 0;
        for (uint8_t row = 0; row < 8; ++row) {
            const bool on = bits & (1u << row);
            if (!on && transparent) continue;
            const Color c = on ? fg : bg;
            for (uint8_t sy = 0; sy < scale; ++sy) {
                const int16_t py = y + row * scale + sy;
                if (py < 0 || py >= int16_t(h_)) continue;
                for (uint8_t sx = 0; sx < scale; ++sx) {
                    const int16_t px = x + col * scale + sx;
                    if (px >= 0 && px < int16_t(w_)) fb_[size_t(py) * w_ + px] = c;
                }
            }
        }
    }
    markDirty(x, y, adv, 8 * scale);
    return adv;
}

int16_t Canvas::drawText(int16_t x, int16_t y, const char* s, Color fg, Color bg, uint8_t scale, bool transparent) {
    int16_t cx = x;
    for (; s && *s; ++s) cx += drawChar(cx, y, *s, fg, bg, scale, transparent);
    return cx;
}

int16_t Canvas::textWidth(const char* s, uint8_t scale) {
    int16_t n = 0;
    for (; s && *s; ++s) ++n;
    return int16_t(n * 6 * scale);
}

uint32_t Canvas::flush(Panel& panel) {
    if (dirty_.empty()) return 0;
    const Rect d = dirty_;
    dirty_ = {0, 0, 0, 0};
    panel.setWindow(d.x, d.y, d.w, d.h);
    uint8_t buf[512];  // 256 pixels per SPI burst
    for (int16_t j = 0; j < d.h; ++j) {
        const Color* row = fb_ + size_t(d.y + j) * w_ + d.x;
        int16_t left = d.w;
        while (left > 0) {
            const int16_t n = imin(left, 256);
            for (int16_t i = 0; i < n; ++i) {
                buf[2 * i] = uint8_t(row[i] >> 8);  // panel wants MSB first
                buf[2 * i + 1] = uint8_t(row[i]);
            }
            panel.pushBytes(buf, size_t(n) * 2);
            row += n;
            left -= n;
        }
    }
    return uint32_t(d.w) * uint32_t(d.h);
}

}  // namespace qs
