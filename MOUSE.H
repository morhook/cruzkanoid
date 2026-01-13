#ifndef MOUSE_H
#define MOUSE_H

int far mouse_init(void);
void far mouse_shutdown(void);
void far mouse_update(void);
void far mouse_set_pos(int x, int y);

extern int mouse_available;
extern int mouse_buttons;
extern int mouse_prev_buttons;
extern int mouse_x;
extern int mouse_y;

#endif /* MOUSE_H */
