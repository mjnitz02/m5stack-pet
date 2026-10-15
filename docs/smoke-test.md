# Smoke test on a real stick

`make test` covers the game's rules on a computer. This is the short list for
what only the hardware can show. Run it after a change to power handling,
sound, movement or drawing, and before handing a stick to a child.

Flash with `make flash`, unplug USB, and work down the list.

## Power

- [ ] Leave it on the pet screen. It dims and sleeps after about a minute,
      and the screen goes fully dark about 15 seconds later.
- [ ] The green light by the power button is off while it naps.
- [ ] The blue button wakes it at once, on the pet screen, and that press
      does not open the food tray.
- [ ] Put it face down on a table. It goes to bed within a second or two.
- [ ] After a nap that has run its full length (12 hours), the blue button no
      longer wakes it and the side power button does.
- [ ] Plugged in to USB, a nap still wakes on the blue button.
- [ ] Note the awake time on the parent screen, then let it nap and wake it
      eight times without touching it otherwise. The awake time has gone up
      by 10 minutes or a little over, not 8.
- [ ] Start the drawing game and put the stick down. After 20 seconds it is
      back on the pet, and a minute later it sleeps as usual.
- [ ] Start a walk and keep the stick moving without pressing anything for
      half a minute. It stays on the walk.
- [ ] Hold both buttons for the parent screen and leave it. After a minute
      it is back on the pet, and it then sleeps as usual.
- [ ] On USB, `make monitor` prints a battery line every ten seconds.

## Sound

- [ ] Button beeps and the feeding tune are audible and not harsh.
- [ ] Hold the side button: the speaker glyph shows crossed out, and the
      stick is silent. Hold again: a chirp, and sound is back.
- [ ] Do the same in hide and seek: the glyph goes away after a moment.
- [ ] No hiss or pop from the speaker while it naps or as it wakes.

## Movement

- [ ] Walk: ordinary walking moves the walker along; standing still stops it.
- [ ] Bath and dance fill with shaking and dancing a child would do.
- [ ] Rock: gentle rocking moves the ball; a stick lying still does not.
- [ ] Freeze: holding still for the open eye earns a star; moving makes the
      pet dizzy.
- [ ] Shake the egg: it wobbles and cracks.

## Tilt

- [ ] Catch, draw and marble: held like a phone, the basket, pen and ball
      go the way the stick is tipped, left, right, up and down.
- [ ] Parent screen, `tilt cal`: the five poses read and it says saved.

## Drawing

- [ ] Each animal looks right on the pet screen, with a hat and a colour on.
- [ ] No trails or leftover marks on the toy box, the food tray or in any
      activity.
- [ ] The battery dashes along the top edge are there and steady.

## A full day

- [ ] From a full charge, note the parent screen's battery voltage and awake
      time morning and evening.
