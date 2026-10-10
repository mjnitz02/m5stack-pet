# m5stack-pet

A small offline virtual pet for the M5StickS3. An egg hatches into an ASCII
animal that you feed, play a tilt game with, and put to bed. There is no
Bluetooth, no Wi-Fi and no app. It is meant for young kids, so nothing on the
main screens needs reading and the pet can't die.

This started as a fork of
[claude-desktop-buddy](https://github.com/anthropics/claude-desktop-buddy)
with the Claude and Bluetooth parts taken out. The ASCII animals and their
animations come from that project.

> **Status: early.** The firmware builds and the pieces below are written,
> but it is still being tested on real hardware. Expect timings and details
> to change.

<p align="center">
  <img src="docs/species/_all.png" alt="The ASCII animals in their idle pose" width="720">
</p>

## How it plays

1. **Egg.** Press the blue button (or shake the stick) a few times to crack
   it. A pet hatches. For now every egg is an octopus.
2. **Home.** The pet sits at the top. Under it are two rows, apples for
   hunger and hearts for happiness, and three icons.
3. **Feed** (apple). An apple drops, the pet is happy, one apple fills in.
4. **Play** (ball). Five apples fall one at a time. Tilt the stick left and
   right to catch them in the basket. Every round ends in a celebration.
5. **Bed** (moon). The pet goes to sleep and the stick turns itself off.

Apples and hearts drain slowly while the stick is on. When one runs out the
pet asks for attention, and that is the worst that happens. Time does not
pass while the stick is off, so a pet left in a drawer comes back as it was.

## Controls

| Input | What it does |
| --- | --- |
| **Side button** | Next icon |
| **Blue front button** | Select |
| **Shake** | Pet gets dizzy |
| **Face down** | Puts the pet to bed |
| **Hold side button** (1.5s) | Sound on or off |
| **Hold both buttons** (1.5s) | Parent screen |
| **Power button** | Turn the stick back on |

### Parent screen

Text menu for grown-ups: brightness, sound, `stay on` (no auto power-off),
`pet` (switch animal), `zero timer`, and `new egg`. It also shows battery
voltage and how long the stick has been awake since the timer was zeroed.

## Battery

The stick has a small battery, so the firmware is built around not wasting
it:

- No radio code is compiled in.
- After 60 seconds without input the pet falls asleep, and 15 seconds later
  the stick powers off. The power button turns it back on.
- The CPU runs at 80 MHz and the screen brightness defaults to low.

To measure screen-on time: charge it, open the parent screen, choose
`zero timer`, turn `stay on` on, and leave it until it dies. After charging
again the parent screen shows the awake time (saved every 5 minutes).

## Hardware

Built for the **M5StickS3** (ESP32-S3). The older **M5StickC Plus** also
compiles from the same code through
[M5Unified](https://github.com/m5stack/M5Unified), but is not the board this
is tested on.

<p align="center">
  <img src="docs/sticks3.png" alt="M5Stack M5StickS3" width="320">
</p>

## Building and flashing

You need [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/)
and, for the Python tools, [uv](https://docs.astral.sh/uv/).

```bash
make build     # compile for the M5StickS3
make flash     # compile and upload over USB
make monitor   # serial log: battery mV and awake minutes
make test      # checks on the animal art
make lint      # ruff on tools/, then a build with warnings as errors
make species   # re-render docs/species/*.png
```

Use `PIO_ENV=m5stickc-plus` with any of these for the older board.

**M5StickS3 download mode:** if the stick is running other firmware and the
upload can't connect, plug in USB and hold the side power button until the
inner green LED blinks, then run `make flash` again.

## Animals

Eleven animals, each with seven animations: sleep, idle, busy, attention,
celebrate, dizzy and heart. Each one is a file in `src/buddies/` and is
listed in `SPECIES_TABLE` in `src/buddy.cpp`.

`docs/species/` has a picture of every animal in every state. To remove one,
delete its file and its two lines in `src/buddy.cpp`, then run
`make test species`.

## Project layout

```
src/
  main.cpp   - game loop, screens, power handling
  pet.h      - saved state (needs, settings, awake timer)
  buddy.cpp  - animal registry and ASCII render helpers
  buddies/   - one file per animal, seven animation functions each
  compat.h   - maps the old display type names onto M5Unified
tools/
  species.py         - parses the animal art out of src/buddies/
  render_species.py  - draws docs/species/*.png
  test_species.py    - art checks
```

## Credits and license

MIT, see [LICENSE](LICENSE). The animal art, the renderer and the original
firmware are from
[claude-desktop-buddy](https://github.com/anthropics/claude-desktop-buddy)
by Anthropic.
