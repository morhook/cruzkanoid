#ifndef VIDEO_H
#define VIDEO_H

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

unsigned char *get_font_char(char c);

void draw_char(int x, int y, char c, unsigned char color);

void draw_text(int x, int y, char *text);

void draw_pause_overlay();

void draw_ui(int score, int lives, int current_level);

void draw_char_transparent(int x, int y, char c, unsigned char color);

void draw_text_transparent(int x, int y, char *text, unsigned char color);

void draw_bordered_text(int x, int y, char *text, unsigned char border_color);

void wait_vblank(void);

void fade_palette_color(unsigned char index,
                        unsigned char from_r, unsigned char from_g, unsigned char from_b,
                        unsigned char to_r, unsigned char to_g, unsigned char to_b,
                        int steps, int delay_ms);

#endif /* VIDEO_H */
