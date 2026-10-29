// Renders the demo dashboard through the real library into a simulated ILI9341
// panel and writes PPM frames + a stats table. Used for README images.
// Usage: render_gallery <outdir>
#include "sim_bus.h"
#include "../src/QuasarUI.h"
#include <math.h>
#include <stdio.h>
#include <string>

using namespace qs;

static const int W = 320, H = 240;
static Color fbuf[W * H];

int main(int argc, char** argv) {
    const std::string out = argc > 1 ? argv[1] : ".";
    SimBus bus(W, H);
    Panel panel(bus, PanelConfig::ILI9341_240x320());
    panel.begin();
    panel.setRotation(1);  // 320x240 landscape
    Canvas cv(fbuf, W, H);

    Label title({10, 8, 220, 16}, 2, colors::White);
    Label sub({10, 28, 200, 8}, 1, colors::Grey);
    Gauge temp(62, 100, 50, "TEMP C", colors::Orange);
    Gauge hum(170, 100, 50, "HUM %", colors::Cyan);
    Label batL({236, 56, 80, 8}, 1, colors::Grey);
    ProgressBar bat({236, 68, 76, 12}, colors::Green);
    Label upL({236, 96, 80, 8}, 1, colors::Grey);
    Label up({236, 108, 80, 16}, 2, colors::White);
    Button btn({236, 140, 76, 26}, "RESET");
    static int16_t samples[96];
    Graph graph({10, 168, 216, 64}, samples, 96, colors::Green);
    Label gl({236, 180, 80, 8}, 1, colors::Grey);
    Label gv({236, 192, 80, 16}, 2, colors::Green);

    Screen scr(cv, panel);
    Widget* all[] = {&title, &sub, &temp, &hum, &batL, &bat, &upL, &up, &btn, &graph, &gl, &gv};
    for (Widget* w : all) scr.add(w);

    title.setText("QUASAR HUB");
    sub.setText("ESP32 - SPI 40 MHz - QuasarUI");
    batL.setText("BATTERY");
    upL.setText("UPTIME s");
    gl.setText("TEMP x10");
    temp.setRange(0, 60);
    hum.setRange(0, 100);
    bat.setRange(0, 100);
    graph.setRange(50, 450);

    FILE* st = fopen((out + "/stats.csv").c_str(), "w");
    fprintf(st, "frame,pixels_sent,bytes_on_bus\n");
    const uint32_t fullFrame = uint32_t(W) * H;

    uint64_t totalPx = 0;
    int frames = 0;
    for (int f = 0; f < 96; ++f) {
        const float t = f / 95.0f;
        const int tempC = int(26 + 14 * sinf(t * 6.2832f * 1.5f) + 6 * t);
        temp.setValue(tempC);
        hum.setValue(int(35 + 40 * t));
        bat.setValue(int(100 - 55 * t));
        bat.setFillColor(t > 0.6f ? colors::Orange : colors::Green);
        char buf[16];
        snprintf(buf, sizeof buf, "%d", 1000 + f * 5);
        up.setText(buf);
        const int v10 = int(tempC * 10);
        graph.push(int16_t(v10));
        snprintf(buf, sizeof buf, "%d.%d", v10 / 10, v10 % 10);
        gv.setText(buf);
        btn.setPressed(f >= 82 && f < 88);

        const size_t before = bus.bytesSent;
        const uint32_t px = scr.update();
        fprintf(st, "%d,%u,%lu\n", f, px, (unsigned long)(bus.bytesSent - before));
        if (f > 0) { totalPx += px; ++frames; }

        char path[256];
        snprintf(path, sizeof path, "%s/frame_%02d.ppm", out.c_str(), f);
        bus.savePPM(path, 2);
    }
    fclose(st);
    printf("full frame: %u px (%u bytes @16bpp)\n", fullFrame, fullFrame * 2);
    printf("avg dirty frame after first: %.0f px  => %.1f%% of a full redraw\n", double(totalPx) / frames,
           100.0 * double(totalPx) / frames / fullFrame);
    printf("SPI time @40MHz: full %.2f ms, avg dirty %.2f ms\n", fullFrame * 16 / 40e3,
           double(totalPx) / frames * 16 / 40e3);
    return 0;
}
