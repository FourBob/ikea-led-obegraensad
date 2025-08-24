#pragma once

#include "PluginManager.h"
#include <array>

class TetrisDemoPlugin : public Plugin {
public:
  void setup() override;
  void loop() override;
  const char* getName() const override;
  void websocketHook(DynamicJsonDocument &event) override;

private:
  static constexpr int BOARD_W = 10;
  static constexpr int BOARD_H = 20; // rows 4..19 visible
  static constexpr int VIS_Y_OFFSET = 4; // top 4 rows hidden

  // Brightness levels
  static constexpr uint8_t BRIGHT_ACTIVE = 255;
  static constexpr uint8_t BRIGHT_FIXED = 120;
  static constexpr uint8_t BRIGHT_UI = 50;

  // Timing (ms)
  static constexpr uint16_t GRAVITY_MS = 220;
  static constexpr uint16_t LOCK_MS = 400;
  static constexpr uint16_t CLEAR_MS = 180;
  static constexpr uint16_t PAUSE_MS = 800;

  // Board data
  std::array<std::array<uint8_t, BOARD_W>, BOARD_H> board_{};

  struct Piece { uint8_t type; uint8_t rot; int x; int y; };
  Piece active_ {0,0,3,0};
  uint8_t nextType_ = 0;

  // AI target for current active piece
  int targetX_ = 3;
  uint8_t targetRot_ = 0;

  // Timing
  unsigned long lastGravity_ = 0;
  unsigned long lockUntil_ = 0;
  unsigned long clearUntil_ = 0;
  unsigned long pauseUntil_ = 0;
  bool clearing_ = false;
  std::array<bool, BOARD_H> clearRows_{};

  // Manual control dampener
  unsigned long manualUntil_ = 0; // skip AI homing while recent manual input

  // Helpers
  void resetBoard();
  void spawnPiece();
  bool collides(const Piece& p) const;
  void placeActive();
  void startClear();
  void applyClear();
  void stepGravity();
  void stepHoming(); // move/rotate towards target
  void render();
  bool isOverflow() const; // blocks in hidden rows
  void restartDemo();

  // AI helpers
  void chooseBestPlacement(uint8_t pieceType, uint8_t &bestRot, int &bestX) const;
  int simulateDropY(uint8_t type, uint8_t rot, int x) const;
  int evaluateBoardAfter(uint8_t type, uint8_t rot, int x) const;

  // Shapes: 7 pieces, 4 rotations, 4x4 mask (bit per cell)
  static const uint16_t SHAPES[7][4];
};

