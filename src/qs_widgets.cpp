#include "qs_widgets.h"

namespace qs {

namespace {
int32_t clampi(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : (v > hi ? hi : v); }

// itoa without printf/heap. Returns length.
int itoa10(int32_t v, char* out) {
    char tmp[12];
    int n = 0, len = 0;
    uint32_t u = v < 0 ? uint32_t(0) - uint32_t(v) : uint32_t(v);
    do { tmp[n++] = char('0' + u % 10); u /= 10; } while (u);
    if (v < 0) out[len++] = '-';
    while (n) out[len++] = tmp[--n];
    out[len] = 0;
    return len;
}
}  // namespace

// ---------------- Label ----------------
void Label::setText(const char* s) {
    uint8_t i = 0;
    bool same = true;
    for (; s && s[i] && i < kMaxChars; ++i) {
        if (text_[i] != s[i]) same = false;
        text_[i] = s[i];
    }
    if (text_[i] != 0) same = false;
    text_[i] = 0;
    if (!same) invalidate();
}
void Label::setColor(Color fg) { if (fg != fg_) { fg_ = fg; invalidate(); } }
void Label::draw(Canvas& cv) {
    cv.fillRect(bounds_.x, bounds_.y, bounds_.w, bounds_.h, bg_);
    cv.drawText(bounds_.x, bounds_.y, text_, fg_, bg_, scale_, true);
}

// ---------------- ProgressBar ----------------
void ProgressBar::setValue(int32_t v) {
    v = clampi(v, lo_, hi_);
    if (v != v_) { v_ = v; invalidate(); }
}
void ProgressBar::setFillColor(Color c) { if (c != fill_) { fill_ = c; invalidate(); } }
void ProgressBar::draw(Canvas& cv) {
    const int16_t r = bounds_.h / 2;
    cv.fillRoundRect(bounds_.x, bounds_.y, bounds_.w, bounds_.h, r, track_);
    const int32_t fw = int32_t(bounds_.w) * (v_ - lo_) / (hi_ - lo_);
    if (fw > 0) cv.fillRoundRect(bounds_.x, bounds_.y, int16_t(fw < bounds_.h ? bounds_.h : fw), bounds_.h, r, fill_);
}

// ---------------- Gauge ----------------
Gauge::Gauge(int16_t cx, int16_t cy, int16_t radius, const char* caption, Color accent)
    : Widget({int16_t(cx - radius), int16_t(cy - radius), int16_t(2 * radius + 1), int16_t(2 * radius + 1)}),
      cx_(cx), cy_(cy), r_(radius), caption_(caption), accent_(accent) {}

void Gauge::setValue(int32_t v) {
    v = clampi(v, lo_, hi_);
    if (v != v_) { v_ = v; invalidate(); }
}
void Gauge::draw(Canvas& cv) {
    cv.fillRect(bounds_.x, bounds_.y, bounds_.w, bounds_.h, colors::Panel);
    const int16_t thick = r_ / 5 < 3 ? 3 : r_ / 5;
    const int16_t rin = r_ - thick;
    cv.fillArc(cx_, cy_, rin, r_, 225, 135, colors::DarkGrey);
    const int16_t sweep = int16_t(270L * (v_ - lo_) / (hi_ - lo_));
    if (sweep > 0) cv.fillArc(cx_, cy_, rin, r_, 225, 225 + sweep, accent_);

    char num[14];
    const uint8_t scale = r_ >= 50 ? 3 : 2;
    const int len = itoa10(v_, num);
    (void)len;
    cv.drawText(cx_ - Canvas::textWidth(num, scale) / 2, cy_ - Canvas::textHeight(scale) / 2, num,
                colors::White, colors::Panel, scale, true);
    if (caption_)
        cv.drawText(cx_ - Canvas::textWidth(caption_, 1) / 2, cy_ + r_ - thick - 6, caption_, colors::Grey,
                    colors::Panel, 1, true);
}

// ---------------- Graph ----------------
Graph::Graph(Rect r, int16_t* storage, uint16_t capacity, Color line)
    : Widget(r), buf_(storage), cap_(capacity ? capacity : 1), line_(line) {}

void Graph::push(int16_t v) {
    buf_[head_] = v;
    head_ = uint16_t((head_ + 1) % cap_);
    if (count_ < cap_) ++count_;
    invalidate();
}
void Graph::draw(Canvas& cv) {
    cv.fillRoundRect(bounds_.x, bounds_.y, bounds_.w, bounds_.h, 4, colors::DarkGrey);
    const Rect in = {int16_t(bounds_.x + 3), int16_t(bounds_.y + 3), int16_t(bounds_.w - 6), int16_t(bounds_.h - 6)};
    cv.fillRect(in.x, in.y, in.w, in.h, colors::Panel);
    for (int i = 1; i < 4; ++i) cv.hline(in.x, in.y + in.h * i / 4, in.w, colors::DarkGrey);
    if (count_ < 2) return;

    int16_t lo = lo_, hi = hi_;
    if (auto_) {
        lo = hi = buf_[(head_ + cap_ - count_) % cap_];
        for (uint16_t i = 0; i < count_; ++i) {
            const int16_t v = buf_[(head_ + cap_ - count_ + i) % cap_];
            if (v < lo) lo = v;
            if (v > hi) hi = v;
        }
        if (hi == lo) { --lo; ++hi; }
    }
    const int32_t span = int32_t(hi) - lo;
    int16_t px = 0, py = 0;
    for (uint16_t i = 0; i < count_; ++i) {
        const int16_t v = clampi(buf_[(head_ + cap_ - count_ + i) % cap_], lo, hi);
        const int16_t x = int16_t(in.x + in.w - 1 - int32_t(count_ - 1 - i) * (in.w - 1) / (cap_ > 1 ? cap_ - 1 : 1));
        const int16_t y = int16_t(in.y + in.h - 1 - int32_t(v - lo) * (in.h - 1) / span);
        if (i) cv.drawLine(px, py, x, y, line_);
        px = x; py = y;
    }
}

// ---------------- Button ----------------
void Button::setPressed(bool p) { if (p != pressed_) { pressed_ = p; invalidate(); } }
void Button::draw(Canvas& cv) {
    const Color face = pressed_ ? colors::DarkGrey : face_;
    cv.fillRoundRect(bounds_.x, bounds_.y, bounds_.w, bounds_.h, 6, face);
    const uint8_t scale = Canvas::textWidth(label_, 2) + 8 <= bounds_.w ? 2 : 1;
    cv.drawText(bounds_.x + (bounds_.w - Canvas::textWidth(label_, scale)) / 2,
                bounds_.y + (bounds_.h - Canvas::textHeight(scale)) / 2, label_, colors::White, face, scale, true);
}

// ---------------- Screen ----------------
bool Screen::add(Widget* w) {
    if (!w || n_ >= kMaxWidgets) return false;
    items_[n_++] = w;
    return true;
}
void Screen::redrawAll() {
    cv_.clear(bg_);
    for (uint8_t i = 0; i < n_; ++i) items_[i]->render(cv_);
}
uint32_t Screen::update() {
    uint32_t sent = 0;
    if (first_) { first_ = false; redrawAll(); sent += cv_.flush(panel_); }
    // One flush per widget: distant widgets never get merged into one huge dirty rect.
    for (uint8_t i = 0; i < n_; ++i) {
        if (!items_[i]->isInvalid()) continue;
        items_[i]->render(cv_);
        sent += cv_.flush(panel_);
    }
    return sent;
}

}  // namespace qs
