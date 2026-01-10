#ifndef AUDIO_H
#define AUDIO_H

void audio_init(void);
void audio_shutdown(void);

void audio_update(void);
void audio_toggle(void);
int audio_is_enabled(void);

void audio_music_toggle(void);
int audio_music_is_enabled(void);
void audio_music_restart(void);

void audio_stop_internal(void);

void audio_event_paddle(void);
void audio_event_wall(void);
void audio_event_brick(int row);
void audio_event_life_lost_blocking(void);
void audio_event_level_clear_blocking(void);

#endif
