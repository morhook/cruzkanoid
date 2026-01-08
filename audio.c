#include "audio.h"

#include <conio.h>
#include <dos.h>
#include <time.h>

static int audio_enabled = 1;
static int audio_active = 0;
static clock_t audio_end_clock = 0;

static void audio_stop_internal(void)
{
    nosound();
    audio_active = 0;
    audio_end_clock = 0;
}

static void audio_start_tone(int freq, int ms)
{
    clock_t now;
    clock_t ticks;

    if (!audio_enabled)
        return;

    if (freq <= 0 || ms <= 0)
        return;

    sound(freq);
    now = clock();

    ticks = (clock_t)(((long)ms * (long)CLK_TCK + 999L) / 1000L);
    if (ticks <= 0)
        ticks = 1;

    audio_end_clock = now + ticks;
    audio_active = 1;
}

static void audio_play_tone(int freq, int ms)
{
    if (!audio_enabled)
        return;

    audio_start_tone(freq, ms);
}

void audio_init(void)
{
    audio_enabled = 1;
    audio_stop_internal();
}

void audio_shutdown(void)
{
    audio_stop_internal();
}

int audio_is_enabled(void)
{
    return audio_enabled;
}

void audio_toggle(void)
{
    audio_enabled = !audio_enabled;
    if (!audio_enabled)
        audio_stop_internal();
}

void audio_update(void)
{
    if (!audio_active)
        return;

    if (clock() < audio_end_clock)
        return;

    audio_stop_internal();
}

void audio_event_paddle(void)
{
    audio_play_tone(1200, 45);
}

void audio_event_wall(void)
{
    audio_play_tone(800, 35);
}

void audio_event_brick(int row)
{
    int freq = 1500 - row * 140;
    if (freq < 400)
        freq = 400;
    audio_play_tone(freq, 55);
}

void audio_event_life_lost_blocking(void)
{
    if (!audio_enabled)
        return;

    audio_stop_internal();
    sound(900);
    delay(110);
    sound(650);
    delay(120);
    sound(450);
    delay(170);
    nosound();
}

void audio_event_level_clear_blocking(void)
{
    if (!audio_enabled)
        return;

    audio_stop_internal();
    sound(660);
    delay(90);
    sound(880);
    delay(90);
    sound(990);
    delay(110);
    sound(1320);
    delay(140);
    nosound();
}
