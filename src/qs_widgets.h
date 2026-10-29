// QuasarUI - retained-mode widgets. A widget redraws only when its state
// changed; Screen::update() draws invalid widgets and flushes the dirty rect.
#pragma once
#include "qs_canvas.h"

namespace qs {

class Widget {
public:
    explicit Widget(Rect bounds) : bounds_(bounds) {}
    virtual ~Widget() {}
    virtual void draw(Canvas& cv) = 0;
    void invalidate() { invalid_ = true; }
    bool isInvalid() const { return invalid_; }
    const Rect& bounds() const { return bounds_; }
    bool contains(int16_t x, int16_t y) const {
        return x >= bounds_.x && y >= bounds_.y && x < bounds_.x + bounds_.w && y < bounds_.y + bounds_.h;
    }
    void render(Canvas& cv) { draw(cv); invalid_ = false; }

protected:
    Rect bounds_;
    bool invalid_ = true;
};

class Label : public Widget {
public:
    static constexpr uint8_t kMaxChars = 31;
    Label(Rect r, uint8_t scale = 1, Color fg = colors::White, Color bg = colors::Panel)
        : Widget(r), scale_(scale), fg_(fg), bg_(bg) { text_[0] = 0; }
    void setText(const char* s);               // invalidates only if the text changed
    void setColor(Color fg);
    const char* text() const { return text_; }
    void draw(Canvas& cv) override;

private:
    char text_[kMaxChars + 1];
    uint8_t scale_;
    Color fg_, bg_;
};

class ProgressBar : public Widget {
public:
    ProgressBar(Rect r, Color fill = colors::Accent, Color track = colors::DarkGrey)
        : Widget(r), fill_(fill), track_(track) {}
    void setRange(int32_t lo, int32_t hi) { lo_ = lo; hi_ = hi > lo ? hi : lo + 1; invalidate(); }
    void setValue(int32_t v);
    void setFillColor(Color c);
    void draw(Canvas& cv) override;

private:
    int32_t lo_ = 0, hi_ = 100, v_ = 0;
    Color fill_, track_;
};

// 270-degree dial with centre readout.
class Gauge : public Widget {
public:
    Gauge(int16_t cx, int16_t cy, int16_t radius, const char* caption, Color accent = colors::Cyan);
    void setRange(int32_t lo, int32_t hi) { lo_ = lo; hi_ = hi > lo ? hi : lo + 1; invalidate(); }
    void setValue(int32_t v);
    void draw(Canvas& cv) override;

private:
    int32_t lo_ = 0, hi_ = 100, v_ = 0;
    int16_t cx_, cy_, r_;
    const char* caption_;
    Color accent_;
};

// Scrolling line chart over a caller-owned ring buffer (no heap).
class Graph : public Widget {
public:
    Graph(Rect r, int16_t* storage, uint16_t capacity, Color line = colors::Green);
    void setRange(int16_t lo, int16_t hi) { auto_ = false; lo_ = lo; hi_ = hi > lo ? hi : lo + 1; invalidate(); }
    void setAutoRange() { auto_ = true; invalidate(); }
    void push(int16_t v);
    uint16_t count() const { return count_; }
    void draw(Canvas& cv) override;

private:
    int16_t* buf_;
    uint16_t cap_, head_ = 0, count_ = 0;
    int16_t lo_ = 0, hi_ = 100;
    bool auto_ = true;
    Color line_;
};

class Button : public Widget {
public:
    Button(Rect r, const char* label, Color face = colors::Accent)
        : Widget(r), label_(label), face_(face) {}
    void setPressed(bool p);
    bool pressed() const { return pressed_; }
    void draw(Canvas& cv) override;

private:
    const char* label_;
    Color face_;
    bool pressed_ = false;
};

class Screen {
public:
    static constexpr uint8_t kMaxWidgets = 24;
    Screen(Canvas& cv, Panel& panel, Color background = colors::Panel)
        : cv_(cv), panel_(panel), bg_(background) {}
    bool add(Widget* w);                 // false when full
    void redrawAll();                    // repaint background + every widget
    uint32_t update();                   // draw invalid widgets, flush; returns pixels sent
private:
    Canvas& cv_;
    Panel& panel_;
    Color bg_;
    Widget* items_[kMaxWidgets] = {};
    uint8_t n_ = 0;
    bool first_ = true;
};

}  // namespace qs
