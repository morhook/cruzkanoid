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
#define BRICK_ROWS 8
#define BRICK_COLS 10
#define BRICK_GAP 2
#define BRICK_PALETTE_START 32
#define BRICK_PALETTE_STRIDE 3
#define PADDLE_ACCEL 3
#define PADDLE_MAX_SPEED 8
#define PADDLE_FRICTION 1
#define MAX_LEVELS 15
#define LIFE_POWERUP_CHANCE 2
#define BALL_SPEED_INCREMENT 0.15f
#define BALL_SPEED_MAX 7.0f
#define LASER_SHOT_SPEED 6
#define LASER_FIRE_COOLDOWN_FRAMES 6
#define MAX_BRICKS_DESTROYED_PER_FRAME (MAX_BALLS + MAX_LASER_SHOTS)

static unsigned int level_layouts[MAX_LEVELS][BRICK_ROWS] =
{
    /* Each row uses 10 bits (bit j => column j). 8 rows per level. */

    /*  1: Bordered diamond - classic opener */
    {0x3FF, 0x2AA, 0x155, 0x2AA, 0x155, 0x2AA, 0x3FF, 0},

    /*  2: Hollow rectangle */
    {0x3FF, 0x201, 0x201, 0x201, 0x201, 0x201, 0x3FF, 0},

    /*  3: Alternating stripes */
    {0x155, 0x2AA, 0x155, 0x2AA, 0x155, 0x2AA, 0x155, 0},

    /*  4: Diamond cross */
    {0x084, 0x1CE, 0x3FF, 0x1CE, 0x084, 0x1CE, 0x3FF, 0},

    /*  5: Staircase left-to-right */
    {0x001, 0x003, 0x007, 0x00F, 0x01F, 0x03F, 0x07F, 0x0FF},

    /*  6: X pattern */
    {0x201, 0x102, 0x084, 0x048, 0x048, 0x084, 0x102, 0x201},

    /*  7: Plus / cross */
    {0x040, 0x040, 0x040, 0x3FF, 0x3FF, 0x040, 0x040, 0x040},

    /*  8: Checkerboard dense */
    {0x2DB, 0x36D, 0x1B6, 0x2DB, 0x36D, 0x1B6, 0x2DB, 0x36D},

    /*  9: V-shape / arrowhead pointing up */
    {0x201, 0x102, 0x084, 0x048, 0x030, 0x048, 0x084, 0x102},

    /* 10: Hourglass */
    {0x3FF, 0x1FE, 0x0FC, 0x078, 0x078, 0x0FC, 0x1FE, 0x3FF},

    /* 11: Spiral inward */
    {0x3FF, 0x201, 0x27F, 0x241, 0x25F, 0x250, 0x270, 0x3FF},

    /* 12: Two separate boxes */
    {0x1E0, 0x120, 0x120, 0x1E0, 0x01E, 0x012, 0x012, 0x01E},

    /* 13: Castle battlements */
    {0x3FF, 0x249, 0x3FF, 0x201, 0x201, 0x3FF, 0x249, 0x3FF},

    /* 14: Random scatter / dense noise */
    {0x2D6, 0x16B, 0x3AD, 0x1DA, 0x2B5, 0x16A, 0x3D6, 0x2AB},

    /* 15: Final - all bricks */
    {0x3FF, 0x3FF, 0x3FF, 0x3FF, 0x3FF, 0x3FF, 0x3FF, 0x3FF},
};

Brick bricks[BRICK_ROWS][BRICK_COLS];
Ball balls[MAX_BALLS];
int ball_active[MAX_BALLS];
Paddle paddle;
Pill life_pill;
LaserShot laser_shots[MAX_LASER_SHOTS];
int score = 0;
int lives = 3;
int current_level = 1;
int ball_stuck = 1;
int launch_requested = 0;
int paused = 0;
int force_redraw = 0;
static int life_pill_spawn_request = 0;
static int life_pill_spawn_type = PILL_TYPE_NONE;
static int life_pill_spawn_x = 0;
static int life_pill_spawn_y = 0;
static int laser_fire_cooldown = 0;
static int brick_field_x1 = 0;
static int brick_field_y1 = 0;
static int brick_field_x2 = 0;
static int brick_field_y2 = 0;

static int key_vx = 0;
static int key_offset = 0;
static int rng_seeded = 0;

static int count_active_balls(void)
{
    int i;
    int count = 0;

    for (i = 0; i < MAX_BALLS; i++)
    {
        if (ball_active[i])
            count++;
    }

    return count;
}

static int find_free_ball_slot(void)
{
    int i;

    for (i = 0; i < MAX_BALLS; i++)
    {
        if (!ball_active[i])
            return i;
    }

    return -1;
}

static int find_first_active_ball(void)
{
    int i;

    for (i = 0; i < MAX_BALLS; i++)
    {
        if (ball_active[i])
            return i;
    }

    return -1;
}

static void reset_balls(void)
{
    int i;
    int radius = BALL_SIZE / 2;

    for (i = 0; i < MAX_BALLS; i++)
    {
        ball_active[i] = 0;
        balls[i].x = 0.0f;
        balls[i].y = 0.0f;
        balls[i].dx = 0.0f;
        balls[i].dy = 0.0f;
    }

    ball_active[0] = 1;
    balls[0].x = paddle.x + paddle.width / 2;
    balls[0].y = paddle.y - radius - 1;
    balls[0].dx = 2.0f;
    balls[0].dy = -2.0f;
}

static int spawn_ball(float x, float y, float dx, float dy)
{
    int slot = find_free_ball_slot();

    if (slot < 0)
        return 0;

    ball_active[slot] = 1;
    balls[slot].x = x;
    balls[slot].y = y;
    balls[slot].dx = dx;
    balls[slot].dy = dy;

    return 1;
}

static void spawn_multiball_bonus(void)
{
    int source_index = find_first_active_ball();
    int i;
    static const float spread_dx[5] = {-3.0f, -1.5f, 0.0f, 1.5f, 3.0f};
    float base_x;
    float base_y;
    float base_dy;

    if (source_index < 0)
    {
        source_index = 0;
        ball_active[0] = 1;
        balls[0].x = paddle.x + paddle.width / 2;
        balls[0].y = paddle.y - (BALL_SIZE / 2) - 1;
        balls[0].dx = 2.0f;
        balls[0].dy = -2.0f;
    }

    base_x = balls[source_index].x;
    base_y = balls[source_index].y;
    base_dy = balls[source_index].dy;
    if (base_dy > -1.0f && base_dy < 1.0f)
        base_dy = -2.5f;

    for (i = 0; i < 5; i++)
    {
        float dx = spread_dx[i];
        float dy = (base_dy < 0.0f) ? (-2.5f - (float)i * 0.1f) : (2.5f + (float)i * 0.1f);

        if (dx == 0.0f)
            dx = (balls[source_index].dx >= 0.0f) ? 1.0f : -1.0f;

        if (!spawn_ball(base_x, base_y, dx, dy))
            break;
    }
}

static void reset_laser_shots(void)
{
    int i;

    for (i = 0; i < MAX_LASER_SHOTS; i++)
    {
        laser_shots[i].active = 0;
        laser_shots[i].x = 0;
        laser_shots[i].y = 0;
        laser_shots[i].dy = -LASER_SHOT_SPEED;
    }

    laser_fire_cooldown = 0;
}

static int fire_laser_shot_pair(void)
{
    int i;
    int first = -1;
    int second = -1;
    int shot_y;
    int left_x;
    int right_x;

    if (!paddle.laser_enabled || laser_fire_cooldown > 0)
        return 0;

    for (i = 0; i < MAX_LASER_SHOTS; i++)
    {
        if (!laser_shots[i].active)
        {
            if (first < 0)
                first = i;
            else
            {
                second = i;
                break;
            }
        }
    }

    if (first < 0 || second < 0)
        return 0;

    shot_y = paddle.y - 1;
    left_x = paddle.x + 2;
    right_x = paddle.x + paddle.width - 3;

    if (left_x < 1)
        left_x = 1;
    if (right_x > SCREEN_WIDTH - 2)
        right_x = SCREEN_WIDTH - 2;

    laser_shots[first].active = 1;
    laser_shots[first].x = left_x;
    laser_shots[first].y = shot_y;
    laser_shots[first].dy = -LASER_SHOT_SPEED;

    laser_shots[second].active = 1;
    laser_shots[second].x = right_x;
    laser_shots[second].y = shot_y;
    laser_shots[second].dy = -LASER_SHOT_SPEED;

    laser_fire_cooldown = LASER_FIRE_COOLDOWN_FRAMES;
    return 1;
}

static void record_destroyed_brick(int *xs, int *ys, int *count, int x, int y)
{
    if (*count >= MAX_BRICKS_DESTROYED_PER_FRAME)
        return;

    xs[*count] = x;
    ys[*count] = y;
    (*count)++;
}

static void update_laser_shots(int *destroyed_xs, int *destroyed_ys, int *destroyed_count)
{
    int i;
    int row;
    int col;

    for (i = 0; i < MAX_LASER_SHOTS; i++)
    {
        if (!laser_shots[i].active)
            continue;

        laser_shots[i].y += laser_shots[i].dy;
        if (laser_shots[i].y < 2)
        {
            laser_shots[i].active = 0;
            continue;
        }

        for (row = 0; row < BRICK_ROWS; row++)
        {
            for (col = 0; col < BRICK_COLS; col++)
            {
                if (!bricks[row][col].active)
                    continue;

                if (laser_shots[i].x >= bricks[row][col].x &&
                    laser_shots[i].x < bricks[row][col].x + BRICK_WIDTH &&
                    laser_shots[i].y >= bricks[row][col].y &&
                    laser_shots[i].y < bricks[row][col].y + BRICK_HEIGHT)
                {
                    int hit_pill_type = PILL_TYPE_NONE;

                    if (bricks[row][col].hp > 0)
                        bricks[row][col].hp--;

                    if (bricks[row][col].hp <= 0)
                    {
                        bricks[row][col].active = 0;
                        score += 10;
                        record_destroyed_brick(destroyed_xs, destroyed_ys, destroyed_count,
                                              bricks[row][col].x, bricks[row][col].y);

                        if (bricks[row][col].gives_life)
                        {
                            bricks[row][col].gives_life = 0;
                            hit_pill_type = PILL_TYPE_LIFE;
                        }
                        else if ((rand() % LIFE_POWERUP_CHANCE) == 0)
                        {
                            int roll = rand() % 5;
                            if (roll == 0)
                                hit_pill_type = PILL_TYPE_LIFE;
                            else if (roll == 1)
                                hit_pill_type = PILL_TYPE_GROW;
                            else if (roll == 2)
                                hit_pill_type = PILL_TYPE_SHRINK;
                            else if (roll == 3)
                                hit_pill_type = PILL_TYPE_MULTIBALL;
                            else
                                hit_pill_type = PILL_TYPE_LASER;
                        }

                        if (hit_pill_type != PILL_TYPE_NONE)
                        {
                            life_pill_spawn_request = 1;
                            life_pill_spawn_type = hit_pill_type;
                            life_pill_spawn_x = bricks[row][col].x;
                            life_pill_spawn_y = bricks[row][col].y;
                        }
                    }

                    laser_shots[i].active = 0;
                    audio_event_brick(row);
                    row = BRICK_ROWS;
                    break;
                }
            }
        }
    }
}

static void increase_ball_speed(Ball *ball)
{
    float speed = (float)sqrt((double)(ball->dx * ball->dx + ball->dy * ball->dy));
    float new_speed = speed + BALL_SPEED_INCREMENT;

    if (new_speed > BALL_SPEED_MAX)
        new_speed = BALL_SPEED_MAX;

    if (speed > 0.0f)
    {
        float scale = new_speed / speed;
        ball->dx *= scale;
        ball->dy *= scale;
    }
}

void drain_keyboard_buffer(void)
{
    while (kbhit())
        (void)getch();
}

void reset_paddle()
{
    paddle.x = SCREEN_WIDTH / 2 - PADDLE_WIDTH / 2;
    paddle.y = SCREEN_HEIGHT - 20;
    paddle.width = PADDLE_WIDTH;
    paddle.vx = 0;
    paddle.laser_enabled = 0;
    key_vx = 0;
    key_offset = 0;

    reset_laser_shots();
    reset_balls();

    /* Prevent a big "jump" when mouse control is active. */
    mouse_set_pos(paddle.x + paddle.width / 2, paddle.y);
}

void reset_life_pill()
{
    life_pill.active = 0;
    life_pill.x = 0;
    life_pill.y = 0;
    life_pill.dy = PILL_FALL_SPEED;
    life_pill.type = PILL_TYPE_NONE;
    life_pill_spawn_request = 0;
    life_pill_spawn_type = PILL_TYPE_NONE;
    life_pill_spawn_x = 0;
    life_pill_spawn_y = 0;
}

void spawn_life_pill(int brick_x, int brick_y, int pill_type)
{
    if (life_pill.active)
        return;

    life_pill.active = 1;
    life_pill.x = brick_x + (BRICK_WIDTH / 2);
    life_pill.y = brick_y + (BRICK_HEIGHT / 2);
    life_pill.dy = PILL_FALL_SPEED;
    life_pill.type = pill_type;
}

void update_life_pill()
{
    int half_w = PILL_WIDTH / 2;
    int half_h = PILL_HEIGHT / 2;
    int prev_paddle_x;
    int prev_paddle_width;
    int paddle_reposition_required;

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
        prev_paddle_x = paddle.x;
        prev_paddle_width = paddle.width;
        if (life_pill.type == PILL_TYPE_LIFE)
        {
            lives++;
            audio_event_life_up_blocking();
        }
        else if (life_pill.type == PILL_TYPE_GROW)
        {
            paddle.width = PADDLE_WIDTH_GROW;
        }
        else if (life_pill.type == PILL_TYPE_SHRINK)
        {
            paddle.width = PADDLE_WIDTH_SHRINK;
        }
        else if (life_pill.type == PILL_TYPE_MULTIBALL)
        {
            audio_event_multiball();
            spawn_multiball_bonus();
        }
        else if (life_pill.type == PILL_TYPE_LASER)
        {
            paddle.laser_enabled = 1;
            laser_fire_cooldown = 0;
        }

        if (paddle.width < 8)
            paddle.width = 8;
        if (paddle.width > SCREEN_WIDTH - 2)
            paddle.width = SCREEN_WIDTH - 2;

        if (paddle.x < 0)
            paddle.x = 0;
        if (paddle.x + paddle.width > SCREEN_WIDTH)
            paddle.x = SCREEN_WIDTH - paddle.width;
        paddle_reposition_required = (paddle.x != prev_paddle_x) ||
                                    (paddle.width != prev_paddle_width);
        if (paddle_reposition_required && mouse_available)
            mouse_set_pos(paddle.x + paddle.width / 2, paddle.y);
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

    /* Row 5: orange */
    set_palette_color(BRICK_PALETTE_START + 5 * BRICK_PALETTE_STRIDE + 0, 55, 30, 0);
    set_palette_color(BRICK_PALETTE_START + 5 * BRICK_PALETTE_STRIDE + 1, 63, 48, 14);
    set_palette_color(BRICK_PALETTE_START + 5 * BRICK_PALETTE_STRIDE + 2, 30, 14, 0);

    /* Row 6: cyan */
    set_palette_color(BRICK_PALETTE_START + 6 * BRICK_PALETTE_STRIDE + 0, 5, 50, 50);
    set_palette_color(BRICK_PALETTE_START + 6 * BRICK_PALETTE_STRIDE + 1, 20, 63, 63);
    set_palette_color(BRICK_PALETTE_START + 6 * BRICK_PALETTE_STRIDE + 2, 2, 25, 25);

    /* Row 7: purple */
    set_palette_color(BRICK_PALETTE_START + 7 * BRICK_PALETTE_STRIDE + 0, 38, 10, 55);
    set_palette_color(BRICK_PALETTE_START + 7 * BRICK_PALETTE_STRIDE + 1, 52, 28, 63);
    set_palette_color(BRICK_PALETTE_START + 7 * BRICK_PALETTE_STRIDE + 2, 18, 4, 28);
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
    /* Pink colors for heart background (indices 59-62). RGB values are 0-63.
       Moved from 52-55 to avoid collision with brick row 5-7 palette (47-55). */
    set_palette_color(59, 63, 32, 48); /* Light pink */
    set_palette_color(60, 55, 15, 35); /* Medium pink */
    set_palette_color(61, 40, 8, 20);  /* Dark pink */
    set_palette_color(62, 63, 63, 63); /* White */
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
    int total_height = (BRICK_ROWS * BRICK_HEIGHT) + ((BRICK_ROWS - 1) * 2);
    int start_x = (SCREEN_WIDTH - total_width) / 2;
    int level_index = level - 1;
    unsigned int row_mask;

    if (start_x < 0)
        start_x = 0;

    if (level_index < 0 || level_index >= MAX_LEVELS)
        level_index = 0;

    brick_field_x1 = start_x;
    brick_field_y1 = start_y;
    brick_field_x2 = start_x + total_width - 1;
    brick_field_y2 = start_y + total_height - 1;

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
    int track;
    current_level = level;

    /* Each level gets its own music track (tracks 0-14). */
    track = (level >= 1 && level <= MAX_LEVELS) ? (level - 1) : 0;
    audio_music_set_track(track);
    audio_music_restart();

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
    int first_row;
    int last_row;
    int first_col;
    int last_col;
    int row_pitch = BRICK_HEIGHT + 2;
    int col_pitch = BRICK_WIDTH + BRICK_GAP;
    int bx1, by1, bx2, by2;

    if (x2 < brick_field_x1 || x1 > brick_field_x2 ||
        y2 < brick_field_y1 || y1 > brick_field_y2)
    {
        return;
    }

    if (y1 <= brick_field_y1)
        first_row = 0;
    else
        first_row = (y1 - brick_field_y1) / row_pitch;

    if (y2 >= brick_field_y2)
        last_row = BRICK_ROWS - 1;
    else
        last_row = (y2 - brick_field_y1) / row_pitch;

    if (x1 <= brick_field_x1)
        first_col = 0;
    else
        first_col = (x1 - brick_field_x1) / col_pitch;

    if (x2 >= brick_field_x2)
        last_col = BRICK_COLS - 1;
    else
        last_col = (x2 - brick_field_x1) / col_pitch;

    if (first_row < 0)
        first_row = 0;
    if (last_row >= BRICK_ROWS)
        last_row = BRICK_ROWS - 1;
    if (first_col < 0)
        first_col = 0;
    if (last_col >= BRICK_COLS)
        last_col = BRICK_COLS - 1;

    for (i = first_row; i <= last_row; i++)
    {
        for (j = first_col; j <= last_col; j++)
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

void get_inputs()
{
    int move_dir = 0;
    int pause_toggle_requested = 0;
    int fire_requested = 0;
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
            fire_requested = 1;
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
        fire_requested = 1;

    if (laser_fire_cooldown > 0)
        laser_fire_cooldown--;

    if (fire_requested)
    {
        if (ball_stuck)
            launch_requested = 1;
        else if (paddle.laser_enabled)
            fire_laser_shot_pair();
    }

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

int check_brick_collision(Ball *ball, float prev_ball_x, float prev_ball_y, int *hit_x, int *hit_y, int *hit_row, int *hit_axis, int *hit_destroyed, int *hit_pill_type)
{
    int i, j;
    int radius = BALL_SIZE / 2;

    if (hit_destroyed)
        *hit_destroyed = 0;
    if (hit_pill_type)
        *hit_pill_type = PILL_TYPE_NONE;

    for (i = 0; i < BRICK_ROWS; i++)
    {
        for (j = 0; j < BRICK_COLS; j++)
        {
            if (bricks[i][j].active)
            {
                if (ball->x + radius > bricks[i][j].x &&
                    ball->x - radius < bricks[i][j].x + BRICK_WIDTH &&
                    ball->y + radius > bricks[i][j].y &&
                    ball->y - radius < bricks[i][j].y + BRICK_HEIGHT)
                {
                    int from_left = (prev_ball_x + radius <= bricks[i][j].x) &&
                                    (ball->x + radius > bricks[i][j].x);
                    int from_right = (prev_ball_x - radius >= bricks[i][j].x + BRICK_WIDTH) &&
                                     (ball->x - radius < bricks[i][j].x + BRICK_WIDTH);
                    int from_top = (prev_ball_y + radius <= bricks[i][j].y) &&
                                   (ball->y + radius > bricks[i][j].y);
                    int from_bottom = (prev_ball_y - radius >= bricks[i][j].y + BRICK_HEIGHT) &&
                                      (ball->y - radius < bricks[i][j].y + BRICK_HEIGHT);

                    if (hit_axis)
                    {
                        if (from_left || from_right)
                            *hit_axis = 1; /* horizontal: flip dx */
                        else if (from_top || from_bottom)
                            *hit_axis = 0; /* vertical: flip dy */
                        else
                        {
                            int overlap_left = (ball->x + radius) - bricks[i][j].x;
                            int overlap_right = (bricks[i][j].x + BRICK_WIDTH) - (ball->x - radius);
                            int overlap_top = (ball->y + radius) - bricks[i][j].y;
                            int overlap_bottom = (bricks[i][j].y + BRICK_HEIGHT) - (ball->y - radius);
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
                            if (hit_pill_type)
                                *hit_pill_type = PILL_TYPE_LIFE;
                        }
                        else
                        {
                            if ((rand() % LIFE_POWERUP_CHANCE) == 0)
                            {
                                if (hit_pill_type)
                                {
                                    int roll = rand() % 5;
                                    if (roll == 0)
                                        *hit_pill_type = PILL_TYPE_LIFE;
                                    else if (roll == 1)
                                        *hit_pill_type = PILL_TYPE_GROW;
                                    else if (roll == 2)
                                        *hit_pill_type = PILL_TYPE_SHRINK;
                                    else if (roll == 3)
                                        *hit_pill_type = PILL_TYPE_MULTIBALL;
                                    else
                                        *hit_pill_type = PILL_TYPE_LASER;
                                }
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

int update_ball(Ball *ball, int ball_index, int *brick_hit_x, int *brick_hit_y)
{
    int hit_pos;
    int brick_hit = 0;
    int brick_collided = 0;
    int brick_pill_type = PILL_TYPE_NONE;
    int radius = BALL_SIZE / 2;
    int min_x = radius + 1;
    int max_x = (SCREEN_WIDTH - 2) - radius;
    int min_y = radius + 1;
    float prev_ball_x = ball->x;
    float prev_ball_y = ball->y;
    int wall_hit = 0;
    int paddle_hit = 0;
    int brick_hit_row = 0;
    int brick_hit_axis = 0;
    int brick_destroyed = 0;

    ball->x += ball->dx;
    ball->y += ball->dy;

    // Wall collision
    if (ball->x < min_x)
    {
        ball->x = min_x;
        if (ball->dx < 0)
            ball->dx = -ball->dx;
        wall_hit = 1;
    }
    else if (ball->x > max_x)
    {
        ball->x = max_x;
        if (ball->dx > 0)
            ball->dx = -ball->dx;
        wall_hit = 1;
    }

    if (ball->y < min_y)
    {
        ball->y = min_y;
        if (ball->dy < 0)
            ball->dy = -ball->dy;
        wall_hit = 1;
    }

    // Paddle collision
    if (ball->x + radius > paddle.x &&
        ball->x - radius < paddle.x + paddle.width &&
        ball->y + radius > paddle.y &&
        ball->y - radius < paddle.y + PADDLE_HEIGHT)
    {

        ball->dy = -ball->dy;
        ball->y = paddle.y - radius;
        paddle_hit = 1;

        hit_pos = (int)ball->x - paddle.x;
        ball->dx = (float)(hit_pos - paddle.width / 2) / 5.0f;
        if (ball->dx == 0.0f)
            ball->dx = 1.0f;
    }

    if (check_brick_collision(ball, prev_ball_x, prev_ball_y, brick_hit_x, brick_hit_y, &brick_hit_row, &brick_hit_axis, &brick_destroyed, &brick_pill_type))
    {
        brick_collided = 1;
        if (brick_hit_axis)
        {
            ball->dx = -ball->dx;
            if (prev_ball_x < *brick_hit_x)
                ball->x = *brick_hit_x - radius;
            else if (prev_ball_x > *brick_hit_x + BRICK_WIDTH)
                ball->x = *brick_hit_x + BRICK_WIDTH + radius;
        }
        else
        {
            ball->dy = -ball->dy;
            if (prev_ball_y < *brick_hit_y)
                ball->y = *brick_hit_y - radius;
            else if (prev_ball_y > *brick_hit_y + BRICK_HEIGHT)
                ball->y = *brick_hit_y + BRICK_HEIGHT + radius;
        }
        brick_hit = brick_destroyed ? 1 : 0;
    }

    if (brick_pill_type != PILL_TYPE_NONE)
    {
        life_pill_spawn_request = 1;
        life_pill_spawn_type = brick_pill_type;
        life_pill_spawn_x = *brick_hit_x;
        life_pill_spawn_y = *brick_hit_y;
    }

    if (brick_collided)
        audio_event_brick(brick_hit_row);
    else if (paddle_hit)
        audio_event_paddle();
    else if (wall_hit)
        audio_event_wall();

    if (brick_collided || paddle_hit || wall_hit)
        increase_ball_speed(ball);

    if (ball->y >= SCREEN_HEIGHT)
    {
        ball_active[ball_index] = 0;

        if (count_active_balls() == 0)
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

        return 0;
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

    fade_palette_color(intro_border_index,
                       intro_border_r, intro_border_g, intro_border_b,
                       0, 0, 0,
                       fade_steps, fade_delay_ms);

    clear_screen(0);
}

void game_loop()
{
    char buffer[50];
    int old_ball_x[MAX_BALLS];
    int old_ball_y[MAX_BALLS];
    int old_ball_active[MAX_BALLS];
    int old_paddle_x;
    int old_paddle_width;
    int old_paddle_laser_enabled;
    int old_laser_x[MAX_LASER_SHOTS];
    int old_laser_y[MAX_LASER_SHOTS];
    int old_laser_active[MAX_LASER_SHOTS];
    int first_frame;
    int brick_hit_x, brick_hit_y;
    int radius = BALL_SIZE / 2;
    int launch_dx;
    int old_pill_x, old_pill_y;
    int old_pill_active;
    int spawned_this_frame;
    int destroyed_brick_x[MAX_BRICKS_DESTROYED_PER_FRAME];
    int destroyed_brick_y[MAX_BRICKS_DESTROYED_PER_FRAME];
    int destroyed_brick_count;
    int i;

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
                for (i = 0; i < MAX_BALLS; i++)
                {
                    if (ball_active[i])
                        draw_ball(balls[i]);
                }
                if (life_pill.active)
                    draw_pill(life_pill);
                for (i = 0; i < MAX_LASER_SHOTS; i++)
                {
                    if (laser_shots[i].active)
                        draw_laser_shot(laser_shots[i]);
                }
                draw_ui(score, lives, current_level);
                if (paused)
                {
                    draw_pause_overlay();
                }
                first_frame = 0;
                force_redraw = 0;
            }

            /* Save old positions */
            for (i = 0; i < MAX_BALLS; i++)
            {
                old_ball_active[i] = ball_active[i];
                if (ball_active[i])
                {
                    old_ball_x[i] = (int)(balls[i].x + 0.5f);
                    old_ball_y[i] = (int)(balls[i].y + 0.5f);
                }
            }
            old_paddle_x = paddle.x;
            old_paddle_width = paddle.width;
            old_paddle_laser_enabled = paddle.laser_enabled;
            old_pill_x = life_pill.x;
            old_pill_y = life_pill.y;
            old_pill_active = life_pill.active;
            for (i = 0; i < MAX_LASER_SHOTS; i++)
            {
                old_laser_active[i] = laser_shots[i].active;
                if (laser_shots[i].active)
                {
                    old_laser_x[i] = laser_shots[i].x;
                    old_laser_y[i] = laser_shots[i].y;
                }
            }

            /* Update game state */
            get_inputs();
            if (!paused)
            {
                spawned_this_frame = 0;
                destroyed_brick_count = 0;
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
                        balls[0].dx = (float)launch_dx;
                        balls[0].dy = -2.0f;
                    }
                    else
                    {
                        balls[0].x = paddle.x + paddle.width / 2;
                        balls[0].y = paddle.y - radius - 1;
                    }
                }
                else
                {
                    for (i = 0; i < MAX_BALLS; i++)
                    {
                        int ball_result;

                        if (!ball_active[i])
                            continue;

                        ball_result = update_ball(&balls[i], i, &brick_hit_x, &brick_hit_y);
                        if (ball_result == 1)
                        {
                            record_destroyed_brick(destroyed_brick_x, destroyed_brick_y,
                                                  &destroyed_brick_count, brick_hit_x, brick_hit_y);
                        }

                        if (life_pill_spawn_request)
                        {
                            spawn_life_pill(life_pill_spawn_x, life_pill_spawn_y, life_pill_spawn_type);
                            life_pill_spawn_request = 0;
                            life_pill_spawn_type = PILL_TYPE_NONE;
                            life_pill_spawn_x = 0;
                            life_pill_spawn_y = 0;
                            spawned_this_frame = 1;
                        }
                    }
                }

                if (!spawned_this_frame)
                    update_life_pill();

                update_laser_shots(destroyed_brick_x, destroyed_brick_y, &destroyed_brick_count);
            }
            else
            {
                destroyed_brick_count = 0;
            }

            wait_vblank();
            /* Erase old positions */
            for (i = 0; i < MAX_BALLS; i++)
            {
                if (!old_ball_active[i])
                    continue;

                erase_ball_with_background(old_ball_x[i], old_ball_y[i], balls[i]);
                {
                    int r = BALL_SIZE / 2;
                    int x1 = old_ball_x[i] - r - 1;
                    int y1 = old_ball_y[i] - r - 1;
                    int x2 = old_ball_x[i] + r + 1;
                    int y2 = old_ball_y[i] + r + 1;
                    redraw_bricks_in_area(x1, y1, x2, y2);
                }
            }
            if (old_paddle_x != paddle.x || old_paddle_width != paddle.width || old_paddle_laser_enabled != paddle.laser_enabled)
            {
                Paddle old_paddle = paddle;
                old_paddle.width = old_paddle_width;
                old_paddle.laser_enabled = old_paddle_laser_enabled;
                erase_paddle_with_background(old_paddle_x, old_paddle);
            }
            for (i = 0; i < MAX_LASER_SHOTS; i++)
            {
                if (old_laser_active[i])
                {
                    erase_laser_shot_with_background(old_laser_x[i], old_laser_y[i]);
                    redraw_bricks_in_area(old_laser_x[i] - 1, old_laser_y[i] - 4, old_laser_x[i] + 1, old_laser_y[i] + 1);
                }
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

            /* Erase destroyed bricks */
            for (i = 0; i < destroyed_brick_count; i++)
            {
                erase_rect_with_background(destroyed_brick_x[i], destroyed_brick_y[i], BRICK_WIDTH, BRICK_HEIGHT);
            }

            /* Draw new positions */
            draw_paddle(paddle);
            for (i = 0; i < MAX_BALLS; i++)
            {
                if (ball_active[i])
                    draw_ball(balls[i]);
            }
            if (life_pill.active)
                draw_pill(life_pill);
            for (i = 0; i < MAX_LASER_SHOTS; i++)
            {
                if (laser_shots[i].active)
                    draw_laser_shot(laser_shots[i]);
            }

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
            delay(1500);
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
