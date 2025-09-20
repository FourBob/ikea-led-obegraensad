#pragma once

#include "PluginManager.h"

class AnimationPlugin : public Plugin
{
private:
  uint8_t step = 0;
  std::vector<std::vector<int>> customAnimationFrames;

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
  void websocketHook(DynamicJsonDocument &request) override;
  // Expose current frames for minimal fetch via WebSocket
  const std::vector<std::vector<int>> &getFrames() const;

};
