#pragma once
#include "PluginManager.h"
#include "WeatherService.h"

class SunsetPlugin : public Plugin {
public:
  void setup() override;
  void loop() override;
  const char *getName() const override { return "Sunset"; }

private:
  int prevSunsetMin_ = -2; // -2: not drawn yet
  unsigned long lastDraw_ = 0;
  void render(int minutes);
  void drawSunsetIcon(int x, int y);
};

