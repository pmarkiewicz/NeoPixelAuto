#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"

#include <NeoPixelBrightnessBus.h>

#pragma GCC diagnostic pop

const uint16_t PixelCount = 8;
const uint8_t PixelPin = 14; // ignored
const RgbColor color(255, 0, 0);

NeoPixelBrightnessBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);

void display_init()
{
  strip.Begin();

  strip.Show();
}

void display_set_color(uint8_t r, uint8_t g, uint8_t b) {
  const RgbColor color(r, g, b);

  for (uint16_t px = 0; px < PixelCount; px++) {
    strip.SetPixelColor(px, color);
  }

  strip.Show();
}