// Host unit tests. Build: see test/Makefile-free instructions in README (g++ one-liner / CMake).
#include "sim_bus.h"
#include "../src/QuasarUI.h"
#include "../src/qs_font5x7.h"
#include <stdio.h>
#include <string.h>

static int g_fail = 0, g_run = 0;
#define CHECK(cond) do { ++g_run; if (!(cond)) { ++g_fail; printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)

using namespace qs;

static const int W = 240, H = 240;
static Color fbuf[W * H];

struct Rig {
    SimBus bus{W, H};
    Panel panel{bus, PanelConfig::ST7789_240x240()};
    Canvas cv{fbuf, W, H};
    Rig() { panel.begin(); cv.clear(0); cv.flush(panel); bus.bytesSent = 0; bus.cmdCount = 0; }
};

static void test_color_and_font() {
    CHECK(rgb(255, 255, 255) == 0xFFFF);
    CHECK(rgb(255, 0, 0) == 0xF800);
    CHECK(rgb(0, 255, 0) == 0x07E0);
    CHECK(rgb(0, 0, 255) == 0x001F);
    CHECK(kFont5x7[0][0] == 0 && kFont5x7['A' - 32][0] == 0x7C);
    CHECK(kFont5x7['~' - 32][1] == 0x01);  // table is exactly 95 glyphs, last is '~'
    CHECK(Canvas::textWidth("abc", 2) == 36);
}

static void test_clipping_is_safe() {
    Rig r;
    r.cv.fillRect(-50, -50, 100, 100, 0xFFFF);
    r.cv.fillRect(200, 200, 500, 500, 0xFFFF);
    r.cv.drawLine(-100, -100, 400, 400, 0x1234);
    r.cv.fillCircle(-5, 300, 40, 0x1234);
    r.cv.drawText(-30, -4, "clip", 1, 0, 3);
    r.cv.fillArc(0, 0, 10, 40, 0, 360, 0x1);
    r.cv.drawPixel(-1, 0, 1); r.cv.drawPixel(W, H, 1);
    Rect d = r.cv.dirty();
    CHECK(d.x >= 0 && d.y >= 0 && d.x + d.w <= W && d.y + d.h <= H);
    r.cv.flush(r.panel);
    CHECK(r.bus.writesOutsideWindow == 0);
}

static void test_flush_matches_framebuffer() {
    Rig r;
    r.cv.fillRect(10, 20, 30, 40, rgb(255, 0, 0));
    r.cv.drawText(100, 100, "Quasar", 0xFFFF, 0x0000, 2);
    r.cv.fillCircle(150, 60, 25, rgb(0, 255, 0));
    r.cv.flush(r.panel);
    CHECK(memcmp(r.bus.vram.data(), fbuf, sizeof fbuf) == 0);
    CHECK(r.bus.vram[25 * W + 15] == 0xF800);
}

static void test_dirty_rect_is_minimal() {
    Rig r;
    r.cv.fillRect(50, 60, 10, 5, 0xFFFF);
    Rect d = r.cv.dirty();
    CHECK(d.x == 50 && d.y == 60 && d.w == 10 && d.h == 5);
    r.bus.bytesSent = 0;
    uint32_t px = r.cv.flush(r.panel);
    CHECK(px == 50);
    CHECK(r.bus.bytesSent == 50 * 2 + 4 + 4);          // pixels + CASET + RASET args
    CHECK(r.cv.dirty().empty());
    CHECK(r.cv.flush(r.panel) == 0);                   // nothing dirty -> nothing sent
    // union of two far apart edits
    r.cv.drawPixel(5, 5, 1); r.cv.drawPixel(100, 90, 1);
    d = r.cv.dirty();
    CHECK(d.x == 5 && d.y == 5 && d.w == 96 && d.h == 86);
}

static void test_panel_offsets_and_rotation() {
    SimBus bus(240, 320);
    Panel p(bus, PanelConfig::ST7789_240x240());
    p.begin();
    CHECK(p.width() == 240 && p.height() == 240);
    p.setRotation(2);
    CHECK(bus.madctl == 0xC0);
    // rot 2 -> y offset 80: window must start at RAM row 80
    static Color small[240 * 240];
    Canvas cv(small, 240, 240);
    cv.drawPixel(0, 0, 0xFFFF);
    cv.flush(p);
    CHECK(bus.vram[80 * 240 + 0] == 0xFFFF);
    CHECK(bus.resets == 1);

    SimBus b2(320, 240);
    Panel ili(b2, PanelConfig::ILI9341_240x320());
    ili.begin();
    ili.setRotation(1);
    CHECK(ili.width() == 320 && ili.height() == 240);
    CHECK(b2.madctl == 0x28);
}

static void test_widgets_redraw_only_on_change() {
    Rig r;
    Label lbl({10, 10, 100, 16}, 2);
    ProgressBar bar({10, 40, 120, 12});
    Gauge g(170, 60, 50, "RPM");
    static int16_t gbuf[60];
    Graph gr({10, 120, 200, 80}, gbuf, 60);
    Button btn({10, 205, 100, 30}, "OK");
    Screen s(r.cv, r.panel);
    CHECK(s.add(&lbl) && s.add(&bar) && s.add(&g) && s.add(&gr) && s.add(&btn));

    uint32_t first = s.update();
    CHECK(first == uint32_t(W) * H);                    // first frame paints everything
    CHECK(s.update() == 0);                             // idle frame costs zero bytes

    lbl.setText("hello");
    uint32_t a = s.update();
    CHECK(a > 0 && a <= 100u * 16u);                    // only the label rectangle
    lbl.setText("hello");                               // same text -> no work
    CHECK(s.update() == 0);

    bar.setValue(50);
    CHECK(s.update() <= 120u * 12u);
    bar.setValue(50);
    CHECK(s.update() == 0);

    for (int i = 0; i < 100; ++i) gr.push(int16_t(i * 7 % 50));  // wraps the ring buffer
    CHECK(gr.count() == 60);
    CHECK(s.update() <= 200u * 80u);

    g.setValue(75);
    CHECK(s.update() <= 101u * 101u);
    btn.setPressed(true);
    CHECK(s.update() <= 100u * 30u);
    CHECK(memcmp(r.bus.vram.data(), fbuf, sizeof fbuf) == 0);   // glass == framebuffer after all updates
}

static void test_screen_capacity() {
    Rig r;
    Screen s(r.cv, r.panel);
    Label l({0, 0, 10, 10});
    int ok = 0;
    for (int i = 0; i < Screen::kMaxWidgets + 5; ++i) ok += s.add(&l);
    CHECK(ok == Screen::kMaxWidgets);
    CHECK(!s.add(nullptr));
}

static void test_label_truncates() {
    Label l({0, 0, 10, 10});
    char longText[100];
    memset(longText, 'x', 99); longText[99] = 0;
    l.setText(longText);
    CHECK(strlen(l.text()) == Label::kMaxChars);
}

int main() {
    test_color_and_font();
    test_clipping_is_safe();
    test_flush_matches_framebuffer();
    test_dirty_rect_is_minimal();
    test_panel_offsets_and_rotation();
    test_widgets_redraw_only_on_change();
    test_screen_capacity();
    test_label_truncates();
    printf("%d checks, %d failed\n", g_run, g_fail);
    return g_fail ? 1 : 0;
}
