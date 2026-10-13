# Playing

<img src="home.png" alt="The pet screen" width="180" align="right">

## The loop

1. **Egg.** Press the blue button, or shake the stick, seven times to crack
   it. A pet hatches: a different animal each time, unless the parent screen
   names one.
2. **Pet screen.** The pet sits at the top. Under it are two rows, apples for
   hunger and hearts for happiness, and three icons.
3. **Feed** (apple). A tray of four foods opens: apple, carrot, strawberry
   and ice cream. Every food fills one apple, so there is no better or worse
   choice. One of them is the pet's favourite, picked fresh each time the
   stick wakes, and gets a bigger celebration. The tray opens on the last
   food picked.
4. **Dance party** (party hat). Dance, jump or wiggle with the stick. Any
   movement fills five rainbow bars while the pet parties.
5. **Bed** (moon). The pet goes to sleep and the stick turns itself off.
6. **Toy box** (hold the blue button). Two pages of icon tiles with more
   things to do. See [Toy box](toy-box.md).

Apples drop one every 8 minutes the stick is awake, hearts one every 12. Naps
do not count, and the minutes either side of one add up, so a day of short
sessions wears the pet down as fast as one long one. Switching on after a
power-off costs one of each, so there is always something to do, and starts
both counts again. When a row runs out the pet asks for attention, and that
is the worst that happens. A pet left in a drawer comes back as it was.

## The buttons lesson

A stick that is new, or has just been reset from the parent screen, opens on
a one-minute lesson before the egg. It uses the two pictures from Copycat: a
blue circle for the blue button and a white bar at the edge for the side
button. Whichever one to press pulses, and every press lights its picture.

1. **Blue picks.** Press blue and the thing in the box pops.
2. **Side moves along.** Each press moves the frame over four tiles: right,
   then down to the next row, then back to the start.
3. **Both.** Move the frame to the star with the side button, then pick it
   with blue.
4. **Hold blue.** A ring fills round the circle while blue is held. A full
   ring opens the box into four tiles; another shuts it. A quick tap only
   lights the circle.

The screen does each step once, then waits for it to be copied a few times.
There is no timer and no wrong answer. It ends in a burst of stars, then the
egg. A new egg from the toy box does not play it again. To skip it, open the
parent screen and choose `back`.

## Controls

| Input | On the pet screen | In the toy box or an activity |
| --- | --- | --- |
| **Side button** | Next icon | Next tile, or the activity's second action |
| **Blue front button** | Select | Select, or the activity's main action |
| **Hold blue** (0.7s) | Open the toy box | Back to the pet, from anywhere |
| **Shake** | Pet gets dizzy | Depends on the activity |
| **Face down** | Puts the pet to bed | |
| **Hold side** (1.5s) | Sound on or off; a speaker shows which | Same |
| **Hold both** (1.5s) | Parent screen | Same |
| **Power button** | Turns the stick back on after it has powered off | |

Buttons act on release, so a long press never also counts as a short one.
The toy box and the food tray go back to the pet by themselves after 15
seconds without input.

## Parent screen

A text menu for grown-ups, and the only screen with words on it. Side button
moves, blue changes.

| Line | What it does |
| --- | --- |
| `bright` | Screen brightness, 0 to 4. Starts at 1. |
| `sound` | On or off. |
| `stay on` | Never auto power-off. For battery testing. |
| `egg tile` | Show or hide New egg in the toy box. |
| `pet` | What an egg hatches. On `random`, the default, it is any of the animals. Naming one makes every egg hatch that animal, and changes the current pet to it straight away. |
| `tilt cal` | Teach the stick how it is held for the tilt games. See below. |
| `lesson` | Plays the buttons lesson now. Nothing is reset, and it ends back at the pet. |
| `reset` | Press twice. Back to an egg with everything as it started: stickers, dress-up, hearts, the settings above and the awake timer. The buttons lesson plays again before the egg. The tilt calibration is the one thing kept. |

It also shows the battery voltage and how long the stick has been awake since
the last reset.

### Tilt calibration

Tilt activities (catch, draw, marble) take half a second at the start to
learn how the stick is being held, so "level" is wherever the kid is holding
it, upright included.

Which way counts as left, right, up and down, and how far a tilt goes, comes
from a calibration measured on one StickS3 until the stick is given its own.
Choose `tilt cal` and follow the five pictures, pressing blue on each: hold
the stick the way you play, then tilt it as if rolling a ball left, right, up
and down the screen. The measured directions and reach are saved and used by
every tilt activity. The readings stay on screen at the end and are printed
to the serial log. To go back to the built-in setting, open `tilt cal` and
press the side button on the first picture.
