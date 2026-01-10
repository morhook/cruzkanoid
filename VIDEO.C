#include <dos.h>

#include "cruzkan.h"
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

unsigned char *get_font_char(char c)
{
    static unsigned char font_data[47][7] = {
        /* 0 */ {0x70, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70},
        /* 1 */ {0x20, 0x60, 0x20, 0x20, 0x20, 0x20, 0x70},
        /* 2 */ {0x70, 0x88, 0x08, 0x10, 0x20, 0x40, 0xF8},
        /* 3 */ {0x70, 0x88, 0x08, 0x30, 0x08, 0x88, 0x70},
        /* 4 */ {0x10, 0x30, 0x50, 0x90, 0xF8, 0x10, 0x10},
        /* 5 */ {0xF8, 0x80, 0xF0, 0x08, 0x08, 0x88, 0x70},
        /* 6 */ {0x30, 0x40, 0x80, 0xF0, 0x88, 0x88, 0x70},
        /* 7 */ {0xF8, 0x08, 0x10, 0x20, 0x40, 0x40, 0x40},
        /* 8 */ {0x70, 0x88, 0x88, 0x70, 0x88, 0x88, 0x70},
        /* 9 */ {0x70, 0x88, 0x88, 0x78, 0x08, 0x10, 0x60},
        /* : */ {0x00, 0x20, 0x00, 0x00, 0x00, 0x20, 0x00},
        /* A */ {0x20, 0x50, 0x88, 0x88, 0xF8, 0x88, 0x88},
        /* E */ {0xF8, 0x80, 0x80, 0xF0, 0x80, 0x80, 0xF8},
        /* F */ {0xF8, 0x80, 0x80, 0xF0, 0x80, 0x80, 0x80},
        /* G */ {0x70, 0x88, 0x80, 0xB8, 0x88, 0x88, 0x70},
        /* I */ {0x70, 0x20, 0x20, 0x20, 0x20, 0x20, 0x70},
        /* L */ {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xF8},
        /* M */ {0x88, 0xD8, 0xA8, 0xA8, 0x88, 0x88, 0x88},
        /* N */ {0x88, 0x88, 0xC8, 0xA8, 0x98, 0x88, 0x88},
        /* O */ {0x70, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70},
        /* P */ {0xF0, 0x88, 0x88, 0xF0, 0x80, 0x80, 0x80},
        /* R */ {0xF0, 0x88, 0x88, 0xF0, 0x90, 0x88, 0x88},
        /* S */ {0x70, 0x88, 0x80, 0x70, 0x08, 0x88, 0x70},
        /* U */ {0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70},
        /* V */ {0x88, 0x88, 0x88, 0x88, 0x50, 0x50, 0x20},
        /* W */ {0x88, 0x88, 0x88, 0xA8, 0xA8, 0xD8, 0x88},
        /* Y */ {0x88, 0x88, 0x50, 0x20, 0x20, 0x20, 0x20},
        /* a */ {0x00, 0x00, 0x70, 0x08, 0x78, 0x88, 0x78},
        /* c */ {0x00, 0x00, 0x70, 0x80, 0x80, 0x88, 0x70},
        /* e */ {0x00, 0x00, 0x70, 0x88, 0xF8, 0x80, 0x70},
        /* i */ {0x20, 0x00, 0x60, 0x20, 0x20, 0x20, 0x70},
        /* k */ {0x80, 0x80, 0x90, 0xA0, 0xE0, 0x90, 0x88},
        /* l */ {0x60, 0x20, 0x20, 0x20, 0x20, 0x20, 0x70},
        /* n */ {0x00, 0x00, 0xB0, 0xC8, 0x88, 0x88, 0x88},
        /* o */ {0x00, 0x00, 0x70, 0x88, 0x88, 0x88, 0x70},
        /* r */ {0x00, 0x00, 0x68, 0x70, 0x40, 0x40, 0x40},
        /* s */ {0x00, 0x00, 0x70, 0x80, 0x70, 0x08, 0xF0},
        /* t */ {0x20, 0x20, 0x70, 0x20, 0x20, 0x20, 0x18},
        /* v */ {0x00, 0x00, 0x88, 0x88, 0x88, 0x50, 0x20},
        /* x */ {0x00, 0x00, 0x88, 0x50, 0x20, 0x50, 0x88},
        /* y */ {0x00, 0x00, 0x88, 0x88, 0x78, 0x08, 0x70},
        /*spc*/ {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        /* C */ {0x70, 0x88, 0x80, 0x80, 0x80, 0x88, 0x70},
        /* D */ {0xF0, 0x88, 0x88, 0x88, 0x88, 0x88, 0xF0},
        /* K */ {0x88, 0x98, 0xA8, 0xC0, 0xA8, 0x98, 0x88},
        /* Z */ {0xF8, 0x08, 0x10, 0x20, 0x40, 0x80, 0xF8},
        /*unk*/ {0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8}};

    if (c >= '0' && c <= '9')
        return font_data[c - '0'];
    if (c == ':')
        return font_data[10];
    if (c == 'A')
        return font_data[11];
    if (c == 'C')
        return font_data[42];
    if (c == 'D')
        return font_data[43];
    if (c == 'E')
        return font_data[12];
    if (c == 'F')
        return font_data[13];
    if (c == 'G')
        return font_data[14];
    if (c == 'I')
        return font_data[15];
    if (c == 'K')
        return font_data[44];
    if (c == 'L')
        return font_data[16];
    if (c == 'M')
        return font_data[17];
    if (c == 'N')
        return font_data[18];
    if (c == 'O')
        return font_data[19];
    if (c == 'P')
        return font_data[20];
    if (c == 'R')
        return font_data[21];
    if (c == 'S')
        return font_data[22];
    if (c == 'U')
        return font_data[23];
    if (c == 'V')
        return font_data[24];
    if (c == 'W')
        return font_data[25];
    if (c == 'Y')
        return font_data[26];
    if (c == 'Z')
        return font_data[45];
    if (c == 'a')
        return font_data[27];
    if (c == 'c')
        return font_data[28];
    if (c == 'e')
        return font_data[29];
    if (c == 'i')
        return font_data[30];
    if (c == 'k')
        return font_data[31];
    if (c == 'l')
        return font_data[32];
    if (c == 'n')
        return font_data[33];
    if (c == 'o')
        return font_data[34];
    if (c == 'r')
        return font_data[35];
    if (c == 's')
        return font_data[36];
    if (c == 't')
        return font_data[37];
    if (c == 'v')
        return font_data[38];
    if (c == 'x')
        return font_data[39];
    if (c == 'y')
        return font_data[40];
    if (c == ' ')
        return font_data[41];
    return font_data[46]; /* unknown char */
}

void draw_char(int x, int y, char c, unsigned char color)
{
    int i, j;
    unsigned char mask;
    unsigned char *font;

    font = get_font_char(c);

    for (i = 0; i < 7; i++)
    {
        mask = font[i];
        for (j = 0; j < 8; j++)
        {
            if (mask & (0x80 >> j))
            {
                put_pixel(x + j, y + i, color);
            }
            else
            {
                put_pixel(x + j, y + i, 0);
            }
        }
    }
}

void draw_text(int x, int y, char *text)
{
    int i = 0;
    while (text[i] != '\0')
    {
        draw_char(x + i * 6, y, text[i], 15);
        i++;
    }
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

