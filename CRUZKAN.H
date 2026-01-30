#ifndef CRUZKAN_H
#define CRUZKAN_H

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define PADDLE_WIDTH 40
#define PADDLE_HEIGHT 8
#define BALL_SIZE 4
#define PADDLE_PALETTE_START 48
#define PILL_WIDTH 8
#define PILL_HEIGHT 12
#define PILL_FALL_SPEED 2

typedef struct
{
    int x, y;
    int dx, dy;
} Ball;

typedef struct
{
    int x, y;
    int dy;
    int active;
} Pill;

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
    int hp;
    int gives_life;
} Brick;

#endif /* CRUZKAN_H */
