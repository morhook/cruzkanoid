#ifndef MUSIC_H
#define MUSIC_H

void far music_init(void);
void far music_shutdown(void);

int far music_backend_init(int sb_is_present);
void far music_backend_shutdown(void);
int far music_backend_has_opl(void);
void far music_backend_stop(void);
void far music_backend_play_step(unsigned int freq_hz, unsigned int freq2_hz);

int far music_is_enabled(void);
void far music_toggle(void);
void far music_restart(void);
void far music_stop(void);
void far music_set_track(int track_index);

void far music_on_note_finished(void);
void far music_on_sfx_preempt(int preempted_music_tone);

int far music_prepare_next_note(int audio_enabled,
                                 int audio_active,
                                 int life_up_active,
                                 int multiball_active,
                                 unsigned int *out_freq,
                                 unsigned int *out_freq2,
                                 unsigned int *out_ms);

void far music_drum_update(void);

#endif
