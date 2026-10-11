# Development

## Hardware

Built for the **M5StickS3** (ESP32-S3, 135 x 240 LCD, two buttons, an
accelerometer and a small speaker). The older **M5StickC Plus** also compiles
from the same code through [M5Unified](https://github.com/m5stack/M5Unified),
but is not the board this is tested on.

<img src="sticks3.png" alt="M5Stack M5StickS3" width="280">

## Make targets

You need [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/),
and for the Python tools [uv](https://docs.astral.sh/uv/).

| Target | Runs |
| --- | --- |
| `make build` | Compile for the M5StickS3 |
| `make flash` | Compile and upload over USB |
| `make monitor` | Serial log: battery mV and awake minutes every 10 seconds |
| `make flash-debug` | Flash with the on-device log compiled in |
| `make log` | Copy that log to `logs/` (stick awake and on USB) |
| `make log-clear` | Empty it |
| `make test` | Checks on the animal art |
| `make lint` | ruff on `tools/`, then a build with warnings as errors |
| `make species` | Redraw `docs/species/*.png` and `docs/home.png` |
| `make install-hooks` | Pre-commit hook: ruff, art checks, secret scan |

Add `PIO_ENV=m5stickc-plus` to any of the build targets for the older board.

**Download mode:** if the stick is running other firmware and the upload
can't connect, plug in USB and hold the side power button until the inner
green LED blinks, then run `make flash` again.

There are no tests of the game logic. It lives next to the hardware calls,
and splitting it out is worth doing once the rules settle.

## Layout

```
src/
  main.cpp       - game loop, core screens, toy box, power handling
  activities.h   - the toy box activities
  icons.h        - every icon, drawn from shapes
  pet.h          - saved state (needs, settings, stickers, dress-up)
  petlog.h       - the debug build's on-device log
  buddy.cpp      - animal registry and ASCII render helpers
  buddies/       - one file per animal, seven animation functions each
  compat.h       - maps the old display type names onto M5Unified
tools/
  species.py         - parses the animal art out of src/buddies/
  render_species.py  - draws docs/species/*.png and docs/home.png
  glcdfont.py        - the stick's font, for those drawings
  test_species.py    - art checks
  pull_log.py        - reads the on-device log over USB
```

## Tuning

The numbers that decide how the toy feels sit at the top of their sections
as named constants. The movement ones are guesses until they have been tried
in small hands.

| What | Where | Constants |
| --- | --- | --- |
| Time to bed, nap and power-off | `main.cpp` | `IDLE_BED_MS`, `BED_OFF_MS`, `NAP_MS` |
| How fast apples and hearts drop | `main.cpp` | `HUNGER_DECAY_MS`, `HAPPY_DECAY_MS` |
| Taps to hatch | `main.cpp` | `EGG_TAPS` |
| How hard a shake is | `main.cpp` | `SHAKE_DELTA` |
| Sticker odds | `main.cpp` | `STICKER_ODDS` |
| Catch speed and basket width | `main.cpp` | `FALL_PX_MS`, `CATCH_HALF` |
| Sound level | `main.cpp`, `setup()` | `M5.Speaker.setVolume(90)` |
| How much moving each activity needs | `activities.h` | `WALK_PER_DOT`, `DANCE_PER_BAR`, `BATH_PER_BUBBLE`, `ROCK_PER_CROSS` |
| Freeze: how still, for how long | `activities.h` | `FREEZE_MOVE`, `FREEZE_GRACE_MS`, `FREEZE_HOLD_MS` |
| Pen and marble feel | `activities.h` | `PEN_SPEED`, `MARBLE_ACCEL`, `MARBLE_DRAG` |

If the side button is on the left of your board, Copycat's white bar should
be too: set `SIDE_BAR_X` to 6.
