#pragma once

#include "PluginManager.h"
#include <vector>
#include <array>

class ArcadeSpritesPlugin : public Plugin {
public:
  void setup() override;
  void loop() override;
  const char* getName() const override;

private:
  struct SpriteDef { uint8_t w, h; std::vector<std::array<uint8_t, 8>> frames; };
  struct Entity {
    const SpriteDef* def;
    uint8_t frame=0; unsigned long nextFrameAt=0;
    float x=0, y=0; float vx=0, vy=0;
    uint8_t pattern=0; // 0 flyby,1 sine,2 zigzag,3 formation,4 dive
    float phase=0, amp=1.0f;
    uint8_t bright=220;
    int groupId=-1; // for formation
  };

  std::vector<SpriteDef> sprites_;
  std::vector<Entity> entities_;
  unsigned long lastTick_=0;

  void initSprites();
  void spawnEntities();
  void updateEntity(Entity& e, unsigned long now);
  void render();
  void respawn(Entity& e);
};

