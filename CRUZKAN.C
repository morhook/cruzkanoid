#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <time.h>
#include <math.h>

#include "audio.h"
#include "DMAW.H"
#include "mouse.h"
#include "video.h"

#define BRICK_WIDTH 30
#define BRICK_HEIGHT 10
#define BRICK_ROWS 6
#define BRICK_COLS 10
#define BRICK_GAP 2
#define BRICK_PALETTE_START 32
#define BRICK_PALETTE_STRIDE 3
#define PADDLE_ACCEL 3
#define PADDLE_MAX_SPEED 8
#define PADDLE_FRICTION 1
#define MAX_LEVELS 10
#define LIFE_PILL_CHANCE 40
#define BALL_SPEED_INCREMENT 0.15f
#define BALL_SPEED_MAX 7.0f

static unsigned int level_layouts[MAX_LEVELS][BRICK_ROWS] =
    {
        /* Each row uses 10 bits (bit j => column j). */
        /*  1 */ {0x3FF, 0x2AA, 0x155, 0x2AA, 0x3FF, 0},
        /*  2 */ {0x3FF, 0x201, 0x201, 0x201, 0x3FF, 0},
        /*  3 */ {0x155, 0x155, 0x155, 0x155, 0x155, 0},
        /*  4 */ {0x303, 0x0CC, 0x030, 0x0CC, 0x303, 0},
        /*  5 */ {0x030, 0x078, 0x0FC, 0x1FE, 0x3FF, 0},
        /*  6 */ {0x3CF, 0x3CF, 0x201, 0x3CF, 0x3CF, 0},
        /*  7 */ {0x01F, 0x03F, 0x07F, 0x0FF, 0x1FF, 0},
        /*  8 */ {0x2DB, 0x36D, 0x1B6, 0x36D, 0x2DB, 0},
        /*  9 */ {0x0FC, 0x3CF, 0x2AA, 0x3CF, 0x0FC, 0},
        /* 10 */ {0x3FF, 0x3FF, 0x3FF, 0x3FF, 0x3FF, 0X3FF},
};

Brick bricks[BRICK_ROWS][BRICK_COLS];
Ball ball;
Paddle paddle;
Pill life_pill;
int score = 0;
int lives = 3;
int current_level = 1;
int ball_stuck = 1;
int launch_requested = 0;
int paused = 0;
int force_redraw = 0;
static int life_pill_spawn_request = 0;

static int key_vx = 0;
static int key_offset = 0;
static int rng_seeded = 0;

static void increase_ball_speed(void)
{
    float speed = (float)sqrt((double)(ball.dx * ball.dx + ball.dy * ball.dy));
    float new_speed = speed + BALL_SPEED_INCREMENT;

    if (new_speed > BALL_SPEED_MAX)
        new_speed = BALL_SPEED_MAX;

    if (speed > 0.0f)
    {
        float scale = new_speed / speed;
        ball.dx *= scale;
        ball.dy *= scale;
    }
}

void drain_keyboard_buffer(void)
{
    while (kbhit())
        (void)getch();
}

void reset_paddle()
{
    int radius = BALL_SIZE / 2;
    ball.x = paddle.x + paddle.width / 2;
    ball.y = paddle.y - radius - 1;
    ball.dx = 2.0f;
    ball.dy = -2.0f;

    paddle.x = SCREEN_WIDTH / 2 - PADDLE_WIDTH / 2;
    paddle.y = SCREEN_HEIGHT - 20;
    paddle.width = PADDLE_WIDTH;
    paddle.vx = 0;
    key_vx = 0;
    key_offset = 0;

    /* Prevent a big "jump" when mouse control is active. */
    mouse_set_pos(paddle.x + paddle.width / 2, paddle.y);
}

void reset_life_pill()
{
    life_pill.active = 0;
    life_pill.x = 0;
    life_pill.y = 0;
    life_pill.dy = PILL_FALL_SPEED;
    life_pill_spawn_request = 0;
}

void spawn_life_pill(int brick_x, int brick_y)
{
    if (life_pill.active)
        return;

    life_pill.active = 1;
    life_pill.x = brick_x + (BRICK_WIDTH / 2);
    life_pill.y = brick_y + (BRICK_HEIGHT / 2);
    life_pill.dy = PILL_FALL_SPEED;
}

void update_life_pill()
{
    int half_w = PILL_WIDTH / 2;
    int half_h = PILL_HEIGHT / 2;

    if (!life_pill.active)
        return;

    life_pill.y += life_pill.dy;

    if (life_pill.y - half_h > SCREEN_HEIGHT)
    {
        life_pill.active = 0;
        return;
    }

    if (life_pill.x + half_w > paddle.x &&
        life_pill.x - half_w < paddle.x + paddle.width &&
        life_pill.y + half_h > paddle.y &&
        life_pill.y - half_h < paddle.y + PADDLE_HEIGHT)
    {
        life_pill.active = 0;
        lives++;
        audio_event_life_up_blocking();
    }
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
    set_palette_color(PADDLE_PALETTE_START + 2, 0, 0, 63);   /* dark */
}

void init_pink_palette()
{
    /* Pink colors for heart background (indices 52-54). RGB values are 0-63. */
    set_palette_color(52, 63, 32, 48); /* Light pink */
    set_palette_color(53, 55, 15, 35); /* Medium pink */
    set_palette_color(54, 40, 8, 20);  /* Dark pink */
}

void init_pill_palette()
{
    /* Pill colors for life-up (indices 55-58). RGB values are 0-63. */
    set_palette_color(PILL_COLOR_LIGHT, 20, 48, 63);  /* highlight */
    set_palette_color(PILL_COLOR_BASE, 8, 22, 50);    /* base */
    set_palette_color(PILL_COLOR_BORDER, 2, 6, 20);   /* outline */
    set_palette_color(PILL_COLOR_GLYPH, 63, 63, 63);  /* glyph */
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
            bricks[i][j].hp = 1;
            bricks[i][j].gives_life = 0;
        }
    }
}

void init_level(int level)
{
    current_level = level;

    ball_stuck = 1;
    launch_requested = 0;
    reset_paddle();
    reset_life_pill();

    init_bricks(level);
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

void redraw_bricks_in_area(int x1, int y1, int x2, int y2)
{
    int i, j;
    int bx1, by1, bx2, by2;

    for (i = 0; i < BRICK_ROWS; i++)
    {
        for (j = 0; j < BRICK_COLS; j++)
        {
            if (!bricks[i][j].active)
                continue;

            bx1 = bricks[i][j].x;
            by1 = bricks[i][j].y;
            bx2 = bricks[i][j].x + BRICK_WIDTH - 1;
            by2 = bricks[i][j].y + BRICK_HEIGHT - 1;

            if (x1 <= bx2 && x2 >= bx1 && y1 <= by2 && y2 >= by1)
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
    reset_life_pill();
    init_level(1);
}

void update_game()
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
        if (key == 27 || key == 'q' || key == 'Q')
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

int check_brick_collision(float prev_ball_x, float prev_ball_y, int *hit_x, int *hit_y, int *hit_row, int *hit_axis, int *hit_destroyed, int *hit_life_up)
{
    int i, j;
    int radius = BALL_SIZE / 2;

    if (hit_destroyed)
        *hit_destroyed = 0;
    if (hit_life_up)
        *hit_life_up = 0;

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
                    int from_left = (prev_ball_x + radius <= bricks[i][j].x) &&
                                    (ball.x + radius > bricks[i][j].x);
                    int from_right = (prev_ball_x - radius >= bricks[i][j].x + BRICK_WIDTH) &&
                                     (ball.x - radius < bricks[i][j].x + BRICK_WIDTH);
                    int from_top = (prev_ball_y + radius <= bricks[i][j].y) &&
                                   (ball.y + radius > bricks[i][j].y);
                    int from_bottom = (prev_ball_y - radius >= bricks[i][j].y + BRICK_HEIGHT) &&
                                      (ball.y - radius < bricks[i][j].y + BRICK_HEIGHT);

                    if (hit_axis)
                    {
                        if (from_left || from_right)
                            *hit_axis = 1; /* horizontal: flip dx */
                        else if (from_top || from_bottom)
                            *hit_axis = 0; /* vertical: flip dy */
                        else
                        {
                            int overlap_left = (ball.x + radius) - bricks[i][j].x;
                            int overlap_right = (bricks[i][j].x + BRICK_WIDTH) - (ball.x - radius);
                            int overlap_top = (ball.y + radius) - bricks[i][j].y;
                            int overlap_bottom = (bricks[i][j].y + BRICK_HEIGHT) - (ball.y - radius);
                            int min_x = (overlap_left < overlap_right) ? overlap_left : overlap_right;
                            int min_y = (overlap_top < overlap_bottom) ? overlap_top : overlap_bottom;
                            *hit_axis = (min_x < min_y) ? 1 : 0;
                        }
                    }

                    {
                        if (bricks[i][j].hp > 0)
                            bricks[i][j].hp--;
                    }

                    if (bricks[i][j].hp <= 0)
                    {
                        bricks[i][j].active = 0;
                        score += 10;
                        if (hit_destroyed)
                            *hit_destroyed = 1;
                        if (bricks[i][j].gives_life)
                        {
                            bricks[i][j].gives_life = 0;
                            if (hit_life_up)
                                *hit_life_up = 1;
                        }
                        else
                        {
                            if ((rand() % LIFE_PILL_CHANCE) == 0)
                            {
                                if (hit_life_up)
                                    *hit_life_up = 1;
                            }
                        }
                    }
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
    int brick_collided = 0;
    int brick_life_up = 0;
    int radius = BALL_SIZE / 2;
    int min_x = radius + 1;
    int max_x = (SCREEN_WIDTH - 2) - radius;
    int min_y = radius + 1;
    float prev_ball_x = ball.x;
    float prev_ball_y = ball.y;
    int wall_hit = 0;
    int paddle_hit = 0;
    int brick_hit_row = 0;
    int brick_hit_axis = 0;
    int brick_destroyed = 0;

    ball.x += ball.dx;
    ball.y += ball.dy;

    // Wall collision
    if (ball.x < min_x)
    {
        ball.x = min_x;
        if (ball.dx < 0)
            ball.dx = -ball.dx;
        wall_hit = 1;
    }
    else if (ball.x > max_x)
    {
        ball.x = max_x;
        if (ball.dx > 0)
            ball.dx = -ball.dx;
        wall_hit = 1;
    }

    if (ball.y < min_y)
    {
        ball.y = min_y;
        if (ball.dy < 0)
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
        ball.dx = (float)(hit_pos - paddle.width / 2) / 5.0f;
        if (ball.dx == 0.0f)
            ball.dx = 1.0f;
    }

    // Brick collision
    if (check_brick_collision(prev_ball_x, prev_ball_y, brick_hit_x, brick_hit_y, &brick_hit_row, &brick_hit_axis, &brick_destroyed, &brick_life_up))
    {
        brick_collided = 1;
        if (brick_hit_axis)
        {
            ball.dx = -ball.dx;
            if (prev_ball_x < *brick_hit_x)
                ball.x = *brick_hit_x - radius;
            else if (prev_ball_x > *brick_hit_x + BRICK_WIDTH)
                ball.x = *brick_hit_x + BRICK_WIDTH + radius;
        }
        else
        {
            ball.dy = -ball.dy;
            if (prev_ball_y < *brick_hit_y)
                ball.y = *brick_hit_y - radius;
            else if (prev_ball_y > *brick_hit_y + BRICK_HEIGHT)
                ball.y = *brick_hit_y + BRICK_HEIGHT + radius;
        }
        brick_hit = brick_destroyed ? 1 : 0;
    }

    if (brick_life_up)
        life_pill_spawn_request = 1;

    if (brick_collided)
        audio_event_brick(brick_hit_row);
    else if (paddle_hit)
        audio_event_paddle();
    else if (wall_hit)
        audio_event_wall();

    if (brick_collided || paddle_hit || wall_hit)
        increase_ball_speed();

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

    if (brick_hit)
        return 1;
    return 0;
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
    struct WavFilePlay intro_wav;
    char title[] = "CRUZKANOID";
    int text_width = 10 * 8; /* 10 chars * 8 pixels wide */
    int x = (SCREEN_WIDTH - text_width) / 2;
    int y = (SCREEN_HEIGHT - 7) / 2; /* 7 is font height */
    unsigned char intro_border_index = 16;
    unsigned char intro_border_r = 0;
    unsigned char intro_border_g = 60;
    unsigned char intro_border_b = 63;
    int fade_steps = 30;
    int fade_delay_ms = 20;
    int wav_active = 0;
    int wav_done = 0;
    int i;
    unsigned char intro_master_volume = 0xC0;
    unsigned char intro_voice_volume = 0xC0;

    wav_set_mixer_volume(intro_master_volume, intro_voice_volume);
    play_wav("e2-pmute.wav");
    play_wav("e2-pmute.wav");
    play_wav("e2-pmute.wav");

    clear_screen(0);

    /* Draw once, then fade by changing the palette (no redraw). */
    set_palette_color(intro_border_index, 0, 0, 0);
    draw_bordered_text(x, y, title, intro_border_index);

    if (start_wav_file(&intro_wav, "e2.wav") != 0)
        wav_active = 1;

    for (i = 1; i <= fade_steps; i++)
    {
        int r = (int)(((long)intro_border_r * (long)i) / (long)fade_steps);
        int g = (int)(((long)intro_border_g * (long)i) / (long)fade_steps);
        int b = (int)(((long)intro_border_b * (long)i) / (long)fade_steps);

        set_palette_color(intro_border_index, (unsigned char)r, (unsigned char)g, (unsigned char)b);
        wait_vblank();
        if (fade_delay_ms > 0)
            delay(fade_delay_ms);
        if (wav_active && !wav_done)
            wav_done = update_wav_file(&intro_wav);
    }

    if (wav_active && !wav_done)
    {
        while (!wav_done)
        {
            int any_clicked = 0;

            wav_done = update_wav_file(&intro_wav);
            delay(10);

            if (mouse_available)
            {
                mouse_update();
                any_clicked = ((mouse_buttons & 3) != 0) && ((mouse_prev_buttons & 3) == 0);
                mouse_prev_buttons = mouse_buttons;
            }

            if (kbhit() || any_clicked)
            {
                if (kbhit())
                    getch();
                if (wav_active)
                {
                    stop_wav_file(&intro_wav);
                    wav_active = 0;
                }
                play_wav("e2-pmute.wav");
                break;
            }
        }
    } else { // no wav playing. wait just a little bit for PC speaker users
        delay(1000);
    }

    if (wav_active)
        stop_wav_file(&intro_wav);

    wav_set_mixer_volume(0xFF, 0xFF);

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
    int old_pill_x, old_pill_y;
    int old_pill_active;
    int spawned_this_frame;

    while (lives > 0)
    {
        first_frame = 1;
        while (lives > 0 && !check_win())
        {
            if (first_frame || force_redraw)
            {
                clear_screen(0);
                draw_background();
                draw_bricks();
                draw_paddle(paddle);
                draw_ball(ball);
                if (life_pill.active)
                    draw_pill(life_pill);
                draw_ui(score, lives, current_level);
                if (paused)
                {
                    draw_pause_overlay();
                }
                first_frame = 0;
                force_redraw = 0;
            }

            /* Save old positions */
            old_ball_x = (int)(ball.x + 0.5f);
            old_ball_y = (int)(ball.y + 0.5f);
            old_paddle_x = paddle.x;
            old_pill_x = life_pill.x;
            old_pill_y = life_pill.y;
            old_pill_active = life_pill.active;

            /* Update game state */
            update_game();
            if (!paused)
            {
                spawned_this_frame = 0;
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
                        ball.dx = (float)launch_dx;
                        ball.dy = -2.0f;
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
                    if (life_pill_spawn_request)
                    {
                        spawn_life_pill(brick_hit_x, brick_hit_y);
                        life_pill_spawn_request = 0;
                        spawned_this_frame = 1;
                    }
                }

                if (!spawned_this_frame)
                    update_life_pill();
            }
            else
            {
                brick_was_hit = 0;
            }

            wait_vblank();
            /* Erase old positions */
            erase_ball_with_background(old_ball_x, old_ball_y, ball);
            {
                int r = BALL_SIZE / 2;
                int x1 = old_ball_x - r - 1;
                int y1 = old_ball_y - r - 1;
                int x2 = old_ball_x + r + 1;
                int y2 = old_ball_y + r + 1;
                redraw_bricks_in_area(x1, y1, x2, y2);
            }
            if (old_paddle_x != paddle.x)
            {
                erase_paddle_with_background(old_paddle_x, paddle);
            }
            if (old_pill_active)
            {
                int x1 = old_pill_x - (PILL_WIDTH / 2) - 1;
                int y1 = old_pill_y - (PILL_HEIGHT / 2) - 1;
                int x2 = old_pill_x + (PILL_WIDTH / 2) + 1;
                int y2 = old_pill_y + (PILL_HEIGHT / 2) + 1;

                erase_pill_with_background(old_pill_x, old_pill_y);
                redraw_bricks_in_area(x1, y1, x2, y2);
            }

            /* Erase destroyed brick */
            if (brick_was_hit == 1)
            {
                erase_rect_with_background(brick_hit_x, brick_hit_y, BRICK_WIDTH, BRICK_HEIGHT);
            }
            else if (brick_was_hit == 2)
            {
                force_redraw = 1;
            }

            /* Draw new positions */
            draw_paddle(paddle);
            draw_ball(ball);
            if (life_pill.active)
                draw_pill(life_pill);

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
            draw_background();
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
    draw_background();
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
    init_pink_palette();
    init_pill_palette();
    audio_init();
    mouse_init();

    if (!rng_seeded)
    {
        srand((unsigned int)time(NULL));
        rng_seeded = 1;
    }

    intro_scene();

    while (1)
    {
        init_game();
        game_loop();
    }

    return 0;
}
