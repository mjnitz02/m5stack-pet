# Toy box

Hold the blue button on the pet screen to open it, and hold it again from
anywhere to go back to the pet.

## The rules every activity keeps

- **No words.** Kid-facing screens are icons and ASCII only.
- **Nothing can be failed or lost.** No score, no timer that runs out, no
  losing state. A miss repeats the round.
- **Moving comes first.** The best of these get a kid up: walking the pet
  around, shaking it clean, rocking it to sleep, freezing like a statue. The
  screen stays quiet and slow so the activity is in the room, not on the
  display.
- **Close enough counts.** Movement is measured coarsely on purpose. If the
  kid is doing something roughly like the activity, the screen plays out as
  if they were doing it properly. Walk is not a step counter and Rock to
  sleep is not a sway detector.
- **Quiet.** Sounds are short and obey mute.
- **Shapes only.** Every icon is drawn from circles, rectangles, lines and
  triangles in `src/icons.h`. There are no bitmaps.

Finishing an activity adds a heart to the happiness row. One time in three it
also awards a sticker the kid does not have yet, shown full screen with a
short jingle.

## The grid

Two pages of six tiles, 58 by 66 pixels each. The side button walks through
them in reading order and rolls from the last tile of one page onto the
first of the next, so there is no separate "next page" action to learn. Two
dots at the bottom show which page is up, and the selected tile gets the same
double white outline as the icons on the pet screen.

It always opens on Walk. The first page is care and moving; the second is the
sit-still games. New egg is the very last tile, as far from the opening
position as it can be.

| Page 1 | | Page 2 | |
| --- | --- | --- | --- |
| Walk | Bath | Catch | Draw |
| Rock to sleep | Freeze | Marble | Copycat |
| Dress-up | Stickers | Hide and seek | New egg |

## Moving

Dance party lives on the pet screen's middle icon, not in the grid, but
belongs with these.

| Activity | What you do | Ends |
| --- | --- | --- |
| **Dance party** | Dance, jump or wiggle with the stick. Any movement fills five rainbow bars while the pet parties. | When the fifth bar is full, after about half a minute of dancing. |
| **Walk** | Take the pet somewhere. Walking, running, jumping and spinning all count: the stick adds up how much it has been moved and lights the next dot on a path. Standing still just pauses. | At the flower, ten dots and about a minute of real moving. |
| **Bath** | Shake the bubbles off. The next bubble swells as you shake, then pops. | When the eighth bubble goes. |
| **Rock to sleep** | Rock the stick gently, any way that feels like rocking. A ball sways across its track and back, and each crossing fills a moon. A hard shake is no faster than a gentle rock. | After six moons the pet goes to bed and the stick powers off as usual. |
| **Freeze** | Wiggle while the pet's eyes are shut. When the big eye opens, hold still for a star. Moving makes the pet dizzy and the round comes again. | After five stars. |

## Tilting

| Activity | What you do | Ends |
| --- | --- | --- |
| **Catch** | Five snacks fall one at a time. Tilt left and right to slide the basket under them. | After the fifth. Three or more catches is worth two hearts. |
| **Draw** | An Etch A Sketch. Tilt moves the pen and the line drifts through the rainbow as it goes. Blue starts a fresh page from the middle; a shake wipes the page. | Never. Hold blue to leave. |
| **Marble** | Roll a ball, with a little momentum, onto three apples one at a time. Blue starts it over. | After the third apple. |

Nobody holds the stick level, so every tilt activity spends its first half
second averaging how the stick is being held and steers from there. The stick
is held like a phone, looking down at it with the top tipped up, and a tilt
never needs the screen turned out of view. See
[tilt calibration](playing.md#tilt-calibration).

## Buttons only

| Activity | What you do | Ends |
| --- | --- | --- |
| **Copycat** | The pet lights the two buttons in a pattern: a blue circle for the blue button, a white bar at the edge for the side button. Repeat it. A miss replays the same pattern. | After four patterns. |
| **Hide and seek** | The pet's eyes peek over one of three bushes, then duck. Side moves, blue looks. A wrong bush rustles and the eyes peek again. | After three finds. |

## Keeping

**Dress-up.** Side changes the colour, blue changes the hat. Colours are the
animal's own, aqua, pink, yellow, green and amber. Hats are none, bow, crown,
party hat and flower. The choice is saved as it changes and follows the pet
through every animation and activity.

**Stickers.** A page of twelve slots; empty ones show as outlines. There is
nothing to press. The twelve are a star, heart, apple, moon, flower, ball,
crown, carrot, strawberry, bubbles, ice cream and bow.

## New egg

A tap on the egg tile only makes it wobble. Holding blue on it fills a ring
around the egg over three seconds with rising beeps; letting go early
cancels. When the ring closes the screen flashes and the stick is back at an
unhatched egg.

It clears the pet, both need rows, the stickers, the colour, the hat and the
remembered food. It keeps sound, brightness, `stay on` and the awake timer.

Hold-blue means "back to the pet" everywhere else, so this is the one tile
where it means something different. Leaving from here is one side press to
another tile, or the 15-second timeout.

A three-second hold is easy for a curious five-year-old to discover. The
parent screen's `egg tile` line hides the tile once testing is over.

## What is saved

| Value | Size | Used for |
| --- | --- | --- |
| `food` | 1 byte | Last food picked, so the tray opens on it |
| `stickers` | 2 bytes | One bit per sticker |
| `color` | 1 byte | Dress-up colour |
| `hat` | 1 byte | Dress-up hat |
| `eggtile` | 1 bit | Whether the New egg tile is shown |

Saves happen on events only: a food picked, a sticker earned, dress-up
changed, an activity finished. Nothing writes on a timer, because flash wears
out.

## Ideas not built

- **A nudge toward moving.** The moving activities could give two hearts
  where the sit-still games give one. Today only a good round of Catch does.
- **Sticker pace.** One in three finishes means roughly 35 to 40 plays to
  fill the page. Faster makes it a one-afternoon thing; slower may be too
  long for a four-year-old to notice progress.
- **A music box** is fun but is the noisy-toy case.
- **Microphone, or infrared between two sticks** so the pets can visit.
  Needs a check of what the StickS3 has before it is worth sketching.
