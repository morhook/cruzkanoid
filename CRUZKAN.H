#ifndef CRUZKAN_H
#define CRUZKAN_H

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define PADDLE_WIDTH 40
#define PADDLE_HEIGHT 8
#define BALL_SIZE 4
/* Brick rows 0-7 use palette indices 32-55 (8 rows x 3 shades = 24 entries).
   Paddle and pill palettes live after that to avoid collision. */
#define PADDLE_PALETTE_START 56
#define PILL_WIDTH 8
#define PILL_HEIGHT 12
#define PILL_FALL_SPEED 2
#define PILL_PALETTE_START 63
#define PILL_COLOR_LIGHT (PILL_PALETTE_START + 0)
#define PILL_COLOR_BASE (PILL_PALETTE_START + 1)
#define PILL_COLOR_BORDER (PILL_PALETTE_START + 2)
#define PILL_COLOR_GLYPH (PILL_PALETTE_START + 3)
#define PILL_TYPE_NONE 0
#define PILL_TYPE_NEXTLEVEL 1
#define PILL_TYPE_GROW 2
#define PILL_TYPE_SHRINK 3
#define PILL_TYPE_MULTIBALL 4
#define PILL_TYPE_LASER 5
#define MAX_BALLS 16
#define MAX_LASER_SHOTS 8
#define PADDLE_WIDTH_GROW (PADDLE_WIDTH + 12)
#define PADDLE_WIDTH_SHRINK (PADDLE_WIDTH - 12)
#define MONSTER_WIDTH 24
#define MONSTER_HEIGHT 16
#define MONSTER_HP 3
#define MONSTER_SPEED 2
#define MONSTER_PALETTE_START 67

typedef struct
{
    float x, y;
    float dx, dy;
} Ball;

typedef struct
{
    int x, y;
    int dy;
    int active;
    int type;
} Pill;

typedef struct
{
    int x, y;
    int width;
    int vx;
    int laser_enabled;
} Paddle;

typedef struct
{
    int x, y;
    int dy;
    int active;
} LaserShot;

typedef struct
{
    int x, y;
    int active;
    int color;
    int hp;
} Brick;

typedef struct
{
    int x, y;
    int hp;
    int active;
} Monster;

#endif /* CRUZKAN_H */
