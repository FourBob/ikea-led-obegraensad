#include "plugins/ArcadeSpritesPlugin.h"
#include "screen.h"
#include <math.h>
#ifdef ESP32
#include <esp_system.h>
#endif

// Each frame stores up to 8 rows, each row uses LSB-at-left bit order.
static std::array<uint8_t,8> makeRowMask(const uint8_t rows8[8]){
  std::array<uint8_t,8> r{};
  for(int y=0;y<8;y++){ r[y]= rows8[y]; }
  return r;
}

// unified RNG helper
static inline uint32_t urand(){
#ifdef ESP32
  return esp_random();
#else
  static bool seeded=false; if(!seeded){ srand((unsigned)millis()); seeded=true; }
  return ((uint32_t)rand()<<16) ^ (uint32_t)rand();
#endif
}

void ArcadeSpritesPlugin::initSprites(){
  sprites_.clear();
  // 5 Space-Invaders-inspirierte Sprites (6x6 bis 8x8), je 2 Frames
  // 1) Invader Small 6x6
  SpriteDef invSmall{6,6, {}};
  invSmall.frames.push_back(makeRowMask((const uint8_t[8]){0b001100,0b011110,0b111111,0b101101,0b001100,0b010010,0,0}));
  invSmall.frames.push_back(makeRowMask((const uint8_t[8]){0b001100,0b011110,0b111111,0b101101,0b010010,0b100001,0,0}));
  sprites_.push_back(invSmall);
  // 2) Invader Squid 6x6
  SpriteDef invSquid{6,6, {}};
  invSquid.frames.push_back(makeRowMask((const uint8_t[8]){0b001100,0b011110,0b111111,0b010010,0b010010,0b100001,0,0}));
  invSquid.frames.push_back(makeRowMask((const uint8_t[8]){0b001100,0b011110,0b111111,0b010010,0b100001,0b010010,0,0}));
  sprites_.push_back(invSquid);
  // 3) Invader Crab (compact) 6x6
  SpriteDef invCrab6{6,6, {}};
  invCrab6.frames.push_back(makeRowMask((const uint8_t[8]){0b000000,0b011110,0b110011,0b111111,0b011110,0b010010,0,0}));
  invCrab6.frames.push_back(makeRowMask((const uint8_t[8]){0b000000,0b011110,0b110011,0b111111,0b010010,0b011110,0,0}));
  sprites_.push_back(invCrab6);
  // 4) Invader Crab 8x8 (breit)
  SpriteDef invCrab8{8,8, {}};
  invCrab8.frames.push_back(makeRowMask((const uint8_t[8]){0b00111100,0b01111110,0b11011011,0b11111111,0b01111110,0b00100100,0b01000010,0b10000001}));
  invCrab8.frames.push_back(makeRowMask((const uint8_t[8]){0b00111100,0b01111110,0b11011011,0b11111111,0b01111110,0b01000010,0b00100100,0b00000000}));
  sprites_.push_back(invCrab8);
  // 5) UFO Saucer 7x5
  SpriteDef ufo{7,5,{}};
  ufo.frames.push_back(makeRowMask((const uint8_t[8]){0b0011100,0b0111110,0b1111111,0b0111110,0b0011100,0,0,0}));
  ufo.frames.push_back(makeRowMask((const uint8_t[8]){0b0011100,0b0110110,0b1111111,0b0110110,0b0011100,0,0,0}));
  sprites_.push_back(ufo);
}

void ArcadeSpritesPlugin::respawn(Entity& e){
  e.frame=0; e.nextFrameAt=millis()+220;
  e.bright = 240; // klar erkennbar
  e.pattern = 0; // nur Fly-by
  e.def = &sprites_[urand()%sprites_.size()];
  int maxY = 16 - e.def->h; if (maxY < 0) maxY = 0;
  e.y = (float)(urand() % (maxY + 1));
  e.x = 16 + (urand()%8);
  e.vx = -(0.05f + (urand()%50)/1000.0f);
  e.vy = 0.0f;
  e.phase = 0.0f; e.amp = 0.0f;
}

void ArcadeSpritesPlugin::spawnEntities(){
  entities_.clear();
  entities_.resize(2); // genau 2 Entities
  // Bänder oben/unten passend für 8x8/7x5, vollständig sichtbar
  int bandY[2] = {1, 9};
#ifdef ESP32
  // randomize which band starts on top/bottom at boot
  if (esp_random() & 1) { int tmp = bandY[0]; bandY[0] = bandY[1]; bandY[1] = tmp; }
#endif
  for(size_t i=0;i<entities_.size();++i){
    respawn(entities_[i]);
    const SpriteDef* d = entities_[i].def;
    int y = bandY[i % 2];
    if (y > 16 - d->h) y = 16 - d->h;
    entities_[i].y = y;
    // versetze X, damit sie nicht überlappen beim Start
    entities_[i].x += i * 6; // mehr Abstand am Start
  }
}

void ArcadeSpritesPlugin::updateEntity(Entity& e, unsigned long now){
  // Framewechsel für 2-Frame-Invader
  if (now >= e.nextFrameAt) { e.frame = (e.frame+1) % e.def->frames.size(); e.nextFrameAt = now + 220; }

  // Nur Fly-by: konstantes vx
  e.x += e.vx;

  // Offscreen links? -> Respawn rechts
  if (e.x < -e.def->w) {
    respawn(e);
  }
}

void ArcadeSpritesPlugin::render(){
  Screen.beginUpdate();
  Screen.clear();

  for(const auto &e: entities_){
    const auto &frame = e.def->frames[e.frame];
    for(int dy=0; dy<e.def->h; ++dy){
      uint8_t rowMask = frame[dy];
      for(int dx=0; dx<e.def->w; ++dx){
        if (rowMask & (1 << dx)){
          int px = (int)e.x + dx;
          int py = (int)e.y + dy;
          if (px>=0 && px<16 && py>=0 && py<16) Screen.setPixel(px, py, 1, e.bright);
        }
      }
    }
  }

  Screen.endUpdate();
}

void ArcadeSpritesPlugin::setup(){
  initSprites();
  spawnEntities();
  lastTick_ = millis();
}

void ArcadeSpritesPlugin::loop(){
  unsigned long now = millis();
  // simple fixed-step update
  for(auto &e: entities_) updateEntity(e, now);
  render();
}

const char* ArcadeSpritesPlugin::getName() const { return "Arcade Sprites"; }

