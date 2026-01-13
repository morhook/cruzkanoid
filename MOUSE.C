#include <dos.h>

#include "mouse.h"
#include "cruzkan.h"

int mouse_available = 0;
int mouse_buttons = 0;
int mouse_prev_buttons = 0;
int mouse_x = 0;
int mouse_y = 0;

int far mouse_init(void)
{
    union REGS regs;

    regs.x.ax = 0x0000; /* Reset / get installed status */
    int86(0x33, &regs, &regs);
    mouse_available = (regs.x.ax != 0);
    mouse_buttons = regs.x.bx;
    mouse_prev_buttons = 0;
    mouse_x = 0;
    mouse_y = 0;

    if (!mouse_available)
        return 0;

    /* Clamp mouse coordinates to our mode 13h screen. */
    regs.x.ax = 0x0007; /* Set horizontal range */
    regs.x.cx = 0;
    regs.x.dx = SCREEN_WIDTH - 1;
    int86(0x33, &regs, &regs);

    regs.x.ax = 0x0008; /* Set vertical range */
    regs.x.cx = 0;
    regs.x.dx = SCREEN_HEIGHT - 1;
    int86(0x33, &regs, &regs);

    regs.x.ax = 0x0002; /* Hide cursor */
    int86(0x33, &regs, &regs);

    return 1;
}

void far mouse_shutdown(void)
{
    union REGS regs;

    if (!mouse_available)
        return;

    regs.x.ax = 0x0001; /* Show cursor */
    int86(0x33, &regs, &regs);
}

void far mouse_update(void)
{
    union REGS regs;

    if (!mouse_available)
        return;

    regs.x.ax = 0x0003; /* Get position + buttons */
    int86(0x33, &regs, &regs);
    mouse_buttons = regs.x.bx;
    mouse_x = regs.x.cx;
    mouse_y = regs.x.dx;
}

void far mouse_set_pos(int x, int y)
{
    union REGS regs;

    if (!mouse_available)
        return;

    if (x < 0)
        x = 0;
    if (x >= SCREEN_WIDTH)
        x = SCREEN_WIDTH - 1;
    if (y < 0)
        y = 0;
    if (y >= SCREEN_HEIGHT)
        y = SCREEN_HEIGHT - 1;

    regs.x.ax = 0x0004; /* Set position */
    regs.x.cx = x;
    regs.x.dx = y;
    int86(0x33, &regs, &regs);
    mouse_x = x;
    mouse_y = y;
}
