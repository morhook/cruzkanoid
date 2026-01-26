#ifndef AUDIO_H
#define AUDIO_H

void far audio_init(void);
void far audio_shutdown(void);

void far audio_update(void);
void far audio_toggle(void);
int far audio_is_enabled(void);

void far audio_music_toggle(void);
int far audio_music_is_enabled(void);
void far audio_music_restart(void);
void far audio_music_stop(void);

void far audio_event_paddle(void);
void far audio_event_wall(void);
void far audio_event_brick(int row);
void far audio_event_life_lost_blocking(void);
void far audio_event_life_up_blocking(void);
void far audio_event_level_clear_blocking(void);

#endif
