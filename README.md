# Uno Q Defender (12x8 LED Matrix Edition)

This is a fully playable Defender-style game adapted for the Uno onboard LED matrix.

Because the matrix is only 12x8, this implementation preserves the core Defender loop and systems while compressing visuals:
- Horizontal wraparound world
- Terrain and human protection objective
- Enemy set: Landers, Mutants, Bombers, Pods, Swarmers, Baiters
- Abduction flow (lander picks up human, can mutate if successful)
- Shooting, enemy fire, collisions, lives
- Smart bomb and hyperspace
- Waves and wave-clear bonus

## File
- Sketch: [Uno-Q-Defender.ino](/Volumes/trent/dev/Uno-Q-Defender/Uno-Q-Defender.ino)

## Hardware Assumptions
- Board with `Arduino_LED_Matrix` support and onboard 12x8 matrix
- Analog joystick for movement:
  - `A0`: horizontal
  - `A1`: vertical
- Buttons (active LOW with `INPUT_PULLUP`):
  - `D2`: fire
  - `D3`: smart bomb
  - `D4`: hyperspace
- Optional buzzer:
  - `D9`: piezo buzzer positive lead (negative to GND)

All pin mappings are constants at the top of the sketch and can be changed quickly.

## Controls
- Joystick X/Y: move ship
- Fire button: fire in facing direction
- Smart bomb: destroy all active enemies and enemy shots
- Hyperspace: random teleport with failure risk
- Fire on title screen: start game
- Fire on game-over screen: return to title

## Build / Upload
Use Arduino IDE:
1. Open [Uno-Q-Defender.ino](/Volumes/trent/dev/Uno-Q-Defender/Uno-Q-Defender.ino)
2. Select your Uno board/port
3. Verify and upload

## Gameplay Notes
- Top row is a compressed HUD/radar layer:
  - Left pixels: lives
  - Right pixels: smart bombs
  - Center pixels: wave markers / radar blips
- Humans are tiny single-pixel entities due display constraints.
- The ship is left-biased to preserve Defender’s forward flight feel.

## Tuning Knobs
If you want more/less difficulty, edit constants in the sketch:
- `PLAYER_SPEED`
- `BULLET_SPEED`
- `ENEMY_BULLET_SPEED`
- `FRAME_MS`
- Enemy spawn formulas in `initWave()`
- Baiter timer in `maybeSpawnBaiter()`

## Known Limits vs Original Arcade
- No full bitmap sprites (single-/multi-pixel signatures only)
- No separate scanner/radar panel; merged into HUD row
- Audio is short event tones, not sampled arcade effects
- Text screens are iconized due matrix size
