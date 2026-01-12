#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <time.h>

#include "audio.h"
#include "video.h"

#define BRICK_WIDTH 30
#define BRICK_HEIGHT 10
#define BRICK_ROWS 5
#define BRICK_COLS 10
#define BRICK_GAP 2
#define BRICK_PALETTE_START 32
#define BRICK_PALETTE_STRIDE 3
#define PADDLE_ACCEL 3
#define PADDLE_MAX_SPEED 8
#define PADDLE_FRICTION 1
#define MAX_LEVELS 10

static unsigned int level_layouts[MAX_LEVELS][BRICK_ROWS] =
{
    /* Each row uses 10 bits (bit j => column j). */
    /*  1 */ { 0x3FF, 0x3FF, 0x3FF, 0x3FF, 0x3FF },
    /*  2 */ { 0x3FF, 0x2AA, 0x155, 0x2AA, 0x3FF },
    /*  3 */ { 0x3FF, 0x201, 0x201, 0x201, 0x3FF },
    /*  4 */ { 0x155, 0x155, 0x155, 0x155, 0x155 },
    /*  5 */ { 0x303, 0x0CC, 0x030, 0x0CC, 0x303 },
    /*  6 */ { 0x030, 0x078, 0x0FC, 0x1FE, 0x3FF },
    /*  7 */ { 0x3CF, 0x3CF, 0x201, 0x3CF, 0x3CF },
    /*  8 */ { 0x01F, 0x03F, 0x07F, 0x0FF, 0x1FF },
    /*  9 */ { 0x2DB, 0x36D, 0x1B6, 0x36D, 0x2DB },
    /* 10 */ { 0x0FC, 0x3CF, 0x2AA, 0x3CF, 0x0FC }
};

Brick bricks[BRICK_ROWS][BRICK_COLS];
Ball ball;
Paddle paddle;
int score = 0;
int lives = 3;
int current_level = 1;
int ball_stuck = 1;
int launch_requested = 0;
int paused = 0;
int force_redraw = 0;

static int mouse_available = 0;
static int mouse_buttons = 0;
static int mouse_prev_buttons = 0;
static int mouse_x = 0;
static int mouse_y = 0;
static int key_vx = 0;
static int key_offset = 0;

static int mouse_init(void)
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

static void mouse_shutdown(void)
{
    union REGS regs;

    if (!mouse_available)
        return;

    regs.x.ax = 0x0001; /* Show cursor */
    int86(0x33, &regs, &regs);
}

static void mouse_update(void)
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

static void mouse_set_pos(int x, int y)
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

void drain_keyboard_buffer(void)
{
    while (kbhit())
        (void)getch();
}

void delay_with_audio(int ms)
{
    clock_t start;
    clock_t ticks;

    if (ms <= 0)
	return;

    start = clock();
    ticks = (clock_t)(((long)ms * (long)CLK_TCK + 999L) / 1000L);
    if (ticks <= 0)
        ticks = 1;

    while ((clock() - start) < ticks)
    {
        audio_update();
        delay(10);
    }
}

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
    key_vx = 0;
    key_offset = 0;

    /* Prevent a big "jump" when mouse control is active. */
    mouse_set_pos(paddle.x + paddle.width / 2, paddle.y);
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

void init_bricks(int level)
{
    int i, j;
    int start_y = 20;
    int total_width = (BRICK_COLS * BRICK_WIDTH) + ((BRICK_COLS - 1) * BRICK_GAP);
    int start_x = (SCREEN_WIDTH - total_width) / 2;
    int level_index = level - 1;
    unsigned int row_mask;

    if (start_x < 0)
        start_x = 0;

    if (level_index < 0 || level_index >= MAX_LEVELS)
        level_index = 0;

    for (i = 0; i < BRICK_ROWS; i++)
    {
        row_mask = level_layouts[level_index][i];

        for (j = 0; j < BRICK_COLS; j++)
        {
            bricks[i][j].x = start_x + j * (BRICK_WIDTH + BRICK_GAP);
            bricks[i][j].y = start_y + i * (BRICK_HEIGHT + 2);
            bricks[i][j].active = (row_mask & (1U << j)) ? 1 : 0;
            bricks[i][j].color = BRICK_PALETTE_START + i * BRICK_PALETTE_STRIDE;
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
    /* Avoid the restart key auto-repeat affecting the next game (e.g. toggling music). */
    drain_keyboard_buffer();

    score = 0;
    lives = 3;
    audio_music_restart();
    init_level(1);
}

void update_paddle()
{
    int move_dir = 0;
    int pause_toggle_requested = 0;
    int old_x = paddle.x;
    int use_mouse = mouse_available;
    int target_x = paddle.x;
    int left_clicked = 0;
    int right_clicked = 0;

    if (use_mouse)
    {
        mouse_update();
        left_clicked = ((mouse_buttons & 1) != 0) && ((mouse_prev_buttons & 1) == 0);
        right_clicked = ((mouse_buttons & 2) != 0) && ((mouse_prev_buttons & 2) == 0);
        mouse_prev_buttons = mouse_buttons;
        target_x = mouse_x - (paddle.width / 2);
    }

    /* Consume all pending keys; last direction wins. */
    while (kbhit())
    {
        char key = getch();
        if (key == 'p' || key == 'P')
        {
            pause_toggle_requested = 1;
            continue;
        }
        if (key == 's' || key == 'S')
        {
            audio_toggle();
            continue;
        }
        if (key == 'm' || key == 'M')
        {
            audio_music_toggle();
            continue;
        }
        if (key == ' ' && !paused)
        {
            launch_requested = 1;
            continue;
        }
        if (key == 0 || key == 0xE0)
        {
            char scan = getch();
            if (scan == 75)
            { // Left arrow
                move_dir = -1;
            }
            if (scan == 77)
            { // Right arrow
                move_dir = 1;
            }
            /* Turbo C / DOS variants: Pause can show up as an extended key. */
            if (scan == 0x45)
            {
                pause_toggle_requested = 1;
            }
        }
        if (key == 27)
        { // ESC
            mouse_shutdown();
            audio_shutdown();
            set_mode(0x03);
            exit(0);
        }
    }

    if (right_clicked)
        pause_toggle_requested = 1;

    if (pause_toggle_requested)
    {
        paused = !paused;
        force_redraw = 1;
        paddle.vx = 0;
        key_vx = 0;
        launch_requested = 0;
    }

    if (paused)
    {
        return;
    }

    if (left_clicked)
        launch_requested = 1;

    if (use_mouse)
    {
        if (move_dir < 0)
            key_vx -= PADDLE_ACCEL;
        else if (move_dir > 0)
            key_vx += PADDLE_ACCEL;
        else
        {
            if (key_vx > 0)
                key_vx -= PADDLE_FRICTION;
            else if (key_vx < 0)
                key_vx += PADDLE_FRICTION;
        }

        if (key_vx > PADDLE_MAX_SPEED)
            key_vx = PADDLE_MAX_SPEED;
        else if (key_vx < -PADDLE_MAX_SPEED)
            key_vx = -PADDLE_MAX_SPEED;

        key_offset += key_vx;
        if (key_offset > SCREEN_WIDTH)
            key_offset = SCREEN_WIDTH;
        else if (key_offset < -SCREEN_WIDTH)
            key_offset = -SCREEN_WIDTH;

        paddle.x = target_x;
        paddle.x += key_offset;
        if (paddle.x < 0)
            paddle.x = 0;
        if (paddle.x + paddle.width > SCREEN_WIDTH)
            paddle.x = SCREEN_WIDTH - paddle.width;

        paddle.vx = paddle.x - old_x;
        key_offset = paddle.x - target_x;
    }
    else
    {
        key_vx = 0;
        key_offset = 0;
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
    }

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

int check_brick_collision(int *hit_x, int *hit_y, int *hit_row)
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
                    if (hit_row)
                        *hit_row = i;
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
    int wall_hit = 0;
    int paddle_hit = 0;
    int brick_hit_row = 0;

    ball.x += ball.dx;
    ball.y += ball.dy;

    // Wall collision
    if (ball.x - radius <= 0 || ball.x + radius >= SCREEN_WIDTH)
    {
        ball.dx = -ball.dx;
        wall_hit = 1;
    }

    if (ball.y - radius <= 0)
    {
        ball.dy = -ball.dy;
        wall_hit = 1;
    }

    // Paddle collision
    if (ball.x + radius > paddle.x &&
        ball.x - radius < paddle.x + paddle.width &&
        ball.y + radius > paddle.y &&
        ball.y - radius < paddle.y + PADDLE_HEIGHT)
    {

        ball.dy = -ball.dy;
        ball.y = paddle.y - radius; // Place ball just above paddle
        paddle_hit = 1;

        // Add spin based on where ball hits paddle
        hit_pos = ball.x - paddle.x;
        ball.dx = (hit_pos - paddle.width / 2) / 5;
        if (ball.dx == 0)
            ball.dx = 1;
    }

    // Brick collision
    if (check_brick_collision(brick_hit_x, brick_hit_y, &brick_hit_row))
    {
        ball.dy = -ball.dy;
        brick_hit = 1;
    }

    if (brick_hit)
        audio_event_brick(brick_hit_row);
    else if (paddle_hit)
        audio_event_paddle();
    else if (wall_hit)
        audio_event_wall();

    // Ball lost
    if (ball.y >= SCREEN_HEIGHT)
    {
        lives--;
        audio_music_stop();
        audio_event_life_lost_blocking();
        if (lives > 0)
        {
            ball_stuck = 1;
            launch_requested = 0;
            reset_paddle();
            delay_with_audio(1000);
            audio_music_restart();
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


void intro_scene()
{
    char title[] = "CRUZKANOID";
    int text_width = 10 * 8; /* 10 chars * 8 pixels wide */
    int x = (SCREEN_WIDTH - text_width) / 2;
    int y = (SCREEN_HEIGHT - 7) / 2;     /* 7 is font height */
    unsigned char intro_border_index = 16;
    unsigned char intro_border_r = 0;
    unsigned char intro_border_g = 60;
    unsigned char intro_border_b = 63;
    int fade_steps = 30;
    int fade_delay_ms = 20;

    clear_screen(0);

    /* Draw once, then fade by changing the palette (no redraw). */
    set_palette_color(intro_border_index, 0, 0, 0);
    draw_bordered_text(x, y, title, intro_border_index);
    fade_palette_color(intro_border_index,
                       0, 0, 0,
                       intro_border_r, intro_border_g, intro_border_b,
                       fade_steps, fade_delay_ms);

    delay(2000); /* Hold for 2 seconds */

    fade_palette_color(intro_border_index,
                       intro_border_r, intro_border_g, intro_border_b,
                       0, 0, 0,
                       fade_steps, fade_delay_ms);
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
            if (first_frame || force_redraw)
            {
                clear_screen(0);
                draw_bricks();
                draw_paddle(paddle);
		draw_ball(ball);
		draw_ui(score, lives, current_level);
		if (paused)
		{
		    draw_pause_overlay();
		}
		first_frame = 0;
		force_redraw = 0;
	    }

	    /* Save old positions */
	    old_ball_x = ball.x;
	    old_ball_y = ball.y;
	    old_paddle_x = paddle.x;

	    /* Update game state */
	    update_paddle();
	    if (!paused)
	    {
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
	    }
	    else
	    {
		brick_was_hit = 0;
	    }

	    /* Erase old positions */
	    erase_ball(old_ball_x, old_ball_y, ball);
	    if (old_paddle_x != paddle.x)
	    {
		erase_paddle(old_paddle_x, paddle);
	    }

	    /* Erase destroyed brick */
	    if (brick_was_hit)
	    {
		draw_filled_rect(brick_hit_x, brick_hit_y, BRICK_WIDTH, BRICK_HEIGHT, 0);
	    }

	    /* Draw new positions */
	    draw_paddle(paddle);
	    draw_ball(ball);

	    /* Redraw UI (borders, score, lives) */
	    draw_ui(score, lives, current_level);

	    if (paused)
	    {
		draw_pause_overlay();
            }

            audio_update();
            delay(20);
        }

        if (lives == 0)
            break;

        if (current_level < MAX_LEVELS)
        {
            clear_screen(0);
            draw_text(105, 90, "LEVEL CLEAR!");
            audio_event_level_clear_blocking();
            delay_with_audio(1500);
            init_level(current_level + 1);
        }
        else
        {
            break;
        }
    }

    audio_music_stop();

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
    delay(2000);

    while (!kbhit())
    {
        audio_update();
        delay(20);
    }
    getch();
    drain_keyboard_buffer();
}

int main()
{
    set_mode(0x13); /* 320x200 256 color mode */
    init_brick_palette();
    init_paddle_palette();
    audio_init();
    mouse_init();

    intro_scene();

    while (1)
    {
        init_game();
        game_loop();
    }

    return 0;
}
