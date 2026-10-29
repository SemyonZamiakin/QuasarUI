// QuasarUI - Dashboard example (ESP32 + ILI9341 320x240, landscape)
// Wiring (ESP32 default VSPI): SCK=18  MOSI=23  CS=5  DC=2  RST=4  LED->3V3
#include <QuasarUI.h>

using namespace qs;

static const uint16_t W = 320, H = 240;

ArduinoSpiBus bus(SPI, /*cs*/ 5, /*dc*/ 2, /*rst*/ 4, 40000000UL);
Panel panel(bus, PanelConfig::ILI9341_240x320());

Label title({10, 8, 220, 16}, 2);
Gauge temp(62, 100, 50, "TEMP C", colors::Orange);
ProgressBar bar({10, 168, 216, 12});
int16_t samples[96];
Graph graph({10, 190, 216, 44}, samples, 96);

Canvas* canvas = nullptr;  // 150 KB framebuffer is heap-allocated (too big for static DRAM on ESP32)
Screen* screen = nullptr;

void halt(const char* msg) {
    Serial.println(msg);
    while (true) delay(1000);
}

void setup() {
    Serial.begin(115200);
    Color* fb = (Color*)malloc(Canvas::bytesFor(W, H));
    if (!fb) halt("framebuffer alloc failed");
    canvas = new Canvas(fb, W, H);

    if (!panel.begin()) halt("panel init failed");
    panel.setRotation(1);

    screen = new Screen(*canvas, panel);
    screen->add(&title);
    screen->add(&temp);
    screen->add(&bar);
    screen->add(&graph);
    title.setText("QUASAR HUB");
    temp.setRange(0, 60);
    graph.setAutoRange();
}

void loop() {
    static uint32_t last = 0;
    if (millis() - last < 200) return;
    last = millis();

    const int t = 20 + (millis() / 500) % 30;  // replace with a real sensor read
    temp.setValue(t);
    bar.setValue(map(t, 20, 50, 0, 100));
    graph.push(int16_t(t * 10));

    screen->update();  // redraws only what changed, sends only the dirty rectangles
}
