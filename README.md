# Uno Q Defender (13x8 LED Matrix Edition)

[![Arduino-lint](https://flat.badgen.net/badge/Arduino-lint/passing/2da44e?labelColor=24292f)](https://github.com/ripred/Uno-Q-Defender/actions/workflows/arduino-lint.yml)
[![License](https://flat.badgen.net/badge/License/MIT/0969da?labelColor=24292f)](https://github.com/ripred/Uno-Q-Defender/blob/main/LICENSE)
[![Stars](https://flat.badgen.net/badge/Stars/0/bf8700?labelColor=24292f)](https://github.com/ripred/Uno-Q-Defender/stargazers)
[![Forks](https://flat.badgen.net/badge/Forks/0/6f42c1?labelColor=24292f)](https://github.com/ripred/Uno-Q-Defender/network/members)

This is a fully playable Defender-style game adapted for the Uno onboard LED matrix.

Because the matrix is only 13x8, this implementation preserves the core Defender loop and systems while compressing visuals:
- Horizontal wraparound world
- Terrain and human protection objective
- Enemy set: Landers, Mutants, Bombers, Pods, Swarmers, Baiters
- Abduction flow (lander picks up human, can mutate if successful)
- Shooting, enemy fire, collisions, lives
- Smart bomb and hyperspace
- Waves and wave-clear bonus

## File
- Sketch: [Uno-Q-Defender.ino](/Volumes/trent/dev/Uno-Q-Defender/Uno-Q-Defender.ino)

## Input Support
The game now supports multiple input backends at once (hybrid mode):
- Analog joystick
- Digital D-pad buttons
- Action buttons
- Serial keyboard commands (from terminal/serial monitor)
- Optional `Keyboard.h` hook
- Optional `Joystick.h` hook
- 8 grayscale levels per LED (`setGrayscaleBits(3)`)

Default firmware profile is board-safe for UNO Q onboard matrix:
- external GPIO controls disabled by default (`ANALOG_JOYSTICK`, `DPAD`, `PIN_ACTION_BUTTONS`)
- audio disabled by default
- serial keyboard + demo/self-play enabled

### Default Pins
- Analog joystick:
  - `A0`: X axis
  - `A1`: Y axis
  - `D5`: joystick button (mapped to Fire)
- Action buttons (active LOW with `INPUT_PULLUP`):
  - `D2`: Fire
  - `D3`: Smart Bomb
  - `D4`: Hyperspace
- D-pad buttons (active LOW with `INPUT_PULLUP`):
  - `D6`: Left
  - `D7`: Right
  - `D8`: Up
  - `D10`: Down
- Audio:
  - `D9`: piezo buzzer positive lead (negative to GND)

All pin mappings are constants near the top of the sketch.

### Serial Keyboard Controls (`115200`)
- Movement:
  - `W A S D`
  - Arrow keys (`ESC [ A/B/C/D`) from terminals that send ANSI escapes
  - Diagonal shortcuts: `Q E Z C`
  - Stop: `X` or `0`
- Actions:
  - Fire: `Space`, `F`, or `J`
  - Smart Bomb: `B` or `K`
  - Hyperspace: `H` or `L`

### Demo / Attract Mode
- An autonomous self-play pilot is built in.
- If no user input is detected for 60 seconds, demo mode starts automatically and plays the game.
- Demo mode stops when:
  - a user input is detected, or
  - the demo run reaches game over.
- After demo game over, the normal game-over screen is shown briefly, then demo auto-restarts.
- Any user input during demo immediately returns to title (and `Fire` can start a normal game right away).

### Optional `Keyboard.h` / `Joystick.h` Integration
Optional compile-time toggles:
- `DEFENDER_ENABLE_KEYBOARD_LIB`
- `DEFENDER_ENABLE_JOYSTICK_LIB`

If the header is available, the sketch exposes weak hooks you can override:
- `bool defenderKeyboardPressed(uint8_t keycode)`
- `void defenderJoystickSample(float* outX, float* outY, bool* outFire, bool* outSmart, bool* outHyper)`

This avoids hard-coding one specific third-party API flavor of `Joystick.h`.

## Build / Upload (CLI)
Default build:
```bash
arduino-cli compile --fqbn arduino:zephyr:unoq /Volumes/trent/dev/Uno-Q-Defender
```

Enable optional library hooks:
```bash
arduino-cli compile --fqbn arduino:zephyr:unoq \
  --build-property build.extra_flags="-DDEFENDER_ENABLE_KEYBOARD_LIB=1 -DDEFENDER_ENABLE_JOYSTICK_LIB=1" \
  /Volumes/trent/dev/Uno-Q-Defender
```

Enable external wired controls/audio (if your wiring is known-safe for your board):
```bash
arduino-cli compile --fqbn arduino:zephyr:unoq \
  --build-property build.extra_flags="-DDEFENDER_ENABLE_ANALOG_JOYSTICK=1 -DDEFENDER_ENABLE_DPAD=1 -DDEFENDER_ENABLE_PIN_ACTION_BUTTONS=1 -DDEFENDER_ENABLE_AUDIO=1" \
  /Volumes/trent/dev/Uno-Q-Defender
```

Upload:
```bash
arduino-cli upload --fqbn arduino:zephyr:unoq -p <serial-port> /Volumes/trent/dev/Uno-Q-Defender
```

## Build / Upload (Arduino IDE)
For Uno Q, use the Arduino board package that provides the `arduino:zephyr` core.

Notes on IDE version:
- Arduino CLI + current board package is validated in this repo.
- Arduino IDE 1.8.19 is legacy and may not fully support newer Uno Q Zephyr tooling/workflows.
- Recommended path for Uno Q is Arduino CLI or Arduino IDE 2.x.

## Gameplay Notes
- Rendering uses all 8 gray levels with a depth-aware compositor (`z`-style layering).
- Foreground sprites apply contrast edges when drawn over existing pixels so overlaps stay legible.
- Gameplay view has 3 parallax layers (far stars, mid ridges, near haze) plus foreground terrain.
- Top row is a compressed HUD/radar layer:
  - Left pixels: lives
  - Right pixels: smart bombs
  - Center pixels: wave markers / radar blips
- Humans and enemies use tiny shaded signatures tuned for overlap readability on 13x8.
- The ship is left-biased to preserve Defender’s forward flight feel.

## Tuning Knobs
If you want more/less difficulty, edit constants in the sketch:
- `PLAYER_SPEED`
- `BULLET_SPEED`
- `ENEMY_BULLET_SPEED`
- `FRAME_MS`
- `JOYSTICK_Y_UP_SIGN`
- Enemy spawn formulas in `initWave()`
- Baiter timer in `maybeSpawnBaiter()`

## Known Limits vs Original Arcade
- No full bitmap sprites (single-/multi-pixel signatures only)
- No separate scanner/radar panel; merged into HUD row
- Audio is short event tones, not sampled arcade effects
- Text screens are iconized due matrix size
