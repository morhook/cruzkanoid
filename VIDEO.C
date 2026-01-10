#include <dos.h>

#include "video.h"

unsigned char far *VGA = (unsigned char far *)0xA0000000L;

void set_palette_color(unsigned char index, unsigned char r, unsigned char g, unsigned char b)
{
    outp(0x3C8, index);
    outp(0x3C9, r);
    outp(0x3C9, g);
    outp(0x3C9, b);
}

void set_mode(unsigned char mode)
{
    union REGS regs;
    regs.h.ah = 0x00;
    regs.h.al = mode;
    int86(0x10, &regs, &regs);
}

void put_pixel(int x, int y, unsigned char color)
{
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT)
    {
        VGA[y * SCREEN_WIDTH + x] = color;
    }
}

void draw_rect(int x, int y, int width, int height, unsigned char color)
{
    int i, j;
    for (i = 0; i < height; i++)
    {
        for (j = 0; j < width; j++)
        {
            put_pixel(x + j, y + i, color);
        }
    }
}

void draw_filled_rect(int x, int y, int width, int height, unsigned char color)
{
    draw_rect(x, y, width, height, color);
}

void clear_screen(unsigned char color)
{
    int i, j;
    for (i = 0; i < SCREEN_WIDTH; i++)
    {
        for (j = 0; j < SCREEN_HEIGHT; j++)
        {
            put_pixel(i, j, color);
        }
    }
}

void draw_paddle(Paddle paddle)
{
    int i, j;
    int radius = PADDLE_HEIGHT / 2;
    int r = radius - 1;
    int cy = r;
    unsigned char base_color = PADDLE_PALETTE_START + 0;
    unsigned char light_color = PADDLE_PALETTE_START + 1;
    unsigned char dark_color = PADDLE_PALETTE_START + 2;

    for (i = 0; i < PADDLE_HEIGHT; i++)
    {
        for (j = 0; j < paddle.width; j++)
        {
            int inside = 1;
            int dx, dy;
            unsigned char c = base_color;

            /* Rounded ends */
            if (j < radius)
            {
                dx = (radius - 1) - j;
                dy = cy - i;
                if ((dx * dx + dy * dy) > (r * r))
                    inside = 0;
            }
            else if (j >= paddle.width - radius)
            {
                dx = j - (paddle.width - radius);
                dy = cy - i;
                if ((dx * dx + dy * dy) > (r * r))
                    inside = 0;
            }

            if (!inside)
                continue;

            /* Subtle vertical gradient */
            if (i <= 1)
                c = light_color;
            else if (i >= PADDLE_HEIGHT - 2)
                c = dark_color;

            /* Grip pattern in the middle band */
            if (i >= 2 && i <= PADDLE_HEIGHT - 3)
            {
                int center = paddle.width / 2;
                int dist = j - center;
                if (dist < 0)
                    dist = -dist;

                if (dist < (paddle.width / 2 - 6) && ((j & 3) == 0))
                {
                    if (c == base_color)
                        c = dark_color;
                }
            }

            put_pixel(paddle.x + j, paddle.y + i, c);
        }
    }

    /* Bevel outline: light top/left, dark bottom/right (only where shape exists) */
    for (i = 0; i < PADDLE_HEIGHT; i++)
    {
        for (j = 0; j < paddle.width; j++)
        {
            int inside = 1;
            int dx, dy;

            if (j < radius)
            {
                dx = (radius - 1) - j;
                dy = cy - i;
                if ((dx * dx + dy * dy) > (r * r))
                    inside = 0;
            }
            else if (j >= paddle.width - radius)
            {
                dx = j - (paddle.width - radius);
                dy = cy - i;
                if ((dx * dx + dy * dy) > (r * r))
                    inside = 0;
            }

            if (!inside)
                continue;

            /* Top edge */
            if (i == 0)
                put_pixel(paddle.x + j, paddle.y + i, light_color);
            /* Bottom edge */
            if (i == PADDLE_HEIGHT - 1)
                put_pixel(paddle.x + j, paddle.y + i, dark_color);
            /* Left edge */
            if (j == 0)
                put_pixel(paddle.x + j, paddle.y + i, light_color);
            /* Right edge */
            if (j == paddle.width - 1)
                put_pixel(paddle.x + j, paddle.y + i, dark_color);
        }
    }

    /* Specular highlight + "core" stripe */
    if (paddle.width > 14)
    {
        int hx = paddle.x + 6;
        int hy = paddle.y + 2;
        put_pixel(hx, hy, 15);
        put_pixel(hx + 1, hy, 15);
        put_pixel(hx, hy + 1, 15);

        for (j = 8; j < paddle.width - 8; j++)
        {
            if ((j & 1) == 0)
                put_pixel(paddle.x + j, paddle.y + 3, light_color);
        }
    }
}

void erase_paddle(int x, Paddle paddle)
{
    draw_filled_rect(x, paddle.y, paddle.width, PADDLE_HEIGHT, 0);
}

// Draw a square "ball" centered at (cx,cy).
// Uses parameter r as half-size (so size = r*2). Draws a lighter border,
// leaves the four corner pixels empty.
void draw_filled_circle(int cx, int cy, int r, unsigned char color, unsigned char border_color)
{
    int i, j;
    int size = r * 2;
    int x0 = cx - r;
    int y0 = cy - r;

    for (i = 0; i < size; i++)
    {
        for (j = 0; j < size; j++)
        {
            int px = x0 + j;
            int py = y0 + i;

            // Skip the four corner pixels to give the ball "missing corners".
            if ((i == 0 && j == 0) || (i == 0 && j == size - 1) ||
                (i == size - 1 && j == 0) || (i == size - 1 && j == size - 1))
            {
                continue;
            }

            // Border
            if (i == 0 || j == 0 || i == size - 1 || j == size - 1)
            {
                put_pixel(px, py, border_color);
            }
            else
            {
                // Interior
                put_pixel(px, py, color);
            }
        }
    }
}

void draw_ball(Ball ball)
{
    draw_filled_circle(ball.x, ball.y, BALL_SIZE / 2, 0x64, 0x3f);
}

void erase_ball(int x, int y, Ball ball)
{
    int i, j;
    int r = BALL_SIZE / 2;
    int size = r * 2;
    int x0 = x - r;
    int y0 = y - r;

    for (i = 0; i < size; i++)
    {
	for (j = 0; j < size; j++)
	{
	    // Skip the four corners (they were never drawn)
	    if ((i == 0 && j == 0) || (i == 0 && j == size - 1) ||
		(i == size - 1 && j == 0) || (i == size - 1 && j == size - 1))
	    {
		continue;
	    }
	    put_pixel(x0 + j, y0 + i, 0);
	}
    }
}
