# Battery

The stick has a small battery, so the firmware is built around not wasting
it.

- **No radio.** Bluetooth and Wi-Fi are not compiled in, so they cannot be
  on.
- **It puts itself to bed.** After 60 seconds without input the pet falls
  asleep on a dimmed screen. 15 seconds later the screen goes off and the
  stick naps. For the next 60 minutes the blue button wakes it; after that it
  powers off and the side power button turns it back on.
- **It leaves a game nobody is playing.** An activity with no button
  pressed and the stick not moving for 20 seconds goes back to the pet, and
  the toy box and the food tray do the same after 15. The pet screen is the
  one that goes to bed, so a stick set down anywhere ends up asleep.
- **Slow CPU.** 80 MHz instead of 240. The animations run at 5 frames a
  second, so it is plenty.
- **Low brightness.** 1 of 4 by default, changed on the parent screen.

The StickS3 screen is an LCD, not an OLED. Black pixels do not save power on
it, because the backlight is lit either way. Only the brightness level and
the time the screen is on affect the battery.

Five dim dashes along the top edge of the screen show the charge, one per
20%. The last one turns orange.

On USB the stick cannot cut its own power, so "off" is a deep sleep that the
blue button wakes.

## Measuring screen-on time

1. Charge it fully.
2. Hold both buttons for the parent screen and choose `reset`.
3. Open the parent screen again and turn `stay on` on.
4. Leave it on the pet screen until it dies.
5. Charge it again. The parent screen shows the awake time.

The time is saved every 5 minutes, so it reads up to 5 minutes low. Repeat at
another brightness to see what the backlight costs. With `stay on` off, the
same counter shows real-use time across days.

## The on-device log

A debug build (`make flash-debug`) records a row every ten seconds, and on
every screen change, to the stick's own flash: battery voltage, backlight,
how hard it is being moved, button presses and frame rate. Play with it off
the cable, plug it back in, and `make log` writes the rows to `logs/` as CSV.
`make log-clear` empties it. The columns are described in `src/petlog.h`.
`make flash` goes back to a build with none of it.

The debug build also naps for 12 hours instead of 60 minutes, so one night
off the cable shows how much the nap itself drains: compare `bat_mv` on the
"nap" row with the next "boot:" row.

The power chip has no current sensor, so drain has to be read off how fast
the voltage falls. While USB is connected the battery is charging and its
voltage says nothing about drain.
