#pragma once
#include "PluginManager.h"
#include "WeatherService.h"

class SunrisePlugin : public Plugin {
public:
  void setup() override;
  void loop() override;
  const char *getName() const override { return "Sunrise"; }

private:
  int prevSunriseMin_ = -2; // -2: not drawn yet
  unsigned long lastDraw_ = 0;
  void render(int minutes);
  void drawSunriseIcon(int x, int y);
};

