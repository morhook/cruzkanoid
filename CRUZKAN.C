#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <time.h>

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define PADDLE_WIDTH 40
#define PADDLE_HEIGHT 8
#define BALL_SIZE 4
#define BRICK_WIDTH 30
#define BRICK_HEIGHT 10
#define BRICK_ROWS 5
#define BRICK_COLS 10
#define BRICK_GAP 2
#define BRICK_PALETTE_START 32
#define BRICK_PALETTE_STRIDE 3
#define PADDLE_PALETTE_START 48
#define PADDLE_ACCEL 3
#define PADDLE_MAX_SPEED 8
#define PADDLE_FRICTION 1
#define MAX_LEVELS 2

typedef struct
{
    int x, y;
    int dx, dy;
} Ball;

typedef struct
{
    int x, y;
    int width;
    int vx;
} Paddle;

typedef struct
{
    int x, y;
    int active;
    int color;
} Brick;

unsigned char far *VGA = (unsigned char far *)0xA0000000L;
Brick bricks[BRICK_ROWS][BRICK_COLS];
Ball ball;
Paddle paddle;
int score = 0;
int lives = 3;
int current_level = 1;
int ball_stuck = 1;
int launch_requested = 0;

void reset_paddle()
{
    int radius = BALL_SIZE / 2;
    ball.x = paddle.x + paddle.width / 2;
    ball.y = paddle.y - radius - 1;
    ball.dx = 2;
    ball.dy = -2;

    paddle.x = SCREEN_WIDTH / 2 - PADDLE_WIDTH / 2;
    paddle.y = SCREEN_HEIGHT - 20;
    paddle.width = PADDLE_WIDTH;
    paddle.vx = 0;
}

void set_palette_color(unsigned char index, unsigned char r, unsigned char g, unsigned char b)
{
    outp(0x3C8, index);
    outp(0x3C9, r);
    outp(0x3C9, g);
    outp(0x3C9, b);
}

void init_brick_palette()
{
    /* Each row gets 3 shades: base, light, dark (RGB values are 0-63). */
    /* Row 0: red */
    set_palette_color(BRICK_PALETTE_START + 0 * BRICK_PALETTE_STRIDE + 0, 50, 5, 5);
    set_palette_color(BRICK_PALETTE_START + 0 * BRICK_PALETTE_STRIDE + 1, 63, 22, 18);
    set_palette_color(BRICK_PALETTE_START + 0 * BRICK_PALETTE_STRIDE + 2, 28, 2, 2);

    /* Row 1: green */
    set_palette_color(BRICK_PALETTE_START + 1 * BRICK_PALETTE_STRIDE + 0, 8, 50, 10);
    set_palette_color(BRICK_PALETTE_START + 1 * BRICK_PALETTE_STRIDE + 1, 28, 63, 26);
    set_palette_color(BRICK_PALETTE_START + 1 * BRICK_PALETTE_STRIDE + 2, 4, 28, 5);

    /* Row 2: magenta */
    set_palette_color(BRICK_PALETTE_START + 2 * BRICK_PALETTE_STRIDE + 0, 48, 10, 48);
    set_palette_color(BRICK_PALETTE_START + 2 * BRICK_PALETTE_STRIDE + 1, 63, 26, 63);
    set_palette_color(BRICK_PALETTE_START + 2 * BRICK_PALETTE_STRIDE + 2, 26, 6, 26);

    /* Row 3: yellow-green */
    set_palette_color(BRICK_PALETTE_START + 3 * BRICK_PALETTE_STRIDE + 0, 42, 52, 8);
    set_palette_color(BRICK_PALETTE_START + 3 * BRICK_PALETTE_STRIDE + 1, 60, 63, 24);
    set_palette_color(BRICK_PALETTE_START + 3 * BRICK_PALETTE_STRIDE + 2, 22, 28, 4);

    /* Row 4: blue */
    set_palette_color(BRICK_PALETTE_START + 4 * BRICK_PALETTE_STRIDE + 0, 10, 20, 55);
    set_palette_color(BRICK_PALETTE_START + 4 * BRICK_PALETTE_STRIDE + 1, 28, 40, 63);
    set_palette_color(BRICK_PALETTE_START + 4 * BRICK_PALETTE_STRIDE + 2, 5, 10, 30);
}

void init_paddle_palette()
{
    /* Steel/cyan paddle with highlight + shadow. */
    set_palette_color(PADDLE_PALETTE_START + 0, 18, 44, 52); /* base */
    set_palette_color(PADDLE_PALETTE_START + 1, 38, 60, 63); /* light */
    set_palette_color(PADDLE_PALETTE_START + 2, 8, 22, 28);  /* dark */
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

void init_bricks(int level)
{
    int i, j;
    int start_y = 20;
    int total_width = (BRICK_COLS * BRICK_WIDTH) + ((BRICK_COLS - 1) * BRICK_GAP);
    int start_x = (SCREEN_WIDTH - total_width) / 2;

    if (start_x < 0)
        start_x = 0;

    for (i = 0; i < BRICK_ROWS; i++)
    {
        for (j = 0; j < BRICK_COLS; j++)
        {
            bricks[i][j].x = start_x + j * (BRICK_WIDTH + BRICK_GAP);
            bricks[i][j].y = start_y + i * (BRICK_HEIGHT + 2);
            bricks[i][j].active = 0;
            bricks[i][j].color = BRICK_PALETTE_START + i * BRICK_PALETTE_STRIDE;

            if (level == 1)
            {
                bricks[i][j].active = 1;
            }
            else if (level == 2)
            {
                /* Checkerboard-ish layout (fewer bricks than level 1). */
                if (i == 0 || i == BRICK_ROWS - 1)
                    bricks[i][j].active = 1;
                else if (((i + j) & 1) == 0)
                    bricks[i][j].active = 1;
            }
            else
            {
                bricks[i][j].active = 1;
            }
        }
    }
}

void init_level(int level)
{
    current_level = level;

    ball_stuck = 1;
    launch_requested = 0;
    reset_paddle();

    init_bricks(level);
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

void draw_bricks()
{
    int i, j;
    for (i = 0; i < BRICK_ROWS; i++)
    {
        for (j = 0; j < BRICK_COLS; j++)
        {
            if (bricks[i][j].active)
            {
                draw_brick(bricks[i][j].x, bricks[i][j].y, BRICK_WIDTH, BRICK_HEIGHT, bricks[i][j].color);
            }
        }
    }
}

void init_game()
{
    score = 0;
    lives = 3;
    init_level(1);
}

void draw_paddle()
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

void erase_paddle(int x)
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

void draw_ball()
{
    draw_filled_circle(ball.x, ball.y, BALL_SIZE / 2, 0x64, 0x3f);
}

void erase_ball(int x, int y)
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

void update_paddle()
{
    int move_dir = 0;

    /* Consume all pending keys; last direction wins. */
    while (kbhit())
    {
        char key = getch();
        if (key == ' ')
        {
            launch_requested = 1;
            continue;
        }
        if (key == 0 || key == 0xE0)
        {
            key = getch();
            if (key == 75)
            { // Left arrow
                move_dir = -1;
            }
            if (key == 77)
            { // Right arrow
                move_dir = 1;
            }
        }
        if (key == 27)
        { // ESC
            set_mode(0x03);
            exit(0);
        }
    }

    if (move_dir < 0)
        paddle.vx -= PADDLE_ACCEL;
    else if (move_dir > 0)
        paddle.vx += PADDLE_ACCEL;
    else
    {
        /* Friction toward stop when no input this frame. */
        if (paddle.vx > 0)
            paddle.vx -= PADDLE_FRICTION;
        else if (paddle.vx < 0)
            paddle.vx += PADDLE_FRICTION;
    }

    if (paddle.vx > PADDLE_MAX_SPEED)
        paddle.vx = PADDLE_MAX_SPEED;
    else if (paddle.vx < -PADDLE_MAX_SPEED)
        paddle.vx = -PADDLE_MAX_SPEED;

    paddle.x += paddle.vx;

    if (paddle.x < 0)
    {
        paddle.x = 0;
        paddle.vx = 0;
    }
    if (paddle.x + paddle.width > SCREEN_WIDTH)
    {
        paddle.x = SCREEN_WIDTH - paddle.width;
        paddle.vx = 0;
    }
}

int check_brick_collision(int *hit_x, int *hit_y)
{
    int i, j;
    int radius = BALL_SIZE / 2;

    for (i = 0; i < BRICK_ROWS; i++)
    {
        for (j = 0; j < BRICK_COLS; j++)
        {
            if (bricks[i][j].active)
            {
                if (ball.x + radius > bricks[i][j].x &&
                    ball.x - radius < bricks[i][j].x + BRICK_WIDTH &&
                    ball.y + radius > bricks[i][j].y &&
                    ball.y - radius < bricks[i][j].y + BRICK_HEIGHT)
                {

                    bricks[i][j].active = 0;
                    score += 10;
                    *hit_x = bricks[i][j].x;
                    *hit_y = bricks[i][j].y;
                    return 1;
                }
            }
        }
    }
    return 0;
}

int update_ball(int *brick_hit_x, int *brick_hit_y)
{
    int hit_pos;
    int brick_hit = 0;
    int radius = BALL_SIZE / 2;

    ball.x += ball.dx;
    ball.y += ball.dy;

    // Wall collision
    if (ball.x - radius <= 0 || ball.x + radius >= SCREEN_WIDTH)
    {
        ball.dx = -ball.dx;
    }

    if (ball.y - radius <= 0)
    {
        ball.dy = -ball.dy;
    }

    // Paddle collision
    if (ball.x + radius > paddle.x &&
        ball.x - radius < paddle.x + paddle.width &&
        ball.y + radius > paddle.y &&
        ball.y - radius < paddle.y + PADDLE_HEIGHT)
    {

        ball.dy = -ball.dy;
        ball.y = paddle.y - radius; // Place ball just above paddle

        // Add spin based on where ball hits paddle
        hit_pos = ball.x - paddle.x;
        ball.dx = (hit_pos - paddle.width / 2) / 5;
        if (ball.dx == 0)
            ball.dx = 1;
    }

    // Brick collision
    if (check_brick_collision(brick_hit_x, brick_hit_y))
    {
        ball.dy = -ball.dy;
        brick_hit = 1;
    }

    // Ball lost
    if (ball.y >= SCREEN_HEIGHT)
    {
        lives--;
        if (lives > 0)
        {
            ball_stuck = 1;
            launch_requested = 0;
            reset_paddle();
            delay(1000);
        }
    }

    return brick_hit;
}

int check_win()
{
    int i, j;
    for (i = 0; i < BRICK_ROWS; i++)
    {
        for (j = 0; j < BRICK_COLS; j++)
        {
            if (bricks[i][j].active)
            {
                return 0;
            }
        }
    }
    return 1;
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

void draw_ui()
{
    char buffer[50];

    // Draw border
    draw_rect(0, 0, SCREEN_WIDTH, 1, 15);
    draw_rect(0, 0, 1, SCREEN_HEIGHT, 15);
    draw_rect(SCREEN_WIDTH - 1, 0, 1, SCREEN_HEIGHT, 15);

    // Score and lives at top (pixel coordinates now)
    sprintf(buffer, "Score: %d", score);
    draw_text(5, 5, buffer);

    sprintf(buffer, "Level: %d", current_level);
    draw_text(120, 5, buffer);

    sprintf(buffer, "Lives: %d", lives);
    draw_text(200, 5, buffer);
}

void draw_char_transparent(int x, int y, char c, unsigned char color)
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

void draw_text_transparent(int x, int y, char *text, unsigned char color)
{
    int i = 0;
    while (text[i] != '\0')
    {
        draw_char_transparent(x + i * 8, y, text[i], color);
        i++;
    }
}

void draw_bordered_text(int x, int y, char *text, unsigned char border_color)
{
    // Draw border
    draw_text_transparent(x - 1, y, text, border_color);
    draw_text_transparent(x + 1, y, text, border_color);
    draw_text_transparent(x, y - 1, text, border_color);
    draw_text_transparent(x, y + 1, text, border_color);

    // Draw black inner text
    draw_text_transparent(x, y, text, 0);
}

void intro_scene()
{
    char title[] = "CRUZKANOID";
    int text_width = 10 * 8; /* 10 chars * 8 pixels wide */
    int x = (SCREEN_WIDTH - text_width) / 2;
    int y = (SCREEN_HEIGHT - 7) / 2;     /* 7 is font height */
    unsigned char colors[] = {0, 3, 11}; /* Black, Dark Cyan, Light Cyan */
    int i;

    clear_screen(0);

    /* Fade in */
    for (i = 0; i < 3; i++)
    {
        draw_bordered_text(x, y, title, colors[i]);
        delay(200);
    }

    delay(2000); /* Hold for 2 seconds */

    /* Fade out */
    for (i = 2; i >= 0; i--)
    {
        draw_bordered_text(x, y, title, colors[i]);
        delay(200);
    }
    clear_screen(0);
}

void game_loop()
{
    char buffer[50];
    int old_ball_x, old_ball_y;
    int old_paddle_x;
    int first_frame;
    int brick_hit_x, brick_hit_y;
    int brick_was_hit;
    int radius = BALL_SIZE / 2;
    int launch_dx;

    while (lives > 0)
    {
        first_frame = 1;
        while (lives > 0 && !check_win())
        {
            if (first_frame)
            {
                clear_screen(0);
                draw_bricks();
                draw_paddle();
                draw_ball();
                draw_ui();
                first_frame = 0;
            }

            /* Save old positions */
            old_ball_x = ball.x;
            old_ball_y = ball.y;
            old_paddle_x = paddle.x;

            /* Update game state */
            update_paddle();
            if (ball_stuck)
            {
                if (launch_requested)
                {
                    launch_dx = paddle.vx / 2;
                    if (launch_dx > 3)
                        launch_dx = 3;
                    else if (launch_dx < -3)
                        launch_dx = -3;
                    if (launch_dx == 0)
                        launch_dx = 2;

                    ball_stuck = 0;
                    launch_requested = 0;
                    ball.dx = launch_dx;
                    ball.dy = -2;
                }
                else
                {
                    ball.x = paddle.x + paddle.width / 2;
                    ball.y = paddle.y - radius - 1;
                }
                brick_was_hit = 0;
            }
            else
            {
                brick_was_hit = update_ball(&brick_hit_x, &brick_hit_y);
            }

            /* Erase old positions */
            erase_ball(old_ball_x, old_ball_y);
            if (old_paddle_x != paddle.x)
            {
                erase_paddle(old_paddle_x);
            }

            /* Erase destroyed brick */
            if (brick_was_hit)
            {
                draw_filled_rect(brick_hit_x, brick_hit_y, BRICK_WIDTH, BRICK_HEIGHT, 0);
            }

            /* Draw new positions */
            draw_paddle();
            draw_ball();

            /* Redraw UI (borders, score, lives) */
            draw_ui();

            delay(20);
        }

        if (lives == 0)
            break;

        if (current_level < MAX_LEVELS)
        {
            clear_screen(0);
            draw_text(105, 90, "LEVEL CLEAR!");
            delay(1500);
            init_level(current_level + 1);
        }
        else
        {
            break;
        }
    }

    clear_screen(0);
    if (lives == 0)
    {
        draw_text(110, 90, "GAME OVER!");
    }
    else
    {
        draw_text(120, 90, "YOU WIN!");
    }

    sprintf(buffer, "Final Score: %d", score);
    draw_text(90, 105, buffer);
    draw_text(70, 120, "Press any key to restart");

    delay(10000);
    getch();
}

int main()
{
    set_mode(0x13); /* 320x200 256 color mode */
    init_brick_palette();
    init_paddle_palette();

    intro_scene();

    while (1)
    {
        init_game();
        game_loop();
    }

    return 0;
}
