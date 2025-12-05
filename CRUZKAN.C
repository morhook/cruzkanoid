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

typedef struct {
    int x, y;
    int dx, dy;
} Ball;

typedef struct {
    int x, y;
    int width;
} Paddle;

typedef struct {
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

void set_mode(unsigned char mode) {
    union REGS regs;
    regs.h.ah = 0x00;
    regs.h.al = mode;
    int86(0x10, &regs, &regs);
}

void put_pixel(int x, int y, unsigned char color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        VGA[y * SCREEN_WIDTH + x] = color;
    }
}

void draw_rect(int x, int y, int width, int height, unsigned char color) {
    int i, j;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            put_pixel(x + j, y + i, color);
        }
    }
}

void draw_filled_rect(int x, int y, int width, int height, unsigned char color) {
    draw_rect(x, y, width, height, color);
}

void clear_screen(unsigned char color) {
    int i;
    for (i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        VGA[i] = color;
    }
}

void init_bricks() {
    int i, j;
    int start_x = 10;
    int start_y = 20;

    for (i = 0; i < BRICK_ROWS; i++) {
        for (j = 0; j < BRICK_COLS; j++) {
            bricks[i][j].x = start_x + j * (BRICK_WIDTH + 2);
            bricks[i][j].y = start_y + i * (BRICK_HEIGHT + 2);
            bricks[i][j].active = 1;
            bricks[i][j].color = 40 + i * 10;
        }
    }
}

void draw_bricks() {
    int i, j;
    for (i = 0; i < BRICK_ROWS; i++) {
        for (j = 0; j < BRICK_COLS; j++) {
            if (bricks[i][j].active) {
                draw_filled_rect(bricks[i][j].x, bricks[i][j].y,
                               BRICK_WIDTH, BRICK_HEIGHT, bricks[i][j].color);
            }
        }
    }
}

void init_game() {
    paddle.x = SCREEN_WIDTH / 2 - PADDLE_WIDTH / 2;
    paddle.y = SCREEN_HEIGHT - 20;
    paddle.width = PADDLE_WIDTH;

    ball.x = SCREEN_WIDTH / 2;
    ball.y = paddle.y - BALL_SIZE - 1;
    ball.dx = 2;
    ball.dy = -2;

    init_bricks();
    score = 0;
    lives = 3;
}

void draw_paddle() {
    draw_filled_rect(paddle.x, paddle.y, paddle.width, PADDLE_HEIGHT, 15);
}

void erase_paddle(int x) {
    draw_filled_rect(x, paddle.y, paddle.width, PADDLE_HEIGHT, 0);
}

void draw_ball() {
    draw_filled_rect(ball.x, ball.y, BALL_SIZE, BALL_SIZE, 14);
}

void erase_ball(int x, int y) {
    draw_filled_rect(x, y, BALL_SIZE, BALL_SIZE, 0);
}

void update_paddle() {
    if (kbhit()) {
        char key = getch();
	if (key == 0 || key == 0xE0) {
            key = getch();
            if (key == 75) { // Left arrow
                paddle.x -= 5;
                if (paddle.x < 0) paddle.x = 0;
            }
            if (key == 77) { // Right arrow
                paddle.x += 5;
                if (paddle.x + paddle.width > SCREEN_WIDTH) {
                    paddle.x = SCREEN_WIDTH - paddle.width;
                }
            }
        }
        if (key == 27) { // ESC
            exit(0);
        }
    }
}

int check_brick_collision(int *hit_x, int *hit_y) {
    int i, j;
    for (i = 0; i < BRICK_ROWS; i++) {
        for (j = 0; j < BRICK_COLS; j++) {
            if (bricks[i][j].active) {
                if (ball.x + BALL_SIZE > bricks[i][j].x &&
                    ball.x < bricks[i][j].x + BRICK_WIDTH &&
                    ball.y + BALL_SIZE > bricks[i][j].y &&
                    ball.y < bricks[i][j].y + BRICK_HEIGHT) {

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

int update_ball(int *brick_hit_x, int *brick_hit_y) {
    int hit_pos;
    int brick_hit = 0;

    ball.x += ball.dx;
    ball.y += ball.dy;

    // Wall collision
    if (ball.x <= 0 || ball.x + BALL_SIZE >= SCREEN_WIDTH) {
	ball.dx = -ball.dx;
    }

    if (ball.y <= 0) {
	ball.dy = -ball.dy;
    }

    // Paddle collision
    if (ball.y + BALL_SIZE >= paddle.y &&
	ball.y < paddle.y + PADDLE_HEIGHT &&
	ball.x + BALL_SIZE > paddle.x &&
	ball.x < paddle.x + paddle.width) {
	ball.dy = -ball.dy;
	ball.y = paddle.y - BALL_SIZE;

	// Add spin based on where ball hits paddle
	hit_pos = ball.x - paddle.x;
	ball.dx = (hit_pos - paddle.width / 2) / 5;
	if (ball.dx == 0) ball.dx = 1;
    }

    // Brick collision
    if (check_brick_collision(brick_hit_x, brick_hit_y)) {
        ball.dy = -ball.dy;
        brick_hit = 1;
    }

    // Ball lost
    if (ball.y >= SCREEN_HEIGHT) {
        lives--;
        if (lives > 0) {
            ball.x = paddle.x + paddle.width / 2;
            ball.y = paddle.y - BALL_SIZE - 1;
            ball.dx = 2;
            ball.dy = -2;
            delay(1000);
        }
    }

    return brick_hit;
}

int check_win() {
    int i, j;
    for (i = 0; i < BRICK_ROWS; i++) {
        for (j = 0; j < BRICK_COLS; j++) {
            if (bricks[i][j].active) {
                return 0;
            }
        }
    }
    return 1;
}

unsigned char* get_font_char(char c) {
    static unsigned char font_data[43][7] = {
	/* 0 */ {0x70,0x88,0x88,0x88,0x88,0x88,0x70},
	/* 1 */ {0x20,0x60,0x20,0x20,0x20,0x20,0x70},
	/* 2 */ {0x70,0x88,0x08,0x10,0x20,0x40,0xF8},
	/* 3 */ {0x70,0x88,0x08,0x30,0x08,0x88,0x70},
	/* 4 */ {0x10,0x30,0x50,0x90,0xF8,0x10,0x10},
	/* 5 */ {0xF8,0x80,0xF0,0x08,0x08,0x88,0x70},
	/* 6 */ {0x30,0x40,0x80,0xF0,0x88,0x88,0x70},
	/* 7 */ {0xF8,0x08,0x10,0x20,0x40,0x40,0x40},
	/* 8 */ {0x70,0x88,0x88,0x70,0x88,0x88,0x70},
	/* 9 */ {0x70,0x88,0x88,0x78,0x08,0x10,0x60},
	/* : */ {0x00,0x20,0x00,0x00,0x00,0x20,0x00},
	/* A */ {0x20,0x50,0x88,0x88,0xF8,0x88,0x88},
	/* E */ {0xF8,0x80,0x80,0xF0,0x80,0x80,0xF8},
	/* F */ {0xF8,0x80,0x80,0xF0,0x80,0x80,0x80},
	/* G */ {0x70,0x88,0x80,0xB8,0x88,0x88,0x70},
	/* I */ {0x70,0x20,0x20,0x20,0x20,0x20,0x70},
	/* L */ {0x80,0x80,0x80,0x80,0x80,0x80,0xF8},
	/* M */ {0x88,0xD8,0xA8,0xA8,0x88,0x88,0x88},
	/* N */ {0x88,0x88,0xC8,0xA8,0x98,0x88,0x88},
	/* O */ {0x70,0x88,0x88,0x88,0x88,0x88,0x70},
	/* P */ {0xF0,0x88,0x88,0xF0,0x80,0x80,0x80},
	/* R */ {0xF0,0x88,0x88,0xF0,0x90,0x88,0x88},
	/* S */ {0x70,0x88,0x80,0x70,0x08,0x88,0x70},
	/* U */ {0x88,0x88,0x88,0x88,0x88,0x88,0x70},
	/* V */ {0x88,0x88,0x88,0x88,0x50,0x50,0x20},
	/* W */ {0x88,0x88,0x88,0xA8,0xA8,0xD8,0x88},
	/* Y */ {0x88,0x88,0x50,0x20,0x20,0x20,0x20},
	/* a */ {0x00,0x00,0x70,0x08,0x78,0x88,0x78},
	/* c */ {0x00,0x00,0x70,0x80,0x80,0x88,0x70},
	/* e */ {0x00,0x00,0x70,0x88,0xF8,0x80,0x70},
	/* i */ {0x20,0x00,0x60,0x20,0x20,0x20,0x70},
	/* k */ {0x80,0x80,0x90,0xA0,0xE0,0x90,0x88},
	/* l */ {0x60,0x20,0x20,0x20,0x20,0x20,0x70},
	/* n */ {0x00,0x00,0xB0,0xC8,0x88,0x88,0x88},
	/* o */ {0x00,0x00,0x70,0x88,0x88,0x88,0x70},
	/* r */ {0x00,0x00,0x68,0x70,0x40,0x40,0x40},
	/* s */ {0x00,0x00,0x70,0x80,0x70,0x08,0xF0},
	/* t */ {0x20,0x20,0x70,0x20,0x20,0x20,0x18},
	/* v */ {0x00,0x00,0x88,0x88,0x88,0x50,0x20},
	/* x */ {0x00,0x00,0x88,0x50,0x20,0x50,0x88},
	/* y */ {0x00,0x00,0x88,0x88,0x78,0x08,0x70},
	/*spc*/ {0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	/*unk*/ {0xF8,0xF8,0xF8,0xF8,0xF8,0xF8,0xF8}
    };

    if (c >= '0' && c <= '9') return font_data[c - '0'];
    if (c == ':') return font_data[10];
    if (c == 'A') return font_data[11];
    if (c == 'E') return font_data[12];
    if (c == 'F') return font_data[13];
    if (c == 'G') return font_data[14];
    if (c == 'I') return font_data[15];
    if (c == 'L') return font_data[16];
    if (c == 'M') return font_data[17];
    if (c == 'N') return font_data[18];
    if (c == 'O') return font_data[19];
    if (c == 'P') return font_data[20];
    if (c == 'R') return font_data[21];
    if (c == 'S') return font_data[22];
    if (c == 'U') return font_data[23];
    if (c == 'V') return font_data[24];
    if (c == 'W') return font_data[25];
    if (c == 'Y') return font_data[26];
    if (c == 'a') return font_data[27];
    if (c == 'c') return font_data[28];
    if (c == 'e') return font_data[29];
    if (c == 'i') return font_data[30];
    if (c == 'k') return font_data[31];
    if (c == 'l') return font_data[32];
    if (c == 'n') return font_data[33];
    if (c == 'o') return font_data[34];
    if (c == 'r') return font_data[35];
    if (c == 's') return font_data[36];
    if (c == 't') return font_data[37];
    if (c == 'v') return font_data[38];
    if (c == 'x') return font_data[39];
    if (c == 'y') return font_data[40];
    if (c == ' ') return font_data[41];
    return font_data[42]; /* unknown char */
}

void draw_char(int x, int y, char c, unsigned char color) {
    int i, j;
    unsigned char mask;
    unsigned char *font;

    font = get_font_char(c);

    for (i = 0; i < 7; i++) {
	mask = font[i];
	for (j = 0; j < 8; j++) {
	    if (mask & (0x80 >> j)) {
                put_pixel(x + j, y + i, color);
            }
        }
    }
}

void draw_text(int x, int y, char *text) {
    int i = 0;
    while (text[i] != '\0') {
        draw_char(x + i * 6, y, text[i], 15);
        i++;
    }
}

void draw_ui() {
    char buffer[50];

    // Draw border
    draw_rect(0, 0, SCREEN_WIDTH, 1, 15);
    draw_rect(0, 0, 1, SCREEN_HEIGHT, 15);
    draw_rect(SCREEN_WIDTH - 1, 0, 1, SCREEN_HEIGHT, 15);

    // Score and lives at top (pixel coordinates now)
    sprintf(buffer, "Score: %d", score);
    draw_text(5, 5, buffer);

    sprintf(buffer, "Lives: %d", lives);
    draw_text(200, 5, buffer);
}

void game_loop() {
    char buffer[50];
    int old_ball_x, old_ball_y;
    int old_paddle_x;
    int first_frame = 1;
    int brick_hit_x, brick_hit_y;
    int brick_was_hit;

    while (lives > 0 && !check_win()) {
	if (first_frame) {
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
	brick_was_hit = update_ball(&brick_hit_x, &brick_hit_y);

	/* Erase old positions */
	erase_ball(old_ball_x, old_ball_y);
	if (old_paddle_x != paddle.x) {
	    erase_paddle(old_paddle_x);
	}

	/* Erase destroyed brick */
	if (brick_was_hit) {
	    draw_filled_rect(brick_hit_x, brick_hit_y, BRICK_WIDTH, BRICK_HEIGHT, 0);
	}

	/* Draw new positions */
	draw_paddle();
	draw_ball();

	/* Redraw UI (borders, score, lives) */
	draw_ui();

	delay(20);
    }

    clear_screen(0);
    if (lives == 0) {
	draw_text(110, 90, "GAME OVER!");
    } else {
	draw_text(120, 90, "YOU WIN!");
    }

    sprintf(buffer, "Final Score: %d", score);
    draw_text(90, 105, buffer);
    draw_text(70, 120, "Press any key to exit");

    getch();
}

int main() {
    set_mode(0x13); // 320x200 256 color mode

    init_game();
    game_loop();

    set_mode(0x03); // Back to text mode

    return 0;
}
