#include <Arduino_LED_Matrix.h>

ArduinoLEDMatrix matrix;

// ---------------------------------------------------------------------------
// Hardware configuration (edit these pins to match your control rig)
// ---------------------------------------------------------------------------
constexpr uint8_t PIN_JOY_X = A0;   // horizontal
constexpr uint8_t PIN_JOY_Y = A1;   // vertical
constexpr uint8_t PIN_FIRE = 2;
constexpr uint8_t PIN_SMART = 3;
constexpr uint8_t PIN_HYPER = 4;
constexpr uint8_t PIN_BUZZER = 9;

// ---------------------------------------------------------------------------
// Matrix and world constants
// ---------------------------------------------------------------------------
constexpr int MATRIX_W = 12;
constexpr int MATRIX_H = 8;
constexpr int HUD_Y = 0;
constexpr int PLAYFIELD_TOP = 1;
constexpr int PLAYFIELD_H = MATRIX_H - 1;

constexpr int WORLD_W = 192;
constexpr int MAX_ENEMIES = 24;
constexpr int MAX_BULLETS = 20;
constexpr int MAX_HUMANS = 10;

constexpr uint32_t FRAME_MS = 33;  // ~30 FPS
constexpr uint16_t INPUT_DEADZONE = 120;
constexpr float PLAYER_SPEED = 0.20f;
constexpr float BULLET_SPEED = 0.42f;
constexpr float ENEMY_BULLET_SPEED = 0.24f;

enum class GameState : uint8_t {
  TITLE = 0,
  PLAYING = 1,
  WAVE_CLEAR = 2,
  GAME_OVER = 3
};

enum class EnemyType : uint8_t {
  LANDER = 0,
  MUTANT = 1,
  BOMBER = 2,
  POD = 3,
  SWARMER = 4,
  BAITER = 5
};

struct Human {
  bool active;
  bool abducted;
  bool falling;
  bool carriedByPlayer;
  int8_t carrierEnemy;
  float x;
  float y;
  float vy;
};

struct Enemy {
  bool active;
  EnemyType type;
  float x;
  float y;
  float vx;
  float vy;
  int8_t targetHuman;
  int8_t carryingHuman;
  uint16_t shootCooldown;
  uint16_t behaviorTimer;
};

struct Bullet {
  bool active;
  bool fromPlayer;
  float x;
  float y;
  float vx;
  float vy;
  uint8_t ttl;
};

struct Player {
  float x;
  float y;
  int8_t facing;
  bool invulnerable;
  uint16_t invulnTimer;
  uint16_t fireCooldown;
  int8_t carryingHuman;
};

// ---------------------------------------------------------------------------
// Game state
// ---------------------------------------------------------------------------
GameState gameState = GameState::TITLE;
Player player {};
Enemy enemies[MAX_ENEMIES];
Bullet bullets[MAX_BULLETS];
Human humans[MAX_HUMANS];
uint8_t terrain[WORLD_W];
uint8_t frameBuffer[MATRIX_W * MATRIX_H];

uint8_t lives = 3;
uint8_t smartBombs = 3;
uint8_t wave = 0;
uint16_t activeEnemies = 0;
uint32_t score = 0;
uint32_t highScore = 0;

uint32_t lastFrameMs = 0;
uint32_t waveClearStartMs = 0;
uint32_t stateTimerMs = 0;
uint32_t lastBaiterSpawnMs = 0;

bool btnFire = false;
bool btnSmart = false;
bool btnHyper = false;
bool prevFire = false;
bool prevSmart = false;
bool prevHyper = false;

// ---------------------------------------------------------------------------
// Utility
// ---------------------------------------------------------------------------
float wrapX(float x) {
  while (x < 0.0f) x += WORLD_W;
  while (x >= WORLD_W) x -= WORLD_W;
  return x;
}

int wrapInt(int x, int w) {
  int r = x % w;
  return (r < 0) ? (r + w) : r;
}

float wrappedDelta(float from, float to) {
  float d = to - from;
  if (d > (WORLD_W / 2.0f)) d -= WORLD_W;
  if (d < -(WORLD_W / 2.0f)) d += WORLD_W;
  return d;
}

float absf(float v) {
  return (v < 0.0f) ? -v : v;
}

int iround(float v) {
  return (int)(v + ((v >= 0.0f) ? 0.5f : -0.5f));
}

float terrainAt(float worldX) {
  int xi = wrapInt(iround(worldX), WORLD_W);
  return (float)terrain[xi];
}

void toneEvent(uint16_t freq, uint16_t ms) {
  tone(PIN_BUZZER, freq, ms);
}

void clearFrame() {
  for (int i = 0; i < MATRIX_W * MATRIX_H; ++i) {
    frameBuffer[i] = 0;
  }
}

void putPixel(int x, int y, bool on = true) {
  if (x < 0 || x >= MATRIX_W || y < 0 || y >= MATRIX_H) return;
  frameBuffer[y * MATRIX_W + x] = on ? 1 : 0;
}

int worldToScreenX(float worldX, float camX) {
  float d = wrappedDelta(camX, worldX);
  return iround(4.0f + d);  // player is biased left like Defender
}

int worldToScreenY(float worldY) {
  return PLAYFIELD_TOP + iround(worldY);
}

void renderFrame() {
  matrix.loadPixels(frameBuffer, MATRIX_W * MATRIX_H);
}

int16_t readAxisCentered(uint8_t pin) {
  int v = analogRead(pin) - 512;
  if (v > -INPUT_DEADZONE && v < INPUT_DEADZONE) return 0;
  return (int16_t)v;
}

void readControls() {
  btnFire = (digitalRead(PIN_FIRE) == LOW);
  btnSmart = (digitalRead(PIN_SMART) == LOW);
  btnHyper = (digitalRead(PIN_HYPER) == LOW);
}

void resetArrays() {
  for (int i = 0; i < MAX_ENEMIES; ++i) enemies[i].active = false;
  for (int i = 0; i < MAX_BULLETS; ++i) bullets[i].active = false;
  for (int i = 0; i < MAX_HUMANS; ++i) humans[i].active = false;
}

int countLiveHumans() {
  int n = 0;
  for (int i = 0; i < MAX_HUMANS; ++i) {
    if (humans[i].active && !humans[i].abducted && !humans[i].falling && !humans[i].carriedByPlayer) {
      ++n;
    }
  }
  return n;
}

void generateTerrain() {
  int h = 4;
  for (int x = 0; x < WORLD_W; ++x) {
    h += random(-1, 2);
    if (h < 3) h = 3;
    if (h > 6) h = 6;
    terrain[x] = (uint8_t)h;
  }
}

void spawnHumans() {
  for (int i = 0; i < MAX_HUMANS; ++i) {
    humans[i].active = true;
    humans[i].abducted = false;
    humans[i].falling = false;
    humans[i].carriedByPlayer = false;
    humans[i].carrierEnemy = -1;
    humans[i].vy = 0.0f;

    int x = (i + 1) * (WORLD_W / (MAX_HUMANS + 1));
    humans[i].x = (float)x;
    humans[i].y = terrain[x] - 1.0f;
  }
}

int findFreeEnemySlot() {
  for (int i = 0; i < MAX_ENEMIES; ++i) {
    if (!enemies[i].active) return i;
  }
  return -1;
}

void spawnEnemy(EnemyType type, float x, float y) {
  int idx = findFreeEnemySlot();
  if (idx < 0) return;

  Enemy &e = enemies[idx];
  e.active = true;
  e.type = type;
  e.x = wrapX(x);
  e.y = y;
  e.vx = ((random(0, 2) == 0) ? -1.0f : 1.0f) * (0.03f + (random(0, 10) * 0.004f));
  e.vy = 0.0f;
  e.targetHuman = -1;
  e.carryingHuman = -1;
  e.shootCooldown = 40 + random(0, 120);
  e.behaviorTimer = 30 + random(0, 90);
  ++activeEnemies;
}

void splitPodIntoSwarmers(const Enemy &pod) {
  for (int i = 0; i < 3; ++i) {
    spawnEnemy(EnemyType::SWARMER, pod.x + random(-2, 3), pod.y + random(-1, 2));
  }
}

void initWave(uint8_t w) {
  resetArrays();
  generateTerrain();
  spawnHumans();

  uint8_t landers = (uint8_t)min(10, 4 + (int)w);
  uint8_t bombers = (uint8_t)min(4, (int)w / 2);
  uint8_t pods = (uint8_t)min(3, (int)w / 3);

  for (uint8_t i = 0; i < landers; ++i) {
    spawnEnemy(EnemyType::LANDER, random(0, WORLD_W), random(0, 4));
  }
  for (uint8_t i = 0; i < bombers; ++i) {
    spawnEnemy(EnemyType::BOMBER, random(0, WORLD_W), random(1, 5));
  }
  for (uint8_t i = 0; i < pods; ++i) {
    spawnEnemy(EnemyType::POD, random(0, WORLD_W), random(1, 4));
  }

  player.x = WORLD_W / 2.0f;
  player.y = 2.0f;
  player.facing = 1;
  player.invulnerable = true;
  player.invulnTimer = 60;
  player.fireCooldown = 0;
  player.carryingHuman = -1;

  smartBombs = 3;
  lastBaiterSpawnMs = millis();
  toneEvent(700 + (w * 20), 80);
}

void newGame() {
  score = 0;
  lives = 3;
  wave = 1;
  activeEnemies = 0;
  initWave(wave);
  gameState = GameState::PLAYING;
}

void killEnemy(int idx) {
  Enemy &e = enemies[idx];
  if (!e.active) return;

  uint16_t points = 0;
  switch (e.type) {
    case EnemyType::LANDER: points = 150; break;
    case EnemyType::MUTANT: points = 150; break;
    case EnemyType::BOMBER: points = 250; break;
    case EnemyType::POD: points = 1000; break;
    case EnemyType::SWARMER: points = 150; break;
    case EnemyType::BAITER: points = 200; break;
  }
  score += points;

  if (e.type == EnemyType::POD) {
    splitPodIntoSwarmers(e);
  }

  if (e.carryingHuman >= 0 && e.carryingHuman < MAX_HUMANS) {
    Human &h = humans[e.carryingHuman];
    if (h.active) {
      h.abducted = false;
      h.carrierEnemy = -1;
      h.falling = true;
      h.vy = 0.0f;
    }
  }

  e.active = false;
  if (activeEnemies > 0) --activeEnemies;
  toneEvent(2400, 22);
}

void killHuman(int idx) {
  if (idx < 0 || idx >= MAX_HUMANS) return;
  humans[idx].active = false;
  humans[idx].abducted = false;
  humans[idx].falling = false;
  humans[idx].carriedByPlayer = false;
}

void loseLife() {
  if (player.invulnerable) return;
  toneEvent(120, 180);
  if (lives > 0) --lives;

  if (lives == 0) {
    if (score > highScore) highScore = score;
    gameState = GameState::GAME_OVER;
    stateTimerMs = millis();
    return;
  }

  player.x = wrapX(player.x + random(-12, 13));
  player.y = 1.0f + random(0, 3);
  player.invulnerable = true;
  player.invulnTimer = 80;
  player.carryingHuman = -1;
}

void spawnBullet(bool fromPlayer, float x, float y, float vx, float vy, uint8_t ttl = 35) {
  for (int i = 0; i < MAX_BULLETS; ++i) {
    if (!bullets[i].active) {
      bullets[i].active = true;
      bullets[i].fromPlayer = fromPlayer;
      bullets[i].x = wrapX(x);
      bullets[i].y = y;
      bullets[i].vx = vx;
      bullets[i].vy = vy;
      bullets[i].ttl = ttl;
      return;
    }
  }
}

void useSmartBomb() {
  if (smartBombs == 0) return;
  --smartBombs;

  for (int i = 0; i < MAX_ENEMIES; ++i) {
    if (enemies[i].active) killEnemy(i);
  }
  for (int i = 0; i < MAX_BULLETS; ++i) {
    if (!bullets[i].active) continue;
    if (!bullets[i].fromPlayer) bullets[i].active = false;
  }
  toneEvent(1500, 90);
}

void doHyperspace() {
  if (random(0, 100) < 78) {
    player.x = random(0, WORLD_W);
    player.y = random(0, 4);
    player.invulnerable = true;
    player.invulnTimer = 45;
    toneEvent(900, 40);
  } else {
    toneEvent(100, 120);
    loseLife();
  }
}

void maybeSpawnBaiter() {
  if (millis() - lastBaiterSpawnMs < 18000UL) return;
  lastBaiterSpawnMs = millis();

  int baiterCount = 0;
  for (int i = 0; i < MAX_ENEMIES; ++i) {
    if (enemies[i].active && enemies[i].type == EnemyType::BAITER) {
      ++baiterCount;
    }
  }
  if (baiterCount < 2) {
    spawnEnemy(EnemyType::BAITER, wrapX(player.x + random(-20, 21)), random(0, 4));
  }
}

int findClosestHuman(float ex) {
  int idx = -1;
  float best = 10000.0f;
  for (int i = 0; i < MAX_HUMANS; ++i) {
    if (!humans[i].active || humans[i].abducted || humans[i].falling || humans[i].carriedByPlayer) continue;
    float d = absf(wrappedDelta(ex, humans[i].x));
    if (d < best) {
      best = d;
      idx = i;
    }
  }
  return idx;
}

void updateHumans() {
  for (int i = 0; i < MAX_HUMANS; ++i) {
    Human &h = humans[i];
    if (!h.active) continue;

    if (h.carriedByPlayer) {
      h.x = player.x;
      h.y = player.y + 1.0f;
      if (h.y >= terrainAt(h.x) - 1.1f) {
        h.carriedByPlayer = false;
        h.abducted = false;
        h.falling = false;
        h.vy = 0.0f;
        h.y = terrainAt(h.x) - 1.0f;
        player.carryingHuman = -1;
        score += 250;
        toneEvent(1200, 16);
      }
      continue;
    }

    if (h.falling) {
      h.vy += 0.05f;
      h.y += h.vy;
      float safeY = terrainAt(h.x) - 1.0f;
      if (h.y >= safeY) {
        if (h.vy > 0.38f) {
          killHuman(i);
        } else {
          h.falling = false;
          h.abducted = false;
          h.vy = 0.0f;
          h.y = safeY;
        }
      }
      continue;
    }
  }
}

void updateEnemies() {
  for (int i = 0; i < MAX_ENEMIES; ++i) {
    Enemy &e = enemies[i];
    if (!e.active) continue;

    if (e.shootCooldown > 0) --e.shootCooldown;
    if (e.behaviorTimer > 0) --e.behaviorTimer;

    switch (e.type) {
      case EnemyType::LANDER: {
        if (e.carryingHuman >= 0) {
          Human &h = humans[e.carryingHuman];
          e.vx = 0.0f;
          e.vy = -0.035f;
          if (h.active) {
            h.x = e.x;
            h.y = e.y + 0.6f;
          }
          if (e.y < 0.3f) {
            if (h.active) killHuman(e.carryingHuman);
            e.carryingHuman = -1;
            e.type = EnemyType::MUTANT;
            e.vx = (wrappedDelta(e.x, player.x) > 0.0f) ? 0.09f : -0.09f;
            e.vy = 0.0f;
            toneEvent(400, 25);
          }
        } else {
          if (e.targetHuman < 0 || e.targetHuman >= MAX_HUMANS || !humans[e.targetHuman].active || humans[e.targetHuman].abducted) {
            e.targetHuman = findClosestHuman(e.x);
          }

          if (e.targetHuman >= 0) {
            Human &h = humans[e.targetHuman];
            float dx = wrappedDelta(e.x, h.x);
            float dy = (h.y - 0.3f) - e.y;
            e.vx = (dx > 0.0f ? 1.0f : -1.0f) * 0.05f;
            e.vy = (dy > 0.0f ? 1.0f : -1.0f) * 0.025f;

            if (absf(dx) < 0.4f && absf(dy) < 0.5f && !h.abducted) {
              h.abducted = true;
              h.carrierEnemy = i;
              e.carryingHuman = e.targetHuman;
              e.targetHuman = -1;
              toneEvent(500, 18);
            }
          } else if (e.behaviorTimer == 0) {
            e.behaviorTimer = 30 + random(0, 100);
            e.vx = ((random(0, 2) == 0) ? -1.0f : 1.0f) * (0.03f + random(0, 6) * 0.006f);
            e.vy = ((random(0, 2) == 0) ? -1.0f : 1.0f) * 0.012f;
          }
        }

        if (e.shootCooldown == 0 && random(0, 100) < 5) {
          float dx = wrappedDelta(e.x, player.x);
          float sx = (dx > 0.0f) ? ENEMY_BULLET_SPEED : -ENEMY_BULLET_SPEED;
          float dy = ((player.y - e.y) > 0.0f) ? 0.03f : -0.03f;
          spawnBullet(false, e.x, e.y, sx, dy);
          e.shootCooldown = 55 + random(0, 90);
        }
      } break;

      case EnemyType::MUTANT: {
        float dx = wrappedDelta(e.x, player.x);
        float dy = player.y - e.y;
        e.vx = (dx > 0.0f ? 1.0f : -1.0f) * 0.085f;
        e.vy = (dy > 0.0f ? 1.0f : -1.0f) * 0.045f;
        if (e.shootCooldown == 0 && random(0, 100) < 12) {
          spawnBullet(false, e.x, e.y, (dx > 0.0f ? ENEMY_BULLET_SPEED : -ENEMY_BULLET_SPEED), dy * 0.02f);
          e.shootCooldown = 35 + random(0, 50);
        }
      } break;

      case EnemyType::BOMBER: {
        if (e.behaviorTimer == 0) {
          e.behaviorTimer = 50 + random(0, 100);
          e.vx = ((random(0, 2) == 0) ? -0.06f : 0.06f);
          e.vy = ((random(0, 2) == 0) ? -0.01f : 0.01f);
        }
        if (e.shootCooldown == 0) {
          spawnBullet(false, e.x, e.y + 0.2f, 0.0f, 0.16f, 30);
          e.shootCooldown = 35 + random(0, 40);
        }
      } break;

      case EnemyType::POD: {
        float dx = wrappedDelta(e.x, player.x);
        e.vx = (dx > 0.0f ? 1.0f : -1.0f) * 0.04f;
        if (e.behaviorTimer == 0) {
          e.behaviorTimer = 40 + random(0, 90);
          e.vy = ((random(0, 2) == 0) ? -1.0f : 1.0f) * 0.02f;
        }
      } break;

      case EnemyType::SWARMER: {
        float dx = wrappedDelta(e.x, player.x);
        float dy = player.y - e.y;
        e.vx = (dx > 0.0f ? 0.11f : -0.11f);
        e.vy = (dy > 0.0f ? 0.06f : -0.06f);
      } break;

      case EnemyType::BAITER: {
        float dx = wrappedDelta(e.x, player.x);
        float dy = player.y - e.y;
        e.vx = (dx > 0.0f ? 0.13f : -0.13f);
        e.vy = (dy > 0.0f ? 0.07f : -0.07f);
        if (e.shootCooldown == 0) {
          spawnBullet(false, e.x, e.y, (dx > 0.0f ? 0.27f : -0.27f), dy * 0.03f);
          e.shootCooldown = 20 + random(0, 30);
        }
      } break;
    }

    e.x = wrapX(e.x + e.vx);
    e.y += e.vy;
    if (e.y < 0.0f) e.y = 0.0f;
    if (e.y > 6.0f) e.y = 6.0f;

    if (!player.invulnerable) {
      float dx = absf(wrappedDelta(player.x, e.x));
      float dy = absf(player.y - e.y);
      if (dx < 0.45f && dy < 0.45f) {
        loseLife();
      }
    }
  }
}

void updateBullets() {
  for (int i = 0; i < MAX_BULLETS; ++i) {
    Bullet &b = bullets[i];
    if (!b.active) continue;

    b.x = wrapX(b.x + b.vx);
    b.y += b.vy;
    if (b.ttl > 0) --b.ttl;
    if (b.ttl == 0 || b.y < 0.0f || b.y > 6.2f) {
      b.active = false;
      continue;
    }

    float ground = terrainAt(b.x);
    if (b.y >= ground) {
      b.active = false;
      continue;
    }

    if (b.fromPlayer) {
      for (int e = 0; e < MAX_ENEMIES; ++e) {
        if (!enemies[e].active) continue;
        float dx = absf(wrappedDelta(b.x, enemies[e].x));
        float dy = absf(b.y - enemies[e].y);
        if (dx < 0.45f && dy < 0.45f) {
          killEnemy(e);
          b.active = false;
          break;
        }
      }
    } else {
      if (!player.invulnerable) {
        float dx = absf(wrappedDelta(b.x, player.x));
        float dy = absf(b.y - player.y);
        if (dx < 0.45f && dy < 0.45f) {
          b.active = false;
          loseLife();
          continue;
        }
      }
    }
  }
}

void updatePlayer() {
  int16_t ax = readAxisCentered(PIN_JOY_X);
  int16_t ay = readAxisCentered(PIN_JOY_Y);
  float nx = (float)ax / 512.0f;
  float ny = (float)ay / 512.0f;

  player.x = wrapX(player.x + (nx * PLAYER_SPEED));
  player.y += (-ny * (PLAYER_SPEED * 0.72f));

  if (nx > 0.15f) player.facing = 1;
  if (nx < -0.15f) player.facing = -1;

  float ground = terrainAt(player.x) - 0.2f;
  if (player.y > ground) player.y = ground;
  if (player.y < 0.0f) player.y = 0.0f;

  if (player.invulnerable && player.invulnTimer > 0) {
    --player.invulnTimer;
    if (player.invulnTimer == 0) player.invulnerable = false;
  }

  if (player.fireCooldown > 0) --player.fireCooldown;

  if (btnFire && !prevFire && player.fireCooldown == 0) {
    float vx = (player.facing > 0) ? BULLET_SPEED : -BULLET_SPEED;
    spawnBullet(true, player.x + (player.facing * 0.35f), player.y, vx, 0.0f, 26);
    player.fireCooldown = 8;
    toneEvent(2200, 12);
  }

  if (btnSmart && !prevSmart) {
    useSmartBomb();
  }
  if (btnHyper && !prevHyper) {
    doHyperspace();
  }

  // Pick up falling humans.
  if (player.carryingHuman < 0) {
    for (int i = 0; i < MAX_HUMANS; ++i) {
      Human &h = humans[i];
      if (!h.active || !h.falling || h.carriedByPlayer) continue;
      float dx = absf(wrappedDelta(player.x, h.x));
      float dy = absf(player.y - h.y);
      if (dx < 0.45f && dy < 0.55f) {
        h.carriedByPlayer = true;
        h.falling = false;
        h.vy = 0.0f;
        player.carryingHuman = i;
        toneEvent(1400, 12);
        break;
      }
    }
  }
}

void checkWaveClear() {
  bool anyEnemy = false;
  for (int i = 0; i < MAX_ENEMIES; ++i) {
    if (enemies[i].active) {
      anyEnemy = true;
      break;
    }
  }
  if (anyEnemy) return;

  int survivors = countLiveHumans();
  score += survivors * 100;
  gameState = GameState::WAVE_CLEAR;
  waveClearStartMs = millis();
  toneEvent(1800, 100);
}

void updateGameplay() {
  maybeSpawnBaiter();
  updatePlayer();
  updateHumans();
  updateEnemies();
  updateBullets();
  checkWaveClear();
}

void renderHUD() {
  // Lives on the left.
  for (int i = 0; i < lives && i < 3; ++i) {
    putPixel(i, HUD_Y, true);
  }
  // Smart bombs on the right.
  for (int i = 0; i < smartBombs && i < 3; ++i) {
    putPixel(MATRIX_W - 1 - i, HUD_Y, true);
  }
  // Wave marker in center: binary pulse for wave modulo 4.
  uint8_t waveBits = wave & 0x03;
  putPixel(5, HUD_Y, (waveBits & 0x01) != 0);
  putPixel(6, HUD_Y, (waveBits & 0x02) != 0);
}

void renderTerrain(float camX) {
  for (int sx = 0; sx < MATRIX_W; ++sx) {
    float wx = wrapX(camX + (sx - 4));
    int gy = worldToScreenY(terrainAt(wx));
    for (int y = gy; y < MATRIX_H; ++y) {
      putPixel(sx, y, true);
    }
  }
}

void renderHumans(float camX) {
  for (int i = 0; i < MAX_HUMANS; ++i) {
    Human &h = humans[i];
    if (!h.active || h.abducted) continue;
    int sx = worldToScreenX(h.x, camX);
    int sy = worldToScreenY(h.y);
    putPixel(sx, sy, true);
  }
}

void renderEnemies(float camX) {
  for (int i = 0; i < MAX_ENEMIES; ++i) {
    Enemy &e = enemies[i];
    if (!e.active) continue;
    int sx = worldToScreenX(e.x, camX);
    int sy = worldToScreenY(e.y);
    putPixel(sx, sy, true);

    // Minimal 2-pixel signatures for readability.
    if (e.type == EnemyType::MUTANT || e.type == EnemyType::BAITER) {
      putPixel(sx, sy - 1, true);
    } else if (e.type == EnemyType::BOMBER || e.type == EnemyType::POD) {
      putPixel(sx + 1, sy, true);
    }
  }
}

void renderBullets(float camX) {
  for (int i = 0; i < MAX_BULLETS; ++i) {
    if (!bullets[i].active) continue;
    int sx = worldToScreenX(bullets[i].x, camX);
    int sy = worldToScreenY(bullets[i].y);
    putPixel(sx, sy, true);
  }
}

void renderPlayer() {
  if (player.invulnerable && ((millis() / 90UL) % 2UL == 0UL)) return;
  int sx = 4;
  int sy = worldToScreenY(player.y);
  putPixel(sx, sy, true);
  putPixel(sx - player.facing, sy, true);
}

void renderRadar(float camX) {
  // Simple radar overlays on top row center range [3..8].
  for (int i = 0; i < MAX_ENEMIES; ++i) {
    if (!enemies[i].active) continue;
    float d = wrappedDelta(camX, enemies[i].x);
    if (d < -36.0f || d > 36.0f) continue;
    int rx = 6 + (int)(d / 12.0f);
    if (rx < 3) rx = 3;
    if (rx > 8) rx = 8;
    putPixel(rx, HUD_Y, true);
  }
}

void renderTitle() {
  clearFrame();

  // Stylized "D" icon and blinking prompt line.
  putPixel(1, 1); putPixel(1, 2); putPixel(1, 3); putPixel(1, 4); putPixel(1, 5);
  putPixel(2, 1); putPixel(3, 1); putPixel(4, 2); putPixel(4, 3); putPixel(4, 4); putPixel(3, 5); putPixel(2, 5);
  putPixel(6, 2); putPixel(7, 3); putPixel(8, 2);  // ship motif

  if (((millis() / 220UL) % 2UL) == 0UL) {
    for (int x = 0; x < MATRIX_W; ++x) putPixel(x, 7, true);
  }

  renderFrame();
}

void renderGameOver() {
  clearFrame();
  // Explosion-like pattern.
  putPixel(5, 3); putPixel(6, 3); putPixel(5, 4); putPixel(6, 4);
  putPixel(4, 3); putPixel(7, 3); putPixel(5, 2); putPixel(6, 2); putPixel(5, 5); putPixel(6, 5);
  if (((millis() / 200UL) % 2UL) == 0UL) {
    putPixel(3, 1); putPixel(8, 1); putPixel(2, 6); putPixel(9, 6);
  }
  renderFrame();
}

void renderWaveClear() {
  clearFrame();
  uint8_t bars = (uint8_t)min(12, 2 + (wave % 10));
  for (int x = 0; x < bars; ++x) {
    for (int y = 2; y < 6; ++y) {
      putPixel(x, y, true);
    }
  }
  renderFrame();
}

void renderGameplay() {
  clearFrame();
  float camX = player.x;
  renderTerrain(camX);
  renderHumans(camX);
  renderEnemies(camX);
  renderBullets(camX);
  renderPlayer();
  renderHUD();
  renderRadar(camX);
  renderFrame();
}

void setup() {
  matrix.begin();
  pinMode(PIN_FIRE, INPUT_PULLUP);
  pinMode(PIN_SMART, INPUT_PULLUP);
  pinMode(PIN_HYPER, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);

  randomSeed(analogRead(A5));
  resetArrays();

  stateTimerMs = millis();
  lastFrameMs = millis();
}

void loop() {
  uint32_t now = millis();
  if (now - lastFrameMs < FRAME_MS) return;
  lastFrameMs = now;

  readControls();

  switch (gameState) {
    case GameState::TITLE:
      renderTitle();
      if (btnFire && !prevFire) {
        newGame();
      }
      break;

    case GameState::PLAYING:
      updateGameplay();
      renderGameplay();
      break;

    case GameState::WAVE_CLEAR:
      renderWaveClear();
      if (now - waveClearStartMs > 1200UL) {
        ++wave;
        activeEnemies = 0;
        initWave(wave);
        gameState = GameState::PLAYING;
      }
      break;

    case GameState::GAME_OVER:
      renderGameOver();
      if (btnFire && !prevFire && now - stateTimerMs > 500UL) {
        gameState = GameState::TITLE;
      }
      break;
  }

  prevFire = btnFire;
  prevSmart = btnSmart;
  prevHyper = btnHyper;
}
