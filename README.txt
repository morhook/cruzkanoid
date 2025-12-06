CRUZKANOID DOS CLONE
=====================

A classic brick-breaking game written in C for DOS.

FEATURES:
---------
- 320x200 VGA graphics (Mode 13h)
- 5 rows x 10 columns of colorful bricks
- Paddle controls with arrow keys
- Ball physics with paddle spin
- Score tracking
- 3 lives
- Win/lose conditions

CONTROLS:
---------
Left Arrow  - Move paddle left
Right Arrow - Move paddle right
ESC         - Quit game

COMPILATION:
------------
For DOS using Turbo C:
  tcc -ml cruzkan.c

For DOS using Borland C:
  bcc -ml cruzkan.c

For cross-compilation from Linux using DJGPP:
  i586-pc-msdosdjgpp-gcc -o cruzkan.exe cruzkan.c -lpc

Or simply run:
  make

RUNNING:
--------
Run the game in DOS or DOSBox:
  cruzkan.exe

GAME RULES:
-----------
- Destroy all bricks to win
- Don't let the ball fall below the paddle
- Each brick is worth 10 points
- Ball direction changes based on where it hits the paddle

TECHNICAL DETAILS:
------------------
- Uses VGA Mode 13h (320x200, 256 colors)
- Direct VGA memory manipulation at 0xA000:0000
- BIOS interrupts for keyboard input
- Real-time gameplay with delay timing

Enjoy the game!
