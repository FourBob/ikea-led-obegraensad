#include "plugins/TetrisDemoPlugin.h"
#include "screen.h"
#ifdef ESP32
#include <esp_system.h> // esp_random()
#endif

// 4x4 masks (bit 0..15) row-major, bit set = block
// Order: I,O,T,S,Z,J,L ; rotations 0,90,180,270
const uint16_t TetrisDemoPlugin::SHAPES[7][4] = {
  // I
  {0x0F00, 0x2222, 0x00F0, 0x4444},
  // O
  {0x6600, 0x6600, 0x6600, 0x6600},
  // T
  {0x0E40, 0x4C40, 0x4E00, 0x4640},
  // S
  {0x06C0, 0x4620, 0x0360, 0x2640},
  // Z
  {0x0C60, 0x4260, 0x06C0, 0x2640},
  // J
  {0x8E00, 0x6440, 0x0E20, 0x44C0},
  // L (fix rotation 0: vertical on right above 3-wide base)
  {0x1E00, 0x4460, 0x0E80, 0xC440}
};

void TetrisDemoPlugin::resetBoard() {
  for (auto &row : board_) row.fill(0);
}

void TetrisDemoPlugin::spawnPiece() {
  // choose placement using AI for nextType_
  if (nextType_ > 6) nextType_ = 0;
  uint8_t type = nextType_;
  chooseBestPlacement(type, targetRot_, targetX_);
  // spawn new active piece
  active_.type = type;
  active_.rot = 0; // will rotate towards targetRot_
  active_.x = BOARD_W/2 - 2; // start center-ish
  active_.y = 0;
  // pick a new next type (deterministic cycle)
  nextType_ = (nextType_ + 1) % 7;
  lastGravity_ = millis();
  lockUntil_ = 0;
  clearing_ = false;
}

bool TetrisDemoPlugin::collides(const Piece &p) const {
  uint16_t mask = SHAPES[p.type][p.rot];
  for (int dy=0; dy<4; ++dy) {
    for (int dx=0; dx<4; ++dx) {
      if (mask & (1 << ((3-dy)*4 + (3-dx)))) {
        int bx = p.x + dx;
        int by = p.y + dy;
        if (bx < 0 || bx >= BOARD_W || by < 0 || by >= BOARD_H) return true;
        if (board_[by][bx]) return true;
      }
    }
  }
  return false;
}

void TetrisDemoPlugin::placeActive() {
  uint16_t mask = SHAPES[active_.type][active_.rot];
  for (int dy=0; dy<4; ++dy) {
    for (int dx=0; dx<4; ++dx) {
      if (mask & (1 << ((3-dy)*4 + (3-dx)))) {
        int bx = active_.x + dx;
        int by = active_.y + dy;
        if (bx>=0 && bx<BOARD_W && by>=0 && by<BOARD_H) {
          board_[by][bx] = 1;
        }
      }
    }
  }
}

void TetrisDemoPlugin::startClear() {
  clearRows_.fill(false);
  bool any=false;
  for (int y=0; y<BOARD_H; ++y) {
    bool full=true;
    for (int x=0; x<BOARD_W; ++x) full &= (board_[y][x] != 0);
    clearRows_[y] = full;
    any |= full;
  }
  if (any) {
    clearing_ = true;
    clearUntil_ = millis() + CLEAR_MS;
  } else {
    // next piece
    spawnPiece();
  }
}

void TetrisDemoPlugin::applyClear() {
  int dst = BOARD_H - 1;
  for (int y = BOARD_H - 1; y >= 0; --y) {
    if (!clearRows_[y]) {
      if (dst != y) board_[dst] = board_[y];
      dst--;
    }
  }
  // zero out remaining rows at the top [0..dst]
  for (int y = dst; y >= 0; --y) board_[y].fill(0);
  clearing_ = false;
  pauseUntil_ = millis() + PAUSE_MS;
}

void TetrisDemoPlugin::stepHoming() {
  // 1) Try to reach target rotation with simple wall-kicks
  if (active_.rot != targetRot_) {
    Piece p = active_;
    p.rot = (p.rot + 1) % 4;
    if (!collides(p)) { active_ = p; return; }
    // wall-kick left
    p = active_; p.rot = (p.rot + 1) % 4; p.x -= 1;
    if (!collides(p)) { active_ = p; return; }
    // wall-kick right
    p = active_; p.rot = (p.rot + 1) % 4; p.x += 1;
    if (!collides(p)) { active_ = p; return; }
    // if rotation blocked, try horizontal step first to make room
  }
  // 2) Move horizontally toward target (one step per tick)
  if (active_.x != targetX_) {
    Piece p = active_;
    int dir = (active_.x < targetX_) ? 1 : -1;
    p.x += dir;
    if (!collides(p)) { active_ = p; return; }
    // try move with a rotate that may reduce width
    Piece r = active_;
    r.rot = (r.rot + 1) % 4; r.x += dir;
    if (!collides(r)) { active_ = r; return; }
  }
}

void TetrisDemoPlugin::stepGravity() {
  unsigned long now = millis();
  if (now - lastGravity_ < GRAVITY_MS) return;
  lastGravity_ = now;
  Piece moved = active_;
  moved.y++;
  if (!collides(moved)) {
    active_ = moved;
  } else {
    // lock
    if (lockUntil_==0) lockUntil_ = now + LOCK_MS;
    if (now >= lockUntil_) {
      placeActive();
      startClear();
    }
  }
}

int TetrisDemoPlugin::simulateDropY(uint8_t type, uint8_t rot, int x) const {
  // Clamp x so that the piece stays within board horizontally for this rotation
  uint16_t mask = SHAPES[type][rot];
  int minDX = 3, maxDX = 0;
  for (int dy=0; dy<4; ++dy) for (int dx=0; dx<4; ++dx) if (mask & (1 << ((3-dy)*4 + (3-dx)))) {
    if (dx < minDX) minDX = dx; if (dx > maxDX) maxDX = dx;
  }
  if (x < -minDX) x = -minDX;
  if (x > (BOARD_W - 1) - maxDX) x = (BOARD_W - 1) - maxDX;

  Piece p{type, rot, x, 0};
  while (true) {
    Piece n = p; n.y++;
    if (collides(n)) return p.y;
    p = n;
  }
}

int TetrisDemoPlugin::evaluateBoardAfter(uint8_t type, uint8_t rot, int x) const {
  int y = simulateDropY(type, rot, x);
  auto boardCopy = board_;
  uint16_t mask = SHAPES[type][rot];
  // Place blocks safely within bounds
  for (int dy=0; dy<4; ++dy) for (int dx=0; dx<4; ++dx) if (mask & (1 << ((3-dy)*4 + (3-dx)))) {
    int bx = x + dx, by = y + dy;
    if (bx>=0 && bx<BOARD_W && by>=0 && by<BOARD_H) boardCopy[by][bx] = 1;
  }
  // Count completed lines
  int linesCleared = 0;
  for (int ry=0; ry<BOARD_H; ++ry) {
    bool full = true; for (int cx=0; cx<BOARD_W; ++cx) full &= (boardCopy[ry][cx] != 0);
    if (full) linesCleared++;
  }
  // Heights and holes
  int heightSum = 0, holes = 0, bumpiness = 0;
  int prevHeight = -1;
  for (int cx=0; cx<BOARD_W; ++cx) {
    int top = -1;
    for (int cy=0; cy<BOARD_H; ++cy) if (boardCopy[cy][cx]) { top = cy; break; }
    int h = (top == -1) ? 0 : (BOARD_H - top);
    heightSum += h;
    if (top != -1) {
      for (int cy=top+1; cy<BOARD_H; ++cy) if (!boardCopy[cy][cx]) holes++;
    }
    if (prevHeight != -1) bumpiness += abs(h - prevHeight);
    prevHeight = h;
  }
  // Lower is better
  return heightSum*2 + holes*12 + bumpiness*1 - linesCleared*60;
}

void TetrisDemoPlugin::chooseBestPlacement(uint8_t pieceType, uint8_t &bestRot, int &bestX) const {
  int bestScore = 1e9;
  bestRot = 0; bestX = BOARD_W/2 - 2;
  for (int rot=0; rot<4; ++rot) {
    // compute horizontal extents of the shape in this rotation
    uint16_t mask = SHAPES[pieceType][rot];
    int minDX = 3, maxDX = 0;
    for (int dy=0; dy<4; ++dy) for (int dx=0; dx<4; ++dx) if (mask & (1 << ((3-dy)*4 + (3-dx)))) {
      if (dx < minDX) minDX = dx; if (dx > maxDX) maxDX = dx;
    }
    int minX = -minDX; // so that (x+minDX) >= 0
    int maxX = (BOARD_W - 1) - maxDX; // so that (x+maxDX) <= BOARD_W-1
    for (int x=minX; x<=maxX; ++x) {
      Piece p{(uint8_t)pieceType, (uint8_t)rot, x, 0};
      if (collides(p)) continue;
      int score = evaluateBoardAfter(pieceType, rot, x);
      if (score < bestScore) { bestScore = score; bestRot = (uint8_t)rot; bestX = x; }
    }
  }
}

void TetrisDemoPlugin::render() {
  Screen.beginUpdate();
  Screen.clear();

  // draw fixed board (visible part rows 4..19 at y 0..15)
  for (int y=VIS_Y_OFFSET; y<BOARD_H; ++y) {
    for (int x=0; x<BOARD_W; ++x) {
      if (board_[y][x]) {
        Screen.setPixel(3+x, y-VIS_Y_OFFSET, 1, BRIGHT_FIXED);
      }
    }
  }

  // line clear blink
  if (clearing_) {
    for (int y=VIS_Y_OFFSET; y<BOARD_H; ++y) if (clearRows_[y]) {
      for (int x=0; x<BOARD_W; ++x) Screen.setPixel(3+x, y-VIS_Y_OFFSET, 1, 255);
    }
  }

  // draw active piece
  uint16_t mask = SHAPES[active_.type][active_.rot];
  for (int dy=0; dy<4; ++dy) for (int dx=0; dx<4; ++dx) {
    if (mask & (1 << ((3-dy)*4 + (3-dx)))) {
      int px = active_.x + dx;
      int py = active_.y + dy;
      if (py>=VIS_Y_OFFSET) Screen.setPixel(3+px, py-VIS_Y_OFFSET, 1, BRIGHT_ACTIVE);
    }
  }

  // draw next preview at top-right (stay within x=13..15)
  uint16_t nmask = SHAPES[nextType_][0];
  for (int dy=0; dy<4; ++dy) for (int dx=0; dx<4; ++dx) if (nmask & (1 << ((3-dy)*4 + (3-dx)))) {
    int px = 14 + dx - 2; // 12..15 → clamp to 13..15 below
    int py = 1 + dy;
    if (px < 13) px = 13;
    if (px > 15) px = 15;
    if (py>=0 && py<16) Screen.setPixel(px, py, 1, BRIGHT_UI);
  }

  Screen.endUpdate();
}

void TetrisDemoPlugin::setup() {
  resetBoard();
  // Seed nextType_ randomly for varied starts
#ifdef ESP32
  nextType_ = esp_random() % 7;
#else
  static bool seeded=false; if(!seeded){ srand((unsigned)millis()); seeded=true; }
  nextType_ = rand() % 7;
#endif
  spawnPiece();
}

void TetrisDemoPlugin::loop() {
  unsigned long now = millis();
  if (pauseUntil_ && now < pauseUntil_) { render(); return; }
  if (clearing_) {
    if (now >= clearUntil_) applyClear();
    render();
    return;
  }

  // Restart if overflow reached hidden rows
  if (isOverflow()) { restartDemo(); render(); return; }

  // Manual window: if recent manual command, pause AI homing briefly
  if (now < manualUntil_) {
    stepGravity();
    render();
    return;
  }

  stepHoming();
  stepGravity();
  render();
  yield(); // allow WiFi/Async server to run
}

bool TetrisDemoPlugin::isOverflow() const {
  for (int y=0; y<VIS_Y_OFFSET; ++y)
    for (int x=0; x<BOARD_W; ++x)
      if (board_[y][x]) return true;
  return false;
}

void TetrisDemoPlugin::restartDemo() {
  resetBoard();
  nextType_ = 0;
  spawnPiece();
}


void TetrisDemoPlugin::websocketHook(DynamicJsonDocument &event) {
  const char* evt = event["event"];
  if (!evt) return;
  // Allow manual rotate/shift in demo via websocket
  if (!strcmp(evt, "tetris")) {
    const char* action = event["action"] | "";
    Piece p = active_;
    if (!strcmp(action, "rotate")) {
      p.rot = (p.rot + 1) % 4;
      if (!collides(p)) { active_ = p; manualUntil_ = millis() + 600; }
    } else if (!strcmp(action, "left")) {
      p.x -= 1; if (!collides(p)) { active_ = p; manualUntil_ = millis() + 400; }
    } else if (!strcmp(action, "right")) {
      p.x += 1; if (!collides(p)) { active_ = p; manualUntil_ = millis() + 400; }
    } else if (!strcmp(action, "softDrop")) {
      p.y += 1; if (!collides(p)) { active_ = p; manualUntil_ = millis() + 200; }
    } else if (!strcmp(action, "hardDrop")) {
      while (true) { Piece n = active_; n.y++; if (collides(n)) break; active_ = n; }
      manualUntil_ = 0; // place immediately on next gravity tick
    }
  }
}

const char* TetrisDemoPlugin::getName() const { return "Tetris (Demo)"; }

