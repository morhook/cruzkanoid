# CRUZKANOID DOS CLONE

A classic brick-breaking game written in C for DOS. The home page is in [morhook.itch.io/cruzkanoid](https://morhook.itch.io/cruzkanoid). For downloading compiled versions, go there.

![CruzKanoid Art Box](game-box-design.png)

## Screenshots

![Cruzkanoid running in DOSBox-X](cruzkanoid_on_dosbox.png)


## Features
- 320x200 VGA graphics (Mode 13h)
- 5 rows x 10 columns of colorful bricks
- Paddle controls with arrow keys
- Ball physics with paddle spin
- Score tracking
- 3 lives
- Sound effects (PC speaker, optional Sound Blaster)
- Win/lose conditions

## Controls
- Left Arrow: Move paddle left
- Right Arrow: Move paddle right
- Space: Launch ball
- S: Toggle sound effects
- M: Toggle soundtrack
- Esc: Quit game

## Compilation

This project is intended to be compiled with Turbo C 3.0 inside DOSBox-X.

1.  Start DOSBox-X and mount your project directory and the directory containing Turbo C 3.0.
2.  Navigate to your Turbo C 3.0 directory (e.g., `D:\TC\BIN`).
3.  Run `TC.EXE` to launch the Turbo C IDE.
4.  Inside the IDE, open the `CRUZKAN.PRJ` project from your mounted project directory.
5.  Compile and run the program from within the IDE.

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
- Sound: tries Sound Blaster DSP (8-bit DMA) using the `BLASTER` env var, falls back to PC speaker
