# CRUZKANOID DOS CLONE

A classic brick-breaking game written in C for DOS.

![Cruzkanoid running in DOSBox-X](cruzkanoid_on_dosbox.png)

## Features
- 320x200 VGA graphics (Mode 13h)
- 5 rows x 10 columns of colorful bricks
- Paddle controls with arrow keys
- Ball physics with paddle spin
- Score tracking
- 3 lives
- Win/lose conditions

## Controls
- Left Arrow: Move paddle left
- Right Arrow: Move paddle right
- Esc: Quit game

## Compilation

### Turbo C (DOS)
```sh
tcc -ml cruzkan.c
```

### Borland C (DOS)
```sh
bcc -ml cruzkan.c
```

### DJGPP (cross-compile from Linux)
```sh
i586-pc-msdosdjgpp-gcc -o cruzkan.exe cruzkan.c -lpc
```

Or simply run:
```sh
make
```

## Running
Run the game in DOS or DOSBox:
```sh
cruzkan.exe
```

## Game Rules
- Destroy all bricks to win
- Don't let the ball fall below the paddle
- Each brick is worth 10 points
- Ball direction changes based on where it hits the paddle

## Technical Details
- Uses VGA Mode 13h (320x200, 256 colors)
- Direct VGA memory manipulation at 0xA000:0000
- BIOS interrupts for keyboard input
- Real-time gameplay with delay timing
