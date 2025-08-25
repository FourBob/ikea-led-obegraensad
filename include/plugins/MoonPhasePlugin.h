#pragma once

#include "PluginManager.h"
#include "WeatherService.h"

class MoonPhasePlugin : public Plugin {
public:
  void setup() override;
  void loop() override;
  const char* getName() const override;
private:
  void renderLoading();
  void renderPhase();
  // Map moonPhase string or illumination to phase index 0..7
  uint8_t classifyPhase(const WeatherData &d) const;
};

