#ifndef VIDEO_H

#include "cruzkan.h"

void set_palette_color(unsigned char index, unsigned char r, unsigned char g, unsigned char b);

void set_mode(unsigned char mode);

void put_pixel(int x, int y, unsigned char color);

void draw_rect(int x, int y, int width, int height, unsigned char color);

void draw_filled_rect(int x, int y, int width, int height, unsigned char color);

void clear_screen(unsigned char color);

void draw_paddle(Paddle paddle);

void erase_paddle(int x, Paddle paddle);

void draw_filled_circle(int cx, int cy, int r, unsigned char color, unsigned char border_color);

void draw_ball(Ball ball);

void erase_ball(int x, int y, Ball ball);

#endif /* VIDEO_H */