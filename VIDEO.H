#ifndef VIDEO_H

void set_palette_color(unsigned char index, unsigned char r, unsigned char g, unsigned char b);

void set_mode(unsigned char mode);

void put_pixel(int x, int y, unsigned char color);

void draw_rect(int x, int y, int width, int height, unsigned char color);

void draw_filled_rect(int x, int y, int width, int height, unsigned char color);

void clear_screen(unsigned char color);

unsigned char *get_font_char(char c);

void draw_char(int x, int y, char c, unsigned char color);

void draw_text(int x, int y, char *text);

#endif /* VIDEO_H */