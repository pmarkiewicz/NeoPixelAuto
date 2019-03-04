#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"

#include <NeoPixelBrightnessBus.h>

#pragma GCC diagnostic pop

const uint16_t PixelCount = 8;

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount);

void display_init(uint8_t /*no_of_leds*/)
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