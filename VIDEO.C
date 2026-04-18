#include <dos.h>
#include <alloc.h>
#include <string.h>
#include <mem.h>
#include <stdio.h>

#include "cruzkan.h"
#include "video.h" 

extern int current_level;

static unsigned char far *vga_real = (unsigned char far *)0xA0000000L;
static unsigned char far *vga_back = 0;
unsigned char far *VGA = (unsigned char far *)0xA0000000L;

static unsigned char far *background_buffer = 0;

/* Pig pause image */
static unsigned char far *pig_pixels = 0;
static unsigned char pig_palette[256][3];
static unsigned char game_palette[256][3];
static int pig_loaded = 0;
static int game_palette_saved = 0;

void far init_video_buffers(void)
{
    if (!vga_back)
        vga_back = (unsigned char far *)farmalloc(64000L);
    if (!background_buffer)
        background_buffer = (unsigned char far *)farmalloc(64000L);
}

void far close_video_buffers(void)
{
    if (vga_back)
    {
        farfree(vga_back);
        vga_back = 0;
    }
    if (background_buffer)
    {
        farfree(background_buffer);
        background_buffer = 0;
    }
    VGA = vga_real;
}

void far use_backbuffer(int use)
{
    if (use && vga_back)
        VGA = vga_back;
    else
        VGA = vga_real;
}

void far flip_video(void)
{
    if (vga_back)
    {
        wait_vblank();
        _fmemcpy(vga_real, vga_back, 64000U);
    }
}

static void capture_background(void)
{
    if (!background_buffer)
        return;
    _fmemcpy(background_buffer, vga_real, 64000U);
}

void far set_palette_color(unsigned char index, unsigned char r, unsigned char g, unsigned char b)
{
    outp(0x3C8, index);
    outp(0x3C9, r);
    outp(0x3C9, g);
    outp(0x3C9, b);
}

void far wait_vblank(void)
{
    /* VGA status register: bit 3 is vertical retrace. */
    while (inp(0x3DA) & 0x08)
        ;
    while (!(inp(0x3DA) & 0x08))
        ;
}

void far fade_palette_color(unsigned char index,
                        unsigned char from_r, unsigned char from_g, unsigned char from_b,
                        unsigned char to_r, unsigned char to_g, unsigned char to_b,
                        int steps, int delay_ms)
{
    int i;

    if (steps <= 0)
        steps = 1;

    for (i = 1; i <= steps; i++)
    {
        int r = (int)from_r + (((int)to_r - (int)from_r) * i) / steps;
        int g = (int)from_g + (((int)to_g - (int)from_g) * i) / steps;
        int b = (int)from_b + (((int)to_b - (int)from_b) * i) / steps;

        set_palette_color(index, (unsigned char)r, (unsigned char)g, (unsigned char)b);
        wait_vblank();
        if (delay_ms > 0)
            delay(delay_ms);
    }
}

void far set_mode(unsigned char mode)
{
    union REGS regs;
    regs.h.ah = 0x00;
    regs.h.al = mode;
    int86(0x10, &regs, &regs);
}
void far put_pixel(int x, int y, unsigned char color)
{
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT)
    {
        VGA[y * SCREEN_WIDTH + x] = color;
    }
}

void far draw_rect(int x, int y, int width, int height, unsigned char color)
{
    int i;
    for (i = 0; i < height; i++)
    {
        int py = y + i;
        if (py < 0 || py >= SCREEN_HEIGHT) continue;
        _fmemset(VGA + (unsigned long)py * SCREEN_WIDTH + x, color, width);
    }
}

void far draw_filled_rect(int x, int y, int width, int height, unsigned char color)
{
    draw_rect(x, y, width, height, color);
}

void far clear_screen(unsigned char color)
{
    _fmemset(VGA, color, 64000U);
}
void far draw_paddle(Paddle paddle)
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

    if (paddle.laser_enabled && paddle.width >= 12)
    {
        int lx = paddle.x + 1;
        int rx = paddle.x + paddle.width - 3;

        for (i = 1; i < PADDLE_HEIGHT - 1; i++)
        {
            put_pixel(lx, paddle.y + i, dark_color);
            put_pixel(lx + 1, paddle.y + i, 15);

            put_pixel(rx + 1, paddle.y + i, dark_color);
            put_pixel(rx, paddle.y + i, 15);
        }

        put_pixel(lx, paddle.y - 1, dark_color);
        put_pixel(lx + 1, paddle.y - 1, 15);
        put_pixel(lx, paddle.y - 2, dark_color);
        put_pixel(lx + 1, paddle.y - 2, 15);

        put_pixel(rx + 1, paddle.y - 1, dark_color);
        put_pixel(rx, paddle.y - 1, 15);
        put_pixel(rx + 1, paddle.y - 2, dark_color);
        put_pixel(rx, paddle.y - 2, 15);
    }
}

void far erase_paddle(int x, Paddle paddle)
{
    draw_filled_rect(x, paddle.y, paddle.width, PADDLE_HEIGHT, 0);
}


static unsigned char *get_font_char(char c)
{
    static unsigned char font_data[68][7] = {
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
        /* B */ {0xF0, 0x88, 0x88, 0xF0, 0x88, 0x88, 0xF0},
        /* H */ {0x88, 0x88, 0x88, 0xF8, 0x88, 0x88, 0x88},
        /* T */ {0xF8, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20},
        /* ! */ {0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x20},
        /* / */ {0x08, 0x10, 0x10, 0x20, 0x40, 0x40, 0x80},
        /* . */ {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20},
        /* , */ {0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x40},
        /* b */ {0x80, 0x80, 0xF0, 0x88, 0x88, 0x88, 0xF0},
        /* d */ {0x08, 0x08, 0x78, 0x88, 0x88, 0x88, 0x78},
        /* f */ {0x18, 0x20, 0x70, 0x20, 0x20, 0x20, 0x20},
        /* g */ {0x00, 0x00, 0x78, 0x88, 0x88, 0x78, 0x08, },
        /* h */ {0x80, 0x80, 0xF0, 0x88, 0x88, 0x88, 0x88},
        /* m */ {0x00, 0x00, 0xD8, 0xA8, 0xA8, 0xA8, 0x88},
        /* p */ {0x00, 0x00, 0xF0, 0x88, 0x88, 0xF0, 0x80},
        /* u */ {0x00, 0x00, 0x88, 0x88, 0x88, 0x98, 0x68},
        /* w */ {0x00, 0x00, 0x88, 0x88, 0xA8, 0xA8, 0x50},
        /* - */ {0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0x00},
        /* ' */ {0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00},
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
    if (c == 'B')
        return font_data[46];
    if (c == 'H')
        return font_data[47];
    if (c == 'T')
        return font_data[48];
    if (c == '!')
        return font_data[49];
    if (c == '/')
        return font_data[50];
    if (c == '.')
        return font_data[51];
    if (c == ',')
        return font_data[52];
    if (c == 'a')
        return font_data[27];
    if (c == 'b')
        return font_data[53];
    if (c == 'c')
        return font_data[28];
    if (c == 'd')
        return font_data[54];
    if (c == 'e')
        return font_data[29];
    if (c == 'f')
        return font_data[55];
    if (c == 'g')
        return font_data[56];
    if (c == 'h')
        return font_data[57];
    if (c == 'i')
        return font_data[30];
    if (c == 'k')
        return font_data[31];
    if (c == 'l')
        return font_data[32];
    if (c == 'm')
        return font_data[58];
    if (c == 'n')
        return font_data[33];
    if (c == 'o')
        return font_data[34];
    if (c == 'p')
        return font_data[59];
    if (c == 'r')
        return font_data[35];
    if (c == 's')
        return font_data[36];
    if (c == 't')
        return font_data[37];
    if (c == 'u')
        return font_data[60];
    if (c == 'v')
        return font_data[38];
    if (c == 'w')
        return font_data[61];
    if (c == 'x')
        return font_data[39];
    if (c == 'y')
        return font_data[40];
    if (c == ' ')
        return font_data[41];
    if (c == '-')
        return font_data[62];
    if (c == '\'')
        return font_data[63];
    return font_data[64]; /* unknown char */
}

void far draw_char(int x, int y, char c, unsigned char color)
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

void far draw_text(int x, int y, char *text)
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
void far draw_filled_circle(int cx, int cy, int r, unsigned char color, unsigned char border_color)
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

void far draw_ball(Ball ball)
{
    draw_filled_circle(ball.x, ball.y, BALL_SIZE / 2, 0x64, 0x22);
}

void far draw_laser_shot(LaserShot shot)
{
    int i;

    if (!shot.active)
        return;

    for (i = 0; i < 4; i++)
    {
        put_pixel(shot.x - 1, shot.y - i, 12);
        put_pixel(shot.x, shot.y - i, 15);
        put_pixel(shot.x + 1, shot.y - i, 12);
    }
}

void far draw_pill(Pill pill)
{
    int i, j;
    int rx = PILL_WIDTH / 2;
    int top_cy = rx;
    int bottom_cy = (PILL_HEIGHT - 1) - rx;
    int x0 = pill.x - rx;
    int y0 = pill.y - (PILL_HEIGHT / 2);
    unsigned char light_color = PILL_COLOR_LIGHT;
    unsigned char base_color = PILL_COLOR_BASE;
    unsigned char border_color = PILL_COLOR_BORDER;
    unsigned char glyph_color = PILL_COLOR_GLYPH;
    int glyph_w = 5;
    int glyph_h = 5;
    int glyph_px = pill.x - (glyph_w / 2);
    int glyph_py = pill.y - (glyph_h / 2);
    const char (*glyph)[6];
    static const char glyph_l[5][6] = {
        "10000",
        "10000",
        "10000",
        "10000",
        "11111"
    };
    static const char glyph_x[5][6] = {
        "10001",
        "01010",
        "00100",
        "01010",
        "10001"
    };
    static const char glyph_s[5][6] = {
        "01111",
        "10000",
        "01110",
        "00001",
        "11110"
    };
    static const char glyph_m[5][6] = {
        "10001",
        "11011",
        "10101",
        "10001",
        "10001"
    };
    static const char glyph_f[5][6] = {
        "11111",
        "10000",
        "11110",
        "10000",
        "10000"
    };

    for (i = 0; i < PILL_HEIGHT; i++)
    {
        for (j = 0; j < PILL_WIDTH; j++)
        {
            int dx = j - rx;
            int inside = 1;

            if (i < top_cy)
            {
                int dy = i - top_cy;
                if ((dx * dx + dy * dy) > (rx * rx))
                    inside = 0;
            }
            else if (i > bottom_cy)
            {
                int dy = i - bottom_cy;
                if ((dx * dx + dy * dy) > (rx * rx))
                    inside = 0;
            }

            if (!inside)
                continue;

            if (i == 0 || i == PILL_HEIGHT - 1 || j == 0 || j == PILL_WIDTH - 1)
            {
                put_pixel(x0 + j, y0 + i, border_color);
            }
            else
            {
                if (j < rx)
                    put_pixel(x0 + j, y0 + i, light_color);
                else
                    put_pixel(x0 + j, y0 + i, base_color);
            }
        }
    }

    if (pill.type == PILL_TYPE_GROW)
        glyph = glyph_x;
    else if (pill.type == PILL_TYPE_SHRINK)
        glyph = glyph_s;
    else if (pill.type == PILL_TYPE_MULTIBALL)
        glyph = glyph_m;
    else if (pill.type == PILL_TYPE_LASER)
        glyph = glyph_f;
    else
        glyph = glyph_l;

    for (i = 0; i < glyph_h; i++)
    {
        for (j = 0; j < glyph_w; j++)
        {
            if (glyph[i][j] == '1')
            {
                put_pixel(glyph_px + j, glyph_py + i, glyph_color);
            }
        }
    }
}

void far erase_ball(int x, int y, Ball ball)
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

void far erase_pill_with_background(int x, int y)
{
    int x1 = x - (PILL_WIDTH / 2) - 1;
    int y1 = y - (PILL_HEIGHT / 2) - 1;
    int x2 = x + (PILL_WIDTH / 2) + 1;
    int y2 = y + (PILL_HEIGHT / 2) + 1;

    draw_background_area(x1, y1, x2, y2);
}

void far erase_laser_shot_with_background(int x, int y)
{
    draw_background_area(x - 1, y - 4, x + 1, y + 1);
}

void far draw_ui(int score, int lives, int current_level)
{
    char buffer[50];

    // Draw border
    draw_rect(0, 0, SCREEN_WIDTH, 1, 15);
    draw_rect(0, 0, 1, SCREEN_HEIGHT, 15);
    draw_rect(SCREEN_WIDTH - 1, 0, 1, SCREEN_HEIGHT, 15);

    // Score and lives at top (pixel coordinates now)
    erase_rect_with_background(5, 5, 72, 7);
    sprintf(buffer, "Score: %d", score);
    draw_text(5, 5, buffer);

    erase_rect_with_background(120, 5, 72, 7);
    sprintf(buffer, "Level: %d", current_level);
    draw_text(120, 5, buffer);

    erase_rect_with_background(200, 5, 72, 7);
    sprintf(buffer, "Lives: %d", lives);
    draw_text(200, 5, buffer);
}
void far draw_char_transparent(int x, int y, char c, unsigned char color)
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
        }
    }
}

void far draw_text_transparent(int x, int y, char *text, unsigned char color)
{
    int i = 0;
    while (text[i] != '\0')
    {
        draw_char_transparent(x + i * 8, y, text[i], color);
        i++;
    }
}

void far draw_bordered_text(int x, int y, char *text, unsigned char border_color)
{
    // Draw border
    draw_text_transparent(x - 1, y, text, border_color);
    draw_text_transparent(x + 1, y, text, border_color);
    draw_text_transparent(x, y - 1, text, border_color);
    draw_text_transparent(x, y + 1, text, border_color);

    // Draw black inner text
    draw_text_transparent(x, y, text, 0);
}

void far save_vga_palette(void)
{
    int i;
    for (i = 0; i < 256; i++)
    {
        outp(0x3C7, (unsigned char)i);
        game_palette[i][0] = inp(0x3C9);
        game_palette[i][1] = inp(0x3C9);
        game_palette[i][2] = inp(0x3C9);
    }
    game_palette_saved = 1;
}

void far restore_vga_palette(void)
{
    int i;
    if (!game_palette_saved)
        return;
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++)
    {
        outp(0x3C9, game_palette[i][0]);
        outp(0x3C9, game_palette[i][1]);
        outp(0x3C9, game_palette[i][2]);
    }
}

static void set_pig_palette(void)
{
    int i;
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++)
    {
        outp(0x3C9, pig_palette[i][0]);
        outp(0x3C9, pig_palette[i][1]);
        outp(0x3C9, pig_palette[i][2]);
    }
}

static void load_pig_pcx(void)
{
    FILE *f;
    long file_size;
    long palette_offset;
    long pixel_offset;
    unsigned char byte, color;
    int count;
    long decoded;
    int i;
    unsigned char buf3[3];

    if (pig_loaded)
        return;

    if (!pig_pixels)
        pig_pixels = (unsigned char far *)farmalloc(64000L);
    if (!pig_pixels)
        return;

    f = fopen("PIG.PCX", "rb");
    if (!f)
        return;

    /* Get file size */
    fseek(f, 0L, SEEK_END);
    file_size = ftell(f);

    /* Read 256-color palette from last 769 bytes (marker 0x0C + 768 bytes RGB) */
    palette_offset = file_size - 769L;
    fseek(f, palette_offset, SEEK_SET);
    fread(buf3, 1, 1, f); /* skip 0x0C marker */
    for (i = 0; i < 256; i++)
    {
        fread(buf3, 1, 3, f);
        /* PCX palette: 0-255, VGA DAC: 0-63 -> shift right 2 */
        pig_palette[i][0] = buf3[0] >> 2;
        pig_palette[i][1] = buf3[1] >> 2;
        pig_palette[i][2] = buf3[2] >> 2;
    }

    /* Decode RLE pixel data starting at byte 128 */
    pixel_offset = 128L;
    fseek(f, pixel_offset, SEEK_SET);
    decoded = 0L;
    while (decoded < 64000L)
    {
        byte = (unsigned char)fgetc(f);
        if ((byte & 0xC0) == 0xC0)
        {
            count = (int)(byte & 0x3F);
            color = (unsigned char)fgetc(f);
            while (count-- > 0 && decoded < 64000L)
                pig_pixels[decoded++] = color;
        }
        else
        {
            pig_pixels[decoded++] = byte;
        }
    }

    fclose(f);
    pig_loaded = 1;
}

void far draw_pause_overlay(void)
{
    char line1[] = "PAUSED";
    char line2[] = "P OR PAUSE";
    int x1 = (SCREEN_WIDTH - ((sizeof(line1) - 1) * 8)) / 2;
    int x2 = (SCREEN_WIDTH - ((sizeof(line2) - 1) * 8)) / 2;
    int y = 8;

    load_pig_pcx();

    if (pig_loaded && pig_pixels)
    {
        set_pig_palette();
        _fmemcpy(vga_real, pig_pixels, 64000L);
    }
    else
    {
        draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    }

    draw_bordered_text(x1, y, line1, 15);
    draw_bordered_text(x2, y + 12, line2, 15);
}

void far draw_heart(int cx, int cy, int size, unsigned char color)
{
    int x, y;
    int start_x = cx - 4;
    int start_y = cy - 4;
    static const char *heart[9] = {
        "001001000",
        "011111100",
        "111111110",
        "111111110",
        "011111100",
        "001111000",
        "000110000",
        "000000000",
        "000000000"
    };

    (void)size;

    for (y = 0; y < 9; y++)
    {
        for (x = 0; x < 9; x++)
        {
            if (heart[y][x] == '1')
            {
                put_pixel(start_x + x, start_y + y, color);
            }
        }
    }
}

void far draw_dead_icon(int cx, int cy, unsigned char color)
{
    int x, y;
    int start_x = cx - 4;
    int start_y = cy - 4;
    /* 9x9 skull bitmap */
    static const char *skull[9] = {
        "011111100",
        "111111110",
        "101111010",
        "111111110",
        "011111100",
        "010101010",
        "011111100",
        "000000000",
        "000000000"
    };

    for (y = 0; y < 9; y++)
    {
        for (x = 0; x < 9; x++)
        {
            if (skull[y][x] == '1')
            {
                put_pixel(start_x + x, start_y + y, color);
            }
        }
    }
}

void far draw_background()
{
    int x, y;
    int heart_size = 9;
    int spacing = heart_size + 1;
    int offset = heart_size / 2;
    /* Pink palette now lives at indices 59-62 (moved from 52-55).
       base_color = 59 (light pink), variants at 60 and 61. */
    unsigned char base_color = 59;

    /* Level 2 and levels 6-10: plain black background */
    /* Levels 11-15: dark blue tint background */
    /* Levels 1, 3-5: pink heart background (default) */
    if (current_level == 2 || (current_level >= 6 && current_level <= 10))
    {
        draw_filled_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);

        if (!background_buffer)
        {
            background_buffer = (unsigned char far *)farmalloc((unsigned long)SCREEN_WIDTH * SCREEN_HEIGHT);
        }

        capture_background();
        return;
    }

    if (current_level >= 11)
    {
        /* Deep navy / dark blue tint */
        draw_filled_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 1);

        if (!background_buffer)
        {
            background_buffer = (unsigned char far *)farmalloc((unsigned long)SCREEN_WIDTH * SCREEN_HEIGHT);
        }

        capture_background();
        return;
    }

    /* Levels 1-5: pink heart background. */
    /* Clear to base color so hearts don't stack or leave artifacts. */
    draw_filled_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, base_color);

    if (!background_buffer)
    {
        background_buffer = (unsigned char far *)farmalloc((unsigned long)SCREEN_WIDTH * SCREEN_HEIGHT);
    }

    for (y = offset; y + heart_size - 1 < SCREEN_HEIGHT; y += spacing)
    {
        for (x = offset; x + heart_size - 1 < SCREEN_WIDTH; x += spacing)
        {
            /* Vary heart colors for visual interest */
            unsigned char color = 59 + ((x + y) / spacing) % 3;
            draw_heart(x, y, heart_size, color + 1);
        }
    }

    capture_background();
}

/* Helper function to determine the exact color a heart should have */
unsigned char get_heart_color(int x, int y)
{
    int heart_size = 8;
    int spacing = heart_size - 2;

    /* Pink palette now at 59-61 (moved from 52-54). */
    return 59 + ((x + y) / spacing) % 3;
}

void far draw_background_area(int x1, int y1, int x2, int y2)
{
    /* Ensure coordinates are within bounds and x1 < x2, y1 < y2 */
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= SCREEN_WIDTH) x2 = SCREEN_WIDTH - 1;
    if (y2 >= SCREEN_HEIGHT) y2 = SCREEN_HEIGHT - 1;
    if (x1 > x2 || y1 > y2) return;
    
    if (x1 > x2 || y1 > y2) return;

    if (!background_buffer)
    {
        draw_background();
    }
    else
    {
        int x, y;
        unsigned long row;

        for (y = y1; y <= y2; y++)
        {
            row = (unsigned long)y * SCREEN_WIDTH;
            for (x = x1; x <= x2; x++)
            {
                VGA[row + x] = background_buffer[row + x];
            }
        }
    }
}

void far erase_ball_with_background(int x, int y, Ball ball)
{
    int r = BALL_SIZE / 2;
    int size = r * 2;
    int x1 = x - r - 1;
    int y1 = y - r - 1;
    int x2 = x + r + 1;
    int y2 = y + r + 1;
    
    /* Restore background in the ball's previous area */
    draw_background_area(x1, y1, x2, y2);
}

void far erase_paddle_with_background(int x, Paddle paddle)
{
    int x1 = x - 1;
    int y1 = paddle.y - 1;
    int x2 = x + paddle.width + 1;
    int y2 = paddle.y + PADDLE_HEIGHT + 1;

    if (paddle.laser_enabled)
        y1 = paddle.y - 3;
    
    /* Restore background in the paddle's previous area */
    draw_background_area(x1, y1, x2, y2);
}

void far erase_rect_with_background(int x, int y, int width, int height)
{
    int x1 = x;
    int y1 = y;
    int x2 = x + width - 1;
    int y2 = y + height - 1;
    
    /* Restore background only where the brick was drawn */
    draw_background_area(x1, y1, x2, y2);
}


void far draw_monster(Monster monster)
{
    int i, j;
    int x = monster.x;
    int y = monster.y;
    int w = MONSTER_WIDTH;
    int h = MONSTER_HEIGHT;
    unsigned char body_color  = MONSTER_PALETTE_START + 0;
    unsigned char light_color = MONSTER_PALETTE_START + 1;
    unsigned char dark_color  = MONSTER_PALETTE_START + 2;
    unsigned char eye_color   = 15; /* white eyes */
    unsigned char pupil_color = 0;  /* black pupils */
    unsigned char tooth_color = 15; /* white teeth */
    int hp = monster.hp;

    /* Body fill */
    for (i = 1; i < h - 1; i++)
    {
        for (j = 1; j < w - 1; j++)
        {
            unsigned char c = body_color;
            /* Top highlight row */
            if (i == 1)
                c = light_color;
            /* Bottom shadow row */
            else if (i == h - 2)
                c = dark_color;
            put_pixel(x + j, y + i, c);
        }
    }

    /* Border */
    for (j = 0; j < w; j++)
    {
        put_pixel(x + j, y,         dark_color);
        put_pixel(x + j, y + h - 1, dark_color);
    }
    for (i = 0; i < h; i++)
    {
        put_pixel(x,         y + i, dark_color);
        put_pixel(x + w - 1, y + i, dark_color);
    }

    /* Spiky top — three upward spikes */
    put_pixel(x + 4,  y - 1, dark_color);
    put_pixel(x + 4,  y - 2, body_color);
    put_pixel(x + 11, y - 1, dark_color);
    put_pixel(x + 11, y - 2, body_color);
    put_pixel(x + 11, y - 3, body_color);
    put_pixel(x + 18, y - 1, dark_color);
    put_pixel(x + 18, y - 2, body_color);

    /* Eyes — two 3x2 white rectangles */
    /* Left eye */
    put_pixel(x + 4, y + 3, eye_color);
    put_pixel(x + 5, y + 3, eye_color);
    put_pixel(x + 6, y + 3, eye_color);
    put_pixel(x + 4, y + 4, eye_color);
    put_pixel(x + 5, y + 4, eye_color);
    put_pixel(x + 6, y + 4, eye_color);
    /* Left pupil */
    put_pixel(x + 5, y + 4, pupil_color);

    /* Right eye */
    put_pixel(x + 16, y + 3, eye_color);
    put_pixel(x + 17, y + 3, eye_color);
    put_pixel(x + 18, y + 3, eye_color);
    put_pixel(x + 16, y + 4, eye_color);
    put_pixel(x + 17, y + 4, eye_color);
    put_pixel(x + 18, y + 4, eye_color);
    /* Right pupil */
    put_pixel(x + 17, y + 4, pupil_color);

    /* Angry eyebrows (slant inward) */
    put_pixel(x + 4,  y + 2, dark_color);
    put_pixel(x + 5,  y + 2, dark_color);
    put_pixel(x + 6,  y + 1, dark_color);
    put_pixel(x + 16, y + 1, dark_color);
    put_pixel(x + 17, y + 2, dark_color);
    put_pixel(x + 18, y + 2, dark_color);

    /* Mouth — jagged teeth row, width varies with hp */
    {
        int mouth_y = y + h - 4;
        int tooth_gap;

        /* Draw a row of teeth; fewer teeth as hp decreases */
        if (hp >= 3)
        {
            /* Full set: 3 teeth */
            for (j = 4; j <= 19; j++)
                put_pixel(x + j, mouth_y, dark_color);
            /* Tooth tips */
            put_pixel(x + 5,  mouth_y - 1, tooth_color);
            put_pixel(x + 6,  mouth_y - 1, tooth_color);
            put_pixel(x + 11, mouth_y - 1, tooth_color);
            put_pixel(x + 12, mouth_y - 1, tooth_color);
            put_pixel(x + 17, mouth_y - 1, tooth_color);
            put_pixel(x + 18, mouth_y - 1, tooth_color);
        }
        else if (hp == 2)
        {
            /* Damaged: 2 teeth */
            for (j = 4; j <= 19; j++)
                put_pixel(x + j, mouth_y, dark_color);
            put_pixel(x + 7,  mouth_y - 1, tooth_color);
            put_pixel(x + 8,  mouth_y - 1, tooth_color);
            put_pixel(x + 15, mouth_y - 1, tooth_color);
            put_pixel(x + 16, mouth_y - 1, tooth_color);
        }
        else
        {
            /* Nearly dead: cracked mouth */
            for (j = 4; j <= 19; j++)
                put_pixel(x + j, mouth_y, dark_color);
            put_pixel(x + 11, mouth_y - 1, tooth_color);
            put_pixel(x + 12, mouth_y - 1, tooth_color);
        }
        (void)tooth_gap;
    }
}

void far erase_monster_with_background(int x, int y)
{
    /* +3 above for spikes */
    draw_background_area(x - 1, y - 3, x + MONSTER_WIDTH + 1, y + MONSTER_HEIGHT + 1);
}

void draw_brick(int x, int y, int width, int height, unsigned char base_color)
{
    int i, j;
    unsigned char light_color = base_color + 1;
    unsigned char dark_color = base_color + 2;

    /* Fill interior with a subtle "shine" + texture. */
    for (i = 1; i < height - 1; i++)
    {
        for (j = 1; j < width - 1; j++)
        {
            unsigned char c = base_color;

            /* Shiny top half, slightly darker bottom half */
            if (i < (height / 2))
            {
                if (((i + j) & 3) == 0)
                    c = light_color;
            }
            else
            {
                if (((i + j) & 3) == 0)
                    c = dark_color;
            }

            put_pixel(x + j, y + i, c);
        }
    }

    /* Beveled border: light on top/left, dark on bottom/right */
    for (j = 0; j < width; j++)
    {
        put_pixel(x + j, y, light_color);
        put_pixel(x + j, y + height - 1, dark_color);
    }
    for (i = 0; i < height; i++)
    {
        put_pixel(x, y + i, light_color);
        put_pixel(x + width - 1, y + i, dark_color);
    }

    /* Extra inner highlight stripe for a more "arcade" look */
    if (height > 4 && width > 6)
    {
        for (j = 2; j < width - 2; j++)
        {
            if ((j & 1) == 0)
                put_pixel(x + j, y + 2, light_color);
        }
    }

    /* Tiny specular highlight near the top-left */
    if (width > 8 && height > 6)
    {
        put_pixel(x + 2, y + 2, 15);
        put_pixel(x + 3, y + 2, 15);
        put_pixel(x + 2, y + 3, 15);
    }
}
