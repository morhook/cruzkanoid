#ifndef VIDEO_H
#define VIDEO_H

#include "cruzkan.h"

void far set_palette_color(unsigned char index, unsigned char r, unsigned char g, unsigned char b);

void far set_mode(unsigned char mode);

void far put_pixel(int x, int y, unsigned char color);

void far draw_rect(int x, int y, int width, int height, unsigned char color);

void far draw_filled_rect(int x, int y, int width, int height, unsigned char color);

void far clear_screen(unsigned char color);

void far draw_paddle(Paddle paddle);

void far erase_paddle(int x, Paddle paddle);

void far draw_filled_circle(int cx, int cy, int r, unsigned char color, unsigned char border_color);

void far draw_ball(Ball ball);

void far erase_ball(int x, int y, Ball ball);

void far draw_char(int x, int y, char c, unsigned char color);

void far draw_text(int x, int y, char *text);

void far draw_pause_overlay();

void far draw_ui(int score, int lives, int current_level);

void far draw_char_transparent(int x, int y, char c, unsigned char color);

void far draw_text_transparent(int x, int y, char *text, unsigned char color);

void far draw_bordered_text(int x, int y, char *text, unsigned char border_color);

void far wait_vblank(void);

void far fade_palette_color(unsigned char index,
                        unsigned char from_r, unsigned char from_g, unsigned char from_b,
                        unsigned char to_r, unsigned char to_g, unsigned char to_b,
                        int steps, int delay_ms);

void far draw_heart(int cx, int cy, int size, unsigned char color);

void far draw_background();

void far draw_background_area(int x1, int y1, int x2, int y2);

void far erase_ball_with_background(int x, int y, Ball ball);

void far erase_paddle_with_background(int x, Paddle paddle);

void far erase_rect_with_background(int x, int y, int width, int height);

void draw_brick(int x, int y, int width, int height, unsigned char base_color);

#endif /* VIDEO_H */
