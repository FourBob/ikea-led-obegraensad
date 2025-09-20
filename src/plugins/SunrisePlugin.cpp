#include "plugins/SunrisePlugin.h"
#include "screen.h"
#include "secrets.h"

void SunrisePlugin::setup() {
  // Simple loading hint
  Screen.setPixel(4, 7, 1);
  Screen.setPixel(5, 7, 1);
  Screen.setPixel(7, 7, 1);
  Screen.setPixel(8, 7, 1);
  Screen.setPixel(10, 7, 1);
  Screen.setPixel(11, 7, 1);
}

void SunrisePlugin::loop() {
  const WeatherData &d = WeatherService::getInstance().get();
  int sr = d.sunriseMinutes; // -1 if unknown
  unsigned long now = millis();
  if (sr != prevSunriseMin_ || now - lastDraw_ > 1000) {
    render(sr);
    prevSunriseMin_ = sr;
    lastDraw_ = now;
  }
}

void SunrisePlugin::render(int minutes) {
  Screen.beginUpdate();
  Screen.clear();

  // Icon über gesamte Breite (16×16)
  drawSunriseIcon(0, 0);

  if (minutes >= 0) {
    int hh = minutes / 60;
    int mm = minutes % 60;

#if TIME_FORMAT_24H
    // keep 24h
#else
    int h12 = hh % 12; if (h12 == 0) h12 = 12; hh = h12;
#endif

    // Zeit unten im PongClock-Stil (kleine Ziffern, ohne Doppelpunkt)
    int yBase  = 10;
    int hTens = (hh / 10);
    int hOnes = (hh % 10);
    int mTens = (mm / 10);
    int mOnes = (mm % 10);

    // Startpositionen: -1, 3, 8, 12 (entspricht PongClock 0,4,9,13 mit -1 Shift)
    Screen.drawCharacter(-1, yBase, Screen.readBytes(smallNumbers[hTens]), 4, 255);
    Screen.drawCharacter( 3, yBase, Screen.readBytes(smallNumbers[hOnes]), 4, 255);
    Screen.drawCharacter( 8, yBase, Screen.readBytes(smallNumbers[mTens]), 4, 255);
    Screen.drawCharacter(12, yBase, Screen.readBytes(smallNumbers[mOnes]), 4, 255);
  } else {
    // Kein Wert: dezenter Platzhalter unten mittig
    Screen.setPixel(7, 12, 1);
    Screen.setPixel(8, 12, 1);
    Screen.setPixel(9, 12, 1);
  }

  Screen.endUpdate();
}

void SunrisePlugin::drawSunriseIcon(int x, int y) {
  // Full-width layout (16×16): Sun on the left, 2px gap, Up-arrow on the right
  // Sun: center at (x+4, y+3); 3×3 core + 8 rays; rightmost sun pixel at x+6
  int cx = x + 4, cy = y + 3;
  for (int dx=-1; dx<=1; ++dx) {
    for (int dy=-1; dy<=1; ++dy) {
      Screen.setPixel(cx + dx, cy + dy, 1, (dx==0 && dy==0) ? 190 : 150);
    }
  }
  // 8 rays at distance 2
  Screen.setPixel(cx,     cy-2, 1, 110); // N
  Screen.setPixel(cx+2,   cy-2, 1, 100); // NE
  Screen.setPixel(cx+2,   cy,   1, 110); // E
  Screen.setPixel(cx+2,   cy+2, 1, 100); // SE
  Screen.setPixel(cx,     cy+2, 1, 110); // S
  Screen.setPixel(cx-2,   cy+2, 1, 100); // SW
  Screen.setPixel(cx-2,   cy,   1, 110); // W
  Screen.setPixel(cx-2,   cy-2, 1, 100); // NW

  // 2-pixel gap at columns x+7 and x+8 (leave empty)

  // Arrow (right): center column ac = x+12
  int ac = x + 12;
  // tip
  Screen.setPixel(ac, y + 0, 1, 210);
  // head row 2 (width 3)
  Screen.setPixel(ac-1, y + 1, 1, 190);
  Screen.setPixel(ac,   y + 1, 1, 200);
  Screen.setPixel(ac+1, y + 1, 1, 190);
  // head row 3 (width 5)
  Screen.setPixel(ac-2, y + 2, 1, 180);
  Screen.setPixel(ac-1, y + 2, 1, 190);
  Screen.setPixel(ac,   y + 2, 1, 200);
  Screen.setPixel(ac+1, y + 2, 1, 190);
  Screen.setPixel(ac+2, y + 2, 1, 180);
  // shaft
  Screen.setPixel(ac, y + 3, 1, 170);
  Screen.setPixel(ac, y + 4, 1, 170);
  Screen.setPixel(ac, y + 5, 1, 170);
  // base (subtle)
  Screen.setPixel(ac-1, y + 6, 1, 120);
  Screen.setPixel(ac,   y + 6, 1, 120);
  Screen.setPixel(ac+1, y + 6, 1, 120);
}


