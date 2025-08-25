#include "plugins/MoonPhasePlugin.h"
#include "screen.h"
#include <string.h>

void MoonPhasePlugin::setup() {
  // nothing to initialize; WeatherService is global and updated in main loop
}

void MoonPhasePlugin::loop() {
  const WeatherData &d = WeatherService::getInstance().get();
  if (!d.valid || d.moonIllum < 0) {
    renderLoading();
  } else {
    renderPhase();
  }
}

void MoonPhasePlugin::renderLoading() {
  Screen.beginUpdate();
  Screen.clear();
  // simple loading dot
  Screen.setPixel(8,8,1,80);
  Screen.endUpdate();
}

// Helper to classify into 8 textual phases if needed elsewhere
uint8_t MoonPhasePlugin::classifyPhase(const WeatherData &d) const {
  if (d.moonPhase[0]) {
    if (strstr(d.moonPhase, "New")) return 0;
    if (strstr(d.moonPhase, "Waxing Crescent")) return 1;
    if (strstr(d.moonPhase, "First Quarter")) return 2;
    if (strstr(d.moonPhase, "Waxing Gibbous")) return 3;
    if (strstr(d.moonPhase, "Full")) return 4;
    if (strstr(d.moonPhase, "Waning Gibbous")) return 5;
    if (strstr(d.moonPhase, "Last Quarter")) return 6;
    if (strstr(d.moonPhase, "Waning Crescent")) return 7;
  }
  int ill = d.moonIllum; // 0..100
  if (ill <= 2) return 0;
  if (ill < 25) return 1;
  if (ill < 50) return 2;
  if (ill < 75) return 3;
  if (ill >= 98) return 4;
  if (ill > 75) return 5;
  if (ill > 50) return 6;
  return 7;
}

void MoonPhasePlugin::renderPhase() {
  const WeatherData &d = WeatherService::getInstance().get();

  // As large as possible visually round on a non-square panel:
  // Use full width (R=7) and compensate Y by aspect ratio (height/width)
  const int cx = 8, cy = 8, R = 7;
  const float ASPECT_YX = 43.5f / 29.5f; // pixel height / pixel width
  const uint8_t BRIGHT_LIT = 220;
  const uint8_t BRIGHT_SHADOW = 50; // indicate non-visible part

  // Illumination 0..1
  float f = d.moonIllum <= 0 ? 0.0f : (d.moonIllum >= 100 ? 1.0f : d.moonIllum / 100.0f);

  // Determine waxing (right lit) vs waning (left lit) from phase string
  bool waxing = true; // default
  if (d.moonPhase[0]) {
    if (strstr(d.moonPhase, "Waning") || strstr(d.moonPhase, "Last Quarter")) waxing = false;
    if (strstr(d.moonPhase, "Waxing") || strstr(d.moonPhase, "First Quarter")) waxing = true;
  }

  // Two-circle approximation with aspect correction (ellipse tests)
  float dshift = fabsf(2.0f * f - 1.0f) * R; // 0..R
  int shift = (int)(dshift + 0.5f);

  Screen.beginUpdate();
  Screen.clear();

  for (int y = 0; y < 16; ++y) {
    for (int x = 0; x < 16; ++x) {
      int dx = x - cx;
      int dy = y - cy;
      float dyA = dy * ASPECT_YX; // stretch y for distance test
      bool inBase = ((dx*dx) + (dyA*dyA)) <= (R*R);
      if (!inBase) continue;

      // Draw shadow (dark side) first to indicate full disc
      Screen.setPixel(x, y, 1, BRIGHT_SHADOW);

      if (f <= 0.0f) continue;       // new moon: leave as shadow
      if (f >= 1.0f) {               // full moon: brighten all inside
        Screen.setPixel(x, y, 1, BRIGHT_LIT);
        continue;
      }

      // Decide lit vs shadow using shifted circle test (also aspect-corrected)
      int sx_gibbous = waxing ? (cx + shift) : (cx - shift);
      int sx_crescent = waxing ? (cx - shift) : (cx + shift);

      float dyG = dyA; // same vertical scaling
      float dd_g = (x - sx_gibbous); dd_g = dd_g*dd_g + dyG*dyG;
      float dd_c = (x - sx_crescent); dd_c = dd_c*dd_c + dyG*dyG;

      bool lit = false;
      if (f >= 0.5f) {
        // gibbous: intersection with shifted circle
        lit = (dd_g <= R*R);
      } else {
        // crescent: base minus shifted opposite circle
        lit = !(dd_c <= R*R);
      }

      if (lit) Screen.setPixel(x, y, 1, BRIGHT_LIT);
    }
  }

  Screen.endUpdate();
}

const char* MoonPhasePlugin::getName() const { return "Moon Phase"; }

