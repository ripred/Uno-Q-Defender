// Input backends. Set to 0/1 via build flags if needed.
// Example: -DDEFENDER_ENABLE_DPAD=0
// Board-safe defaults keep external GPIO controls disabled because UNO Q onboard
// LED matrix scanning can share pins and cause contention/noisy reads.
#ifndef DEFENDER_ENABLE_ANALOG_JOYSTICK
#define DEFENDER_ENABLE_ANALOG_JOYSTICK 0
#endif

#ifndef DEFENDER_ENABLE_DPAD
#define DEFENDER_ENABLE_DPAD 0
#endif

#ifndef DEFENDER_ENABLE_SERIAL_KEYBOARD
#define DEFENDER_ENABLE_SERIAL_KEYBOARD 1
#endif

#ifndef DEFENDER_ENABLE_PIN_ACTION_BUTTONS
#define DEFENDER_ENABLE_PIN_ACTION_BUTTONS 0
#endif

#ifndef DEFENDER_ENABLE_AUDIO
#define DEFENDER_ENABLE_AUDIO 0
#endif

#ifndef DEFENDER_ENABLE_KEYBOARD_LIB
#define DEFENDER_ENABLE_KEYBOARD_LIB 0
#endif

#ifndef DEFENDER_ENABLE_JOYSTICK_LIB
#define DEFENDER_ENABLE_JOYSTICK_LIB 0
#endif

#include <Arduino_LED_Matrix.h>

#if DEFENDER_ENABLE_KEYBOARD_LIB
  #if defined(__has_include)
    #if __has_include(<Keyboard.h>)
      #include <Keyboard.h>
      #define DEFENDER_HAVE_KEYBOARD_LIB 1
    #endif
  #endif
#endif

#if DEFENDER_ENABLE_JOYSTICK_LIB
  #if defined(__has_include)
    #if __has_include(<Joystick.h>)
      #include <Joystick.h>
      #define DEFENDER_HAVE_JOYSTICK_LIB 1
    #endif
  #endif
#endif

#ifndef DEFENDER_HAVE_KEYBOARD_LIB
#define DEFENDER_HAVE_KEYBOARD_LIB 0
#endif

#ifndef DEFENDER_HAVE_JOYSTICK_LIB
#define DEFENDER_HAVE_JOYSTICK_LIB 0
#endif

#if DEFENDER_ENABLE_KEYBOARD_LIB && !DEFENDER_HAVE_KEYBOARD_LIB
#warning "DEFENDER_ENABLE_KEYBOARD_LIB=1 but <Keyboard.h> was not found for this target."
#endif

#if DEFENDER_ENABLE_JOYSTICK_LIB && !DEFENDER_HAVE_JOYSTICK_LIB
#warning "DEFENDER_ENABLE_JOYSTICK_LIB=1 but <Joystick.h> was not found; joystick hook is disabled."
#endif

ArduinoLEDMatrix matrix;

// ---------------------------------------------------------------------------
// Hardware configuration (edit these pins to match your control rig)
// ---------------------------------------------------------------------------
constexpr uint8_t PIN_JOY_X = A0;   // horizontal
constexpr uint8_t PIN_JOY_Y = A1;   // vertical
constexpr uint8_t PIN_JOY_BTN = 5;
constexpr uint8_t PIN_FIRE = 2;
constexpr uint8_t PIN_SMART = 3;
constexpr uint8_t PIN_HYPER = 4;
constexpr uint8_t PIN_DPAD_LEFT = 6;
constexpr uint8_t PIN_DPAD_RIGHT = 7;
constexpr uint8_t PIN_DPAD_UP = 8;
constexpr uint8_t PIN_DPAD_DOWN = 10;
constexpr uint8_t PIN_BUZZER = 9;

constexpr int8_t JOYSTICK_Y_UP_SIGN = -1;  // flip to +1 if Y axis is inverted

// ---------------------------------------------------------------------------
// Matrix and world constants
// ---------------------------------------------------------------------------
constexpr int MATRIX_W = 13;
constexpr int MATRIX_H = 8;
constexpr int HUD_Y = 0;
constexpr int PLAYFIELD_TOP = 1;
constexpr int PLAYFIELD_H = MATRIX_H - 1;
constexpr int MATRIX_GRAY_BITS = 3;
constexpr uint8_t PIXEL_OFF_LEVEL = 0;
constexpr uint8_t PIXEL_ON_LEVEL = (1U << MATRIX_GRAY_BITS) - 1U;  // 7 with 3 gray bits
constexpr int PLAYER_SCREEN_X = 4;         // keep a left-biased Defender feel
constexpr int RADAR_CENTER_X = MATRIX_W / 2;

constexpr int WORLD_W = 192;
constexpr int MAX_ENEMIES = 24;
constexpr int MAX_BULLETS = 20;
constexpr int MAX_HUMANS = 10;

constexpr uint32_t FRAME_MS = 33;  // ~30 FPS
constexpr uint32_t DEMO_IDLE_TIMEOUT_MS = 60000UL;
constexpr uint32_t DEMO_RESTART_DELAY_MS = 2000UL;
constexpr float USER_MOTION_ACTIVITY_THRESHOLD = 0.90f;
constexpr uint8_t USER_MOTION_STABLE_FRAMES = 18;
constexpr uint16_t INPUT_DEADZONE = 120;
constexpr float PLAYER_SPEED = 0.20f;
constexpr float BULLET_SPEED = 0.42f;
constexpr float ENEMY_BULLET_SPEED = 0.24f;

// Grayscale palette (0..7) and render layer depths.
constexpr uint8_t SHADE_BG_STAR = 1;
constexpr uint8_t SHADE_BG_FAR = 1;
constexpr uint8_t SHADE_BG_MID = 2;
constexpr uint8_t SHADE_BG_NEAR = 2;
constexpr uint8_t SHADE_TERRAIN_FILL = 2;
constexpr uint8_t SHADE_TERRAIN_MID = 3;
constexpr uint8_t SHADE_TERRAIN_TOP = 4;
constexpr uint8_t SHADE_HUMAN_CORE = 6;
constexpr uint8_t SHADE_HUMAN_EDGE = 3;
constexpr uint8_t SHADE_PLAYER_CORE = 7;
constexpr uint8_t SHADE_PLAYER_EDGE = 5;
constexpr uint8_t SHADE_PLAYER_ACCENT = 4;
constexpr uint8_t SHADE_BULLET_PLAYER = 7;
constexpr uint8_t SHADE_BULLET_ENEMY = 4;
constexpr uint8_t SHADE_HUD_MAJOR = 6;
constexpr uint8_t SHADE_HUD_MINOR = 4;
constexpr uint8_t SHADE_RADAR = 5;
constexpr uint8_t SHADE_EDGE_LIGHT = 7;
constexpr uint8_t SHADE_EDGE_DARK = 1;

constexpr int16_t DEPTH_CLEAR = -32768;
constexpr int16_t DEPTH_BG_FAR = 10;
constexpr int16_t DEPTH_BG_MID = 18;
constexpr int16_t DEPTH_BG_NEAR = 26;
constexpr int16_t DEPTH_TERRAIN = 36;
constexpr int16_t DEPTH_HUMAN_BASE = 56;
constexpr int16_t DEPTH_ENEMY_BASE = 68;
constexpr int16_t DEPTH_BULLET_ENEMY = 80;
constexpr int16_t DEPTH_PLAYER_BASE = 90;
constexpr int16_t DEPTH_BULLET_PLAYER = 98;
constexpr int16_t DEPTH_HUD = 120;
constexpr int16_t DEPTH_UI = 125;

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

struct InputState {
  float moveX;
  float moveY;   // positive is up
  bool fire;
  bool smart;
  bool hyper;
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
int16_t depthBuffer[MATRIX_W * MATRIX_H];

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

InputState inputNow { 0.0f, 0.0f, false, false, false };
InputState userInputNow { 0.0f, 0.0f, false, false, false };
bool userInputActiveNow = false;
bool userActionActiveNow = false;
bool prevFire = false;
bool prevSmart = false;
bool prevHyper = false;
bool demoMode = false;
uint32_t lastUserInputMs = 0;
uint32_t demoGameOverAtMs = 0;
uint8_t aiFireCooldown = 0;
uint8_t aiSmartCooldown = 0;
uint8_t aiHyperCooldown = 0;

int8_t serialMoveX = 0;
int8_t serialMoveY = 0;
uint8_t serialEscState = 0;
uint32_t serialMoveExpiresMs = 0;
bool serialFirePulse = false;
bool serialSmartPulse = false;
bool serialHyperPulse = false;
uint8_t userMotionStableCount = 0;
int8_t lastMotionDirX = 0;
int8_t lastMotionDirY = 0;

#if DEFENDER_HAVE_KEYBOARD_LIB
#ifndef KEY_LEFT_ARROW
#define KEY_LEFT_ARROW 0xD8
#endif
#ifndef KEY_RIGHT_ARROW
#define KEY_RIGHT_ARROW 0xD7
#endif
#ifndef KEY_UP_ARROW
#define KEY_UP_ARROW 0xDA
#endif
#ifndef KEY_DOWN_ARROW
#define KEY_DOWN_ARROW 0xD9
#endif
// Optional host-keyboard hook. Override in another .ino/.h file to return key state.
bool defenderKeyboardPressed(uint8_t keycode) __attribute__((weak));
bool defenderKeyboardPressed(uint8_t keycode) {
  (void)keycode;
  return false;
}
#endif

#if DEFENDER_HAVE_JOYSTICK_LIB
// Optional joystick hook. Override to feed normalized axis/buttons from your Joystick.h implementation.
void defenderJoystickSample(float *outX, float *outY, bool *outFire, bool *outSmart, bool *outHyper) __attribute__((weak));
void defenderJoystickSample(float *outX, float *outY, bool *outFire, bool *outSmart, bool *outHyper) {
  *outX = 0.0f;
  *outY = 0.0f;
  *outFire = false;
  *outSmart = false;
  *outHyper = false;
}
#endif

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
#if DEFENDER_ENABLE_AUDIO
  tone(PIN_BUZZER, freq, ms);
#else
  (void)freq;
  (void)ms;
#endif
}

void clearFrame() {
  for (int i = 0; i < MATRIX_W * MATRIX_H; ++i) {
    frameBuffer[i] = PIXEL_OFF_LEVEL;
    depthBuffer[i] = DEPTH_CLEAR;
  }
}

uint8_t clampShade(int shade) {
  if (shade < 0) return PIXEL_OFF_LEVEL;
  if (shade > PIXEL_ON_LEVEL) return PIXEL_ON_LEVEL;
  return (uint8_t)shade;
}

int pixelIndex(int x, int y) {
  return y * MATRIX_W + x;
}

void putDepthPixel(int x, int y, int16_t depth, uint8_t shade) {
  if (x < 0 || x >= MATRIX_W || y < 0 || y >= MATRIX_H) return;
  int idx = pixelIndex(x, y);
  if (depth > depthBuffer[idx] || (depth == depthBuffer[idx] && shade > frameBuffer[idx])) {
    frameBuffer[idx] = clampShade(shade);
    depthBuffer[idx] = depth;
  }
}

uint8_t contrastingEdgeShade(uint8_t behindShade, uint8_t preferred) {
  if (behindShade == PIXEL_OFF_LEVEL) return preferred;
  if (behindShade >= 4) return SHADE_EDGE_DARK;
  return SHADE_EDGE_LIGHT;
}

void putEdgePixel(int x, int y, int16_t depth, uint8_t preferred) {
  if (x < 0 || x >= MATRIX_W || y < 0 || y >= MATRIX_H) return;
  int idx = pixelIndex(x, y);
  uint8_t shade = contrastingEdgeShade(frameBuffer[idx], preferred);
  putDepthPixel(x, y, depth, shade);
}

void putPixel(int x, int y, bool on = true) {
  putDepthPixel(x, y, DEPTH_UI, on ? PIXEL_ON_LEVEL : PIXEL_OFF_LEVEL);
}

uint8_t hash8(uint16_t v) {
  v ^= (uint16_t)(v << 7);
  v ^= (uint16_t)(v >> 9);
  v = (uint16_t)(v * 109U);
  return (uint8_t)v;
}

int worldToScreenX(float worldX, float camX) {
  float d = wrappedDelta(camX, worldX);
  return iround((float)PLAYER_SCREEN_X + d);  // player is biased left like Defender
}

int worldToScreenY(float worldY) {
  return PLAYFIELD_TOP + iround(worldY);
}

void renderFrame() {
  matrix.draw(frameBuffer);
}

int16_t readAxisCentered(uint8_t pin) {
  int v = analogRead(pin) - 512;
  if (v > -INPUT_DEADZONE && v < INPUT_DEADZONE) return 0;
  return (int16_t)v;
}

float clampUnit(float v) {
  if (v < -1.0f) return -1.0f;
  if (v > 1.0f) return 1.0f;
  return v;
}

int8_t readDigitalAxis(uint8_t negativePin, uint8_t positivePin) {
  int8_t v = 0;
  if (digitalRead(negativePin) == LOW) --v;
  if (digitalRead(positivePin) == LOW) ++v;
  return v;
}

void markSerialMove(int8_t x, int8_t y) {
  serialMoveX = x;
  serialMoveY = y;
  serialMoveExpiresMs = millis() + 220UL;
}

void handleSerialCommandChar(char c) {
  if (serialEscState == 1) {
    serialEscState = (c == '[') ? 2 : 0;
    return;
  }
  if (serialEscState == 2) {
    serialEscState = 0;
    if (c == 'A') markSerialMove(0, 1);
    if (c == 'B') markSerialMove(0, -1);
    if (c == 'C') markSerialMove(1, 0);
    if (c == 'D') markSerialMove(-1, 0);
    return;
  }
  if ((uint8_t)c == 27U) {
    serialEscState = 1;
    return;
  }

  switch (c) {
    case 'a':
    case 'A':
      markSerialMove(-1, 0);
      break;
    case 'd':
    case 'D':
      markSerialMove(1, 0);
      break;
    case 'w':
    case 'W':
      markSerialMove(0, 1);
      break;
    case 's':
    case 'S':
      markSerialMove(0, -1);
      break;
    case 'q':
    case 'Q':
      markSerialMove(-1, 1);
      break;
    case 'e':
    case 'E':
      markSerialMove(1, 1);
      break;
    case 'z':
    case 'Z':
      markSerialMove(-1, -1);
      break;
    case 'c':
    case 'C':
      markSerialMove(1, -1);
      break;
    case 'x':
    case 'X':
    case '0':
      markSerialMove(0, 0);
      break;
    case ' ':
    case 'f':
    case 'F':
    case 'j':
    case 'J':
      serialFirePulse = true;
      break;
    case 'k':
    case 'K':
    case 'b':
    case 'B':
      serialSmartPulse = true;
      break;
    case 'h':
    case 'H':
    case 'l':
    case 'L':
      serialHyperPulse = true;
      break;
  }
}

void pollSerialKeyboard() {
#if DEFENDER_ENABLE_SERIAL_KEYBOARD
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    handleSerialCommandChar(c);
  }
  if (millis() > serialMoveExpiresMs) {
    serialMoveX = 0;
    serialMoveY = 0;
  }
#endif
}

void applyPinActionButtons(InputState &s) {
#if DEFENDER_ENABLE_PIN_ACTION_BUTTONS
  if (digitalRead(PIN_FIRE) == LOW) s.fire = true;
  if (digitalRead(PIN_SMART) == LOW) s.smart = true;
  if (digitalRead(PIN_HYPER) == LOW) s.hyper = true;
#endif
}

void applyAnalogJoystick(InputState &s) {
#if DEFENDER_ENABLE_ANALOG_JOYSTICK
  int16_t ax = 0;
  int16_t ay = 0;
#if DEFENDER_HAVE_JOYSTICK_LIB
  float jx = 0.0f;
  float jy = 0.0f;
  bool jf = false;
  bool js = false;
  bool jh = false;
  defenderJoystickSample(&jx, &jy, &jf, &js, &jh);
  s.moveX += jx;
  s.moveY += jy;
  if (jf) s.fire = true;
  if (js) s.smart = true;
  if (jh) s.hyper = true;
#endif
  ax = readAxisCentered(PIN_JOY_X);
  ay = readAxisCentered(PIN_JOY_Y);
  s.moveX += (float)ax / 512.0f;
  s.moveY += ((float)ay / 512.0f) * (float)JOYSTICK_Y_UP_SIGN;
  if (digitalRead(PIN_JOY_BTN) == LOW) s.fire = true;
#endif
}

void applyDpad(InputState &s) {
#if DEFENDER_ENABLE_DPAD
  s.moveX += (float)readDigitalAxis(PIN_DPAD_LEFT, PIN_DPAD_RIGHT);
  s.moveY += (float)readDigitalAxis(PIN_DPAD_DOWN, PIN_DPAD_UP);
#endif
}

void applySerialInput(InputState &s) {
#if DEFENDER_ENABLE_SERIAL_KEYBOARD
  s.moveX += (float)serialMoveX;
  s.moveY += (float)serialMoveY;
  if (serialFirePulse) {
    s.fire = true;
    serialFirePulse = false;
  }
  if (serialSmartPulse) {
    s.smart = true;
    serialSmartPulse = false;
  }
  if (serialHyperPulse) {
    s.hyper = true;
    serialHyperPulse = false;
  }
#endif
}

void applyKeyboardLibInput(InputState &s) {
#if DEFENDER_HAVE_KEYBOARD_LIB
  if (defenderKeyboardPressed('a') || defenderKeyboardPressed('A') || defenderKeyboardPressed(KEY_LEFT_ARROW)) s.moveX -= 1.0f;
  if (defenderKeyboardPressed('d') || defenderKeyboardPressed('D') || defenderKeyboardPressed(KEY_RIGHT_ARROW)) s.moveX += 1.0f;
  if (defenderKeyboardPressed('w') || defenderKeyboardPressed('W') || defenderKeyboardPressed(KEY_UP_ARROW)) s.moveY += 1.0f;
  if (defenderKeyboardPressed('s') || defenderKeyboardPressed('S') || defenderKeyboardPressed(KEY_DOWN_ARROW)) s.moveY -= 1.0f;
  if (defenderKeyboardPressed(' ') || defenderKeyboardPressed('f') || defenderKeyboardPressed('F')) s.fire = true;
  if (defenderKeyboardPressed('b') || defenderKeyboardPressed('B')) s.smart = true;
  if (defenderKeyboardPressed('h') || defenderKeyboardPressed('H')) s.hyper = true;
#endif
}

bool isInputActive(const InputState &s) {
  if (s.fire || s.smart || s.hyper) {
    userMotionStableCount = USER_MOTION_STABLE_FRAMES;
    return true;
  }

  int8_t motionDirX = 0;
  int8_t motionDirY = 0;
  if (absf(s.moveX) > USER_MOTION_ACTIVITY_THRESHOLD) motionDirX = (s.moveX > 0.0f) ? 1 : -1;
  if (absf(s.moveY) > USER_MOTION_ACTIVITY_THRESHOLD) motionDirY = (s.moveY > 0.0f) ? 1 : -1;
  bool motion = (motionDirX != 0) || (motionDirY != 0);

  if (motion) {
    if (motionDirX == lastMotionDirX && motionDirY == lastMotionDirY) {
      if (userMotionStableCount < 255) ++userMotionStableCount;
    } else {
      userMotionStableCount = 1;
      lastMotionDirX = motionDirX;
      lastMotionDirY = motionDirY;
    }
  } else {
    userMotionStableCount = 0;
    lastMotionDirX = 0;
    lastMotionDirY = 0;
  }
  return userMotionStableCount >= USER_MOTION_STABLE_FRAMES;
}

void resetAutoPlayerState() {
  aiFireCooldown = 0;
  aiSmartCooldown = 0;
  aiHyperCooldown = 0;
}

InputState buildAutoPlayerInput() {
  InputState ai { 0.0f, 0.0f, false, false, false };
  if (gameState != GameState::PLAYING) return ai;

  if (aiFireCooldown > 0) --aiFireCooldown;
  if (aiSmartCooldown > 0) --aiSmartCooldown;
  if (aiHyperCooldown > 0) --aiHyperCooldown;

  float avoidX = 0.0f;
  float avoidY = 0.0f;
  int threatScore = 0;

  for (int i = 0; i < MAX_BULLETS; ++i) {
    const Bullet &b = bullets[i];
    if (!b.active || b.fromPlayer) continue;
    float dx = wrappedDelta(player.x, b.x);
    float dy = b.y - player.y;
    if (absf(dx) < 3.2f && absf(dy) < 1.4f) {
      avoidX += (dx > 0.0f) ? -1.0f : 1.0f;
      avoidY += (dy > 0.0f) ? -1.0f : 1.0f;
      ++threatScore;
    }
  }

  for (int i = 0; i < MAX_ENEMIES; ++i) {
    const Enemy &e = enemies[i];
    if (!e.active) continue;
    float dx = wrappedDelta(player.x, e.x);
    float dy = e.y - player.y;
    if (absf(dx) < 1.2f && absf(dy) < 0.9f) {
      avoidX += (dx > 0.0f) ? -1.0f : 1.0f;
      avoidY += (dy > 0.0f) ? -1.0f : 1.0f;
      threatScore += 2;
    }
  }

  bool haveTarget = false;
  float targetX = player.x;
  float targetY = player.y;

  if (player.carryingHuman >= 0) {
    haveTarget = true;
    targetX = player.x;
    targetY = terrainAt(player.x) - 1.0f;
  } else {
    float bestFalling = 10000.0f;
    for (int i = 0; i < MAX_HUMANS; ++i) {
      const Human &h = humans[i];
      if (!h.active || !h.falling || h.carriedByPlayer) continue;
      float dx = absf(wrappedDelta(player.x, h.x));
      float dy = absf(player.y - h.y);
      float score = dx + (dy * 1.8f);
      if (score < bestFalling) {
        bestFalling = score;
        targetX = h.x;
        targetY = h.y;
        haveTarget = true;
      }
    }

    if (!haveTarget) {
      float bestEnemy = 10000.0f;
      for (int i = 0; i < MAX_ENEMIES; ++i) {
        const Enemy &e = enemies[i];
        if (!e.active) continue;
        float dx = wrappedDelta(player.x, e.x);
        float dy = player.y - e.y;
        float score = absf(dx) * 1.1f + absf(dy) * 1.5f;
        if (e.carryingHuman >= 0) score -= 2.5f;
        if (e.type == EnemyType::BAITER || e.type == EnemyType::MUTANT) score -= 0.6f;
        if (score < bestEnemy) {
          bestEnemy = score;
          targetX = e.x;
          targetY = e.y;
          haveTarget = true;
        }
      }
    }
  }

  if (threatScore > 0) {
    ai.moveX = clampUnit(avoidX);
    ai.moveY = clampUnit(avoidY);
  } else if (haveTarget) {
    float dx = wrappedDelta(player.x, targetX);
    float dy = targetY - player.y;
    if (absf(dx) > 0.35f) ai.moveX = (dx > 0.0f) ? 1.0f : -1.0f;
    if (absf(dy) > 0.25f) ai.moveY = (dy < 0.0f) ? 1.0f : -1.0f;  // +moveY means up
  } else {
    ai.moveX = (player.facing > 0) ? 0.35f : -0.35f;
  }

  float ground = terrainAt(player.x) - 0.25f;
  if (player.y > ground - 0.50f) ai.moveY = 1.0f;
  if (player.y < 0.35f && ai.moveY > 0.0f) ai.moveY = 0.0f;

  bool wantFire = false;
  for (int i = 0; i < MAX_ENEMIES; ++i) {
    const Enemy &e = enemies[i];
    if (!e.active) continue;
    float dx = wrappedDelta(player.x, e.x);
    float dy = e.y - player.y;
    if (absf(dy) < 0.8f && absf(dx) < 9.0f) {
      bool inDirection = (dx > 0.0f && ai.moveX >= 0.0f) || (dx < 0.0f && ai.moveX <= 0.0f);
      if (inDirection || absf(dx) < 1.2f) {
        wantFire = true;
        break;
      }
    }
  }
  if (wantFire && aiFireCooldown == 0) {
    ai.fire = true;
    aiFireCooldown = 4;
  }

  if (threatScore >= 5 && smartBombs > 0 && aiSmartCooldown == 0) {
    ai.smart = true;
    aiSmartCooldown = 45;
  }
  if (!player.invulnerable && threatScore >= 7 && aiHyperCooldown == 0) {
    ai.hyper = true;
    aiHyperCooldown = 120;
  }

  return ai;
}

void startDemoMode(bool startFreshGame) {
  demoMode = true;
  demoGameOverAtMs = 0;
  resetAutoPlayerState();
  prevFire = false;
  prevSmart = false;
  prevHyper = false;
  if (startFreshGame || gameState != GameState::PLAYING) {
    newGame();
  }
}

void stopDemoModeToTitle() {
  demoMode = false;
  demoGameOverAtMs = 0;
  resetAutoPlayerState();
  gameState = GameState::TITLE;
  stateTimerMs = millis();
  prevFire = false;
  prevSmart = false;
  prevHyper = false;
}

void pollInputs() {
  pollSerialKeyboard();

  InputState s { 0.0f, 0.0f, false, false, false };
  applyAnalogJoystick(s);
  applyDpad(s);
  applySerialInput(s);
  applyPinActionButtons(s);
  applyKeyboardLibInput(s);

  s.moveX = clampUnit(s.moveX);
  s.moveY = clampUnit(s.moveY);
  userInputNow = s;
  userActionActiveNow = userInputNow.fire || userInputNow.smart || userInputNow.hyper;
  userInputActiveNow = isInputActive(userInputNow);

  if (demoMode) {
    inputNow = buildAutoPlayerInput();
  } else {
    inputNow = userInputNow;
  }
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
  float nx = inputNow.moveX;
  float ny = inputNow.moveY;

  player.x = wrapX(player.x + (nx * PLAYER_SPEED));
  player.y += (-ny * (PLAYER_SPEED * 0.72f));  // +ny means "up"

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

  if (inputNow.fire && !prevFire && player.fireCooldown == 0) {
    float vx = (player.facing > 0) ? BULLET_SPEED : -BULLET_SPEED;
    spawnBullet(true, player.x + (player.facing * 0.35f), player.y, vx, 0.0f, 26);
    player.fireCooldown = 8;
    toneEvent(2200, 12);
  }

  if (inputNow.smart && !prevSmart) {
    useSmartBomb();
  }
  if (inputNow.hyper && !prevHyper) {
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
    putDepthPixel(i, HUD_Y, DEPTH_HUD, SHADE_HUD_MAJOR);
  }
  // Smart bombs on the right.
  for (int i = 0; i < smartBombs && i < 3; ++i) {
    putDepthPixel(MATRIX_W - 1 - i, HUD_Y, DEPTH_HUD, SHADE_HUD_MAJOR - 1);
  }
  // Wave marker in center: binary pulse for wave modulo 4.
  uint8_t waveBits = wave & 0x03;
  putDepthPixel(RADAR_CENTER_X - 1, HUD_Y, DEPTH_HUD, (waveBits & 0x01) ? SHADE_HUD_MINOR : SHADE_BG_STAR);
  putDepthPixel(RADAR_CENTER_X, HUD_Y, DEPTH_HUD, (waveBits & 0x02) ? SHADE_HUD_MINOR : SHADE_BG_STAR);
}

void renderParallaxFar(float camX) {
  uint8_t twinklePhase = (uint8_t)((millis() / 180UL) & 0x03U);
  for (int sx = 0; sx < MATRIX_W; ++sx) {
    int sample = wrapInt((int)(camX * 0.18f) + (sx * 13), WORLD_W);
    uint8_t h = hash8((uint16_t)(sample * 17 + 23));
    if ((h & 0x07U) == 0U) {
      int y = PLAYFIELD_TOP + 1 + (h & 0x01U);
      if (((h >> 3) & 0x03U) != twinklePhase) {
        putDepthPixel(sx, y, DEPTH_BG_FAR, SHADE_BG_STAR);
      }
    }
  }
}

void renderParallaxMid(float camX) {
  for (int sx = 0; sx < MATRIX_W; ++sx) {
    float wx = wrapX((camX * 0.42f) + ((float)(sx - PLAYER_SCREEN_X) * 1.8f));
    int segment = wrapInt(((int)wx) / 4, WORLD_W / 4);
    uint8_t h = hash8((uint16_t)(segment * 29 + 11));
    int ridge = PLAYFIELD_TOP + 1 + (h & 0x01U);  // rows 2..3
    putDepthPixel(sx, ridge, DEPTH_BG_MID + ridge, SHADE_BG_MID);
    if (ridge + 1 < MATRIX_H) {
      putDepthPixel(sx, ridge + 1, DEPTH_BG_MID + ridge - 1, SHADE_BG_FAR);
    }
  }
}

void renderParallaxNear(float camX) {
  for (int sx = 0; sx < MATRIX_W; ++sx) {
    float wx = wrapX((camX * 0.68f) + ((float)(sx - PLAYER_SCREEN_X) * 1.2f));
    int cell = wrapInt(((int)wx) / 3, WORLD_W / 3);
    uint8_t h = hash8((uint16_t)(cell * 41 + 7));
    if ((h & 0x03U) == 0U) {
      int y = PLAYFIELD_TOP + 2 + (h & 0x01U);  // rows 3..4
      putDepthPixel(sx, y, DEPTH_BG_NEAR + y, SHADE_BG_NEAR);
    }
  }
}

void renderTerrain(float camX) {
  for (int sx = 0; sx < MATRIX_W; ++sx) {
    float wx = wrapX(camX + (sx - PLAYER_SCREEN_X));
    int gy = worldToScreenY(terrainAt(wx));
    if (gy < PLAYFIELD_TOP) gy = PLAYFIELD_TOP;
    if (gy >= MATRIX_H) continue;
    putDepthPixel(sx, gy, DEPTH_TERRAIN + gy + 2, SHADE_TERRAIN_TOP);
    if (gy + 1 < MATRIX_H) {
      putDepthPixel(sx, gy + 1, DEPTH_TERRAIN + gy + 1, SHADE_TERRAIN_MID);
    }
    for (int y = gy + 2; y < MATRIX_H; ++y) {
      putDepthPixel(sx, y, DEPTH_TERRAIN + y, SHADE_TERRAIN_FILL);
    }
  }
}

void renderHumans(float camX) {
  for (int i = 0; i < MAX_HUMANS; ++i) {
    Human &h = humans[i];
    if (!h.active || h.abducted) continue;
    int sx = worldToScreenX(h.x, camX);
    int sy = worldToScreenY(h.y);
    int16_t depth = DEPTH_HUMAN_BASE + sy;
    int rimX = (sx <= PLAYER_SCREEN_X) ? (sx + 1) : (sx - 1);
    putEdgePixel(rimX, sy, depth, SHADE_HUMAN_EDGE);
    if (h.falling) {
      putEdgePixel(sx, sy + 1, depth, SHADE_HUMAN_EDGE);
    }
    putDepthPixel(sx, sy, depth + 2, SHADE_HUMAN_CORE);
  }
}

void enemyShades(EnemyType type, uint8_t *core, uint8_t *edge, uint8_t *accent) {
  switch (type) {
    case EnemyType::LANDER:
      *core = 5; *edge = 3; *accent = 6;
      break;
    case EnemyType::MUTANT:
      *core = 6; *edge = 2; *accent = 7;
      break;
    case EnemyType::BOMBER:
      *core = 4; *edge = 2; *accent = 5;
      break;
    case EnemyType::POD:
      *core = 5; *edge = 2; *accent = 6;
      break;
    case EnemyType::SWARMER:
      *core = 6; *edge = 3; *accent = 7;
      break;
    case EnemyType::BAITER:
      *core = 7; *edge = 2; *accent = 6;
      break;
  }
}

void renderEnemies(float camX) {
  for (int i = 0; i < MAX_ENEMIES; ++i) {
    Enemy &e = enemies[i];
    if (!e.active) continue;
    int sx = worldToScreenX(e.x, camX);
    int sy = worldToScreenY(e.y);
    int16_t depth = DEPTH_ENEMY_BASE + sy;
    uint8_t core = 5;
    uint8_t edge = 3;
    uint8_t accent = 6;
    enemyShades(e.type, &core, &edge, &accent);

    int8_t dir = (e.vx >= 0.0f) ? 1 : -1;
    putEdgePixel(sx - dir, sy, depth, edge);
    putEdgePixel(sx, sy - 1, depth, edge);

    if (e.type == EnemyType::MUTANT || e.type == EnemyType::BAITER) {
      putEdgePixel(sx + dir, sy - 1, depth, accent);
      putDepthPixel(sx, sy - 1, depth + 1, accent);
    } else if (e.type == EnemyType::BOMBER || e.type == EnemyType::POD) {
      putEdgePixel(sx + dir, sy, depth, edge);
      putDepthPixel(sx + dir, sy, depth + 1, accent);
    } else if (e.type == EnemyType::SWARMER) {
      putEdgePixel(sx, sy + 1, depth, edge);
    }

    putDepthPixel(sx, sy, depth + 2, core);
  }
}

void renderBullets(float camX) {
  for (int i = 0; i < MAX_BULLETS; ++i) {
    if (!bullets[i].active) continue;
    int sx = worldToScreenX(bullets[i].x, camX);
    int sy = worldToScreenY(bullets[i].y);
    if (bullets[i].fromPlayer) {
      putEdgePixel(sx, sy, DEPTH_BULLET_PLAYER - 1, SHADE_PLAYER_EDGE);
      putDepthPixel(sx, sy, DEPTH_BULLET_PLAYER, SHADE_BULLET_PLAYER);
    } else {
      putEdgePixel(sx, sy, DEPTH_BULLET_ENEMY - 1, SHADE_HUMAN_EDGE);
      putDepthPixel(sx, sy, DEPTH_BULLET_ENEMY, SHADE_BULLET_ENEMY);
    }
  }
}

void renderPlayer() {
  if (player.invulnerable && ((millis() / 90UL) % 2UL == 0UL)) return;
  int sx = PLAYER_SCREEN_X;
  int sy = worldToScreenY(player.y);
  int16_t depth = DEPTH_PLAYER_BASE + sy;
  int8_t dir = (player.facing >= 0) ? 1 : -1;

  // Edge ring first, then bright core to keep front-object silhouettes readable.
  putEdgePixel(sx - dir, sy, depth, SHADE_PLAYER_EDGE);
  putEdgePixel(sx, sy - 1, depth, SHADE_PLAYER_ACCENT);
  putEdgePixel(sx - dir, sy - 1, depth, SHADE_PLAYER_ACCENT);
  putDepthPixel(sx + dir, sy, depth + 1, SHADE_PLAYER_EDGE);
  putDepthPixel(sx, sy, depth + 3, SHADE_PLAYER_CORE);
}

void renderRadar(float camX) {
  // Simple radar overlays on top row around the center.
  for (int i = 0; i < MAX_ENEMIES; ++i) {
    if (!enemies[i].active) continue;
    float d = wrappedDelta(camX, enemies[i].x);
    if (d < -36.0f || d > 36.0f) continue;
    int rx = RADAR_CENTER_X + (int)(d / 12.0f);
    if (rx < 3) rx = 3;
    if (rx > MATRIX_W - 4) rx = MATRIX_W - 4;
    putDepthPixel(rx, HUD_Y, DEPTH_HUD + 1, SHADE_RADAR);
  }
}

void renderTitle() {
  clearFrame();

  // Stylized "D" icon and ship motif with grayscale contouring.
  putDepthPixel(1, 1, DEPTH_UI, 6); putDepthPixel(1, 2, DEPTH_UI, 5); putDepthPixel(1, 3, DEPTH_UI, 5);
  putDepthPixel(1, 4, DEPTH_UI, 5); putDepthPixel(1, 5, DEPTH_UI, 6);
  putDepthPixel(2, 1, DEPTH_UI, 4); putDepthPixel(3, 1, DEPTH_UI, 6); putDepthPixel(4, 2, DEPTH_UI, 4);
  putDepthPixel(4, 3, DEPTH_UI, 3); putDepthPixel(4, 4, DEPTH_UI, 4); putDepthPixel(3, 5, DEPTH_UI, 6); putDepthPixel(2, 5, DEPTH_UI, 4);

  putDepthPixel(6, 2, DEPTH_UI, 4);
  putDepthPixel(7, 3, DEPTH_UI, 7);
  putDepthPixel(8, 2, DEPTH_UI, 5);

  if (((millis() / 220UL) % 2UL) == 0UL) {
    for (int x = 0; x < MATRIX_W; ++x) {
      uint8_t shade = (x & 0x01) ? 2 : 4;
      putDepthPixel(x, 7, DEPTH_UI, shade);
    }
  }

  renderFrame();
}

void renderGameOver() {
  clearFrame();
  // Explosion-like pattern with radial grayscale.
  putDepthPixel(5, 3, DEPTH_UI, 7); putDepthPixel(6, 3, DEPTH_UI, 7); putDepthPixel(5, 4, DEPTH_UI, 6); putDepthPixel(6, 4, DEPTH_UI, 6);
  putDepthPixel(4, 3, DEPTH_UI, 4); putDepthPixel(7, 3, DEPTH_UI, 4); putDepthPixel(5, 2, DEPTH_UI, 5); putDepthPixel(6, 2, DEPTH_UI, 5);
  putDepthPixel(5, 5, DEPTH_UI, 3); putDepthPixel(6, 5, DEPTH_UI, 3);
  if (((millis() / 200UL) % 2UL) == 0UL) {
    putDepthPixel(3, 1, DEPTH_UI, 2); putDepthPixel(8, 1, DEPTH_UI, 2);
    putDepthPixel(2, 6, DEPTH_UI, 2); putDepthPixel(9, 6, DEPTH_UI, 2);
  }
  renderFrame();
}

void renderWaveClear() {
  clearFrame();
  uint8_t bars = (uint8_t)min(MATRIX_W, 2 + (wave % 10));
  for (int x = 0; x < bars; ++x) {
    uint8_t shade = (uint8_t)(2 + (x % 5));
    for (int y = 2; y < 6; ++y) {
      putDepthPixel(x, y, DEPTH_UI, shade);
    }
  }
  renderFrame();
}

void renderGameplay() {
  clearFrame();
  float camX = player.x;
  renderParallaxFar(camX);
  renderParallaxMid(camX);
  renderParallaxNear(camX);
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
  matrix.setGrayscaleBits(MATRIX_GRAY_BITS);
  matrix.clear();

#if DEFENDER_ENABLE_PIN_ACTION_BUTTONS
  pinMode(PIN_FIRE, INPUT_PULLUP);
  pinMode(PIN_SMART, INPUT_PULLUP);
  pinMode(PIN_HYPER, INPUT_PULLUP);
#endif
#if DEFENDER_ENABLE_ANALOG_JOYSTICK
  pinMode(PIN_JOY_BTN, INPUT_PULLUP);
#endif
#if DEFENDER_ENABLE_DPAD
  pinMode(PIN_DPAD_LEFT, INPUT_PULLUP);
  pinMode(PIN_DPAD_RIGHT, INPUT_PULLUP);
  pinMode(PIN_DPAD_UP, INPUT_PULLUP);
  pinMode(PIN_DPAD_DOWN, INPUT_PULLUP);
#endif
#if DEFENDER_ENABLE_SERIAL_KEYBOARD
  Serial.begin(115200);
#endif
#if DEFENDER_HAVE_KEYBOARD_LIB
  Keyboard.begin();
#endif
#if DEFENDER_ENABLE_AUDIO
  pinMode(PIN_BUZZER, OUTPUT);
#endif

  randomSeed(analogRead(A5));
  resetArrays();

  stateTimerMs = millis();
  lastFrameMs = millis();
  lastUserInputMs = millis();
}

void loop() {
  uint32_t now = millis();
  if (now - lastFrameMs < FRAME_MS) return;
  lastFrameMs = now;

  pollInputs();

  if (demoMode && userInputActiveNow) {
    lastUserInputMs = now;
    stopDemoModeToTitle();
    inputNow = userInputNow;
  } else if (!demoMode) {
    // In title/game-over states we only treat action buttons as real user input
    // to avoid analog noise suppressing attract/demo behavior.
    if (userActionActiveNow || (gameState == GameState::PLAYING && userInputActiveNow)) {
      lastUserInputMs = now;
    }
    if (now - lastUserInputMs >= DEMO_IDLE_TIMEOUT_MS) {
      startDemoMode(true);
      lastUserInputMs = now;
    }
  }

  if (demoMode && gameState == GameState::GAME_OVER) {
    if (demoGameOverAtMs == 0) {
      demoGameOverAtMs = now;
    } else if (now - demoGameOverAtMs >= DEMO_RESTART_DELAY_MS) {
      demoGameOverAtMs = 0;
      newGame();
    }
  } else {
    demoGameOverAtMs = 0;
  }

  switch (gameState) {
    case GameState::TITLE:
      renderTitle();
      if (inputNow.fire && !prevFire) {
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
      if (!demoMode && inputNow.fire && !prevFire && now - stateTimerMs > 500UL) {
        gameState = GameState::TITLE;
      }
      break;
  }

  prevFire = inputNow.fire;
  prevSmart = inputNow.smart;
  prevHyper = inputNow.hyper;
}
