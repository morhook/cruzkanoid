#include "audio.h"

#include <alloc.h>
#include <conio.h>
#include <dos.h>
#include <stdlib.h>
#include <time.h>

static int audio_enabled = 1;
static int audio_active = 0;
static clock_t audio_end_clock = 0;

typedef enum
{
    AUDIO_BACKEND_SPEAKER = 0,
    AUDIO_BACKEND_SOUNDBLASTER = 1
} AudioBackend;

static AudioBackend audio_backend = AUDIO_BACKEND_SPEAKER;

typedef enum
{
    TONE_NONE = 0,
    TONE_SFX = 1,
    TONE_MUSIC = 2,
    TONE_SILENCE = 3
} ToneSource;

static ToneSource tone_source = TONE_NONE;

typedef struct
{
    unsigned int freq;
    unsigned int ms;
} MusicNote;

static int music_enabled = 1;
static int music_running = 1;
static unsigned int music_index = 0;
static unsigned int music_len = 0;
static unsigned int music_drum_mute_start = 0;

#define NOTE_C3 131
#define NOTE_D3 147
#define NOTE_E3 165
#define NOTE_F3 175
#define NOTE_G3 196
#define NOTE_A3 220
#define NOTE_B3 247

#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494

#define NOTE_C5 523
#define NOTE_D5 587
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_G5 784
#define NOTE_A5 880

static const MusicNote music_track[] = {
    /* Cruzkanoid Groove (extended), looped. */
    {0, 110}, {0, 110}, {0, 110}, {0, 110},
    {0, 110}, {0, 110}, {0, 110}, {0, 110},
    {0, 110}, {0, 110}, {0, 110}, {0, 110},
    {0, 110}, {0, 110}, {0, 110}, {0, 110},

    {NOTE_A3, 110}, {NOTE_E4, 110}, {NOTE_A4, 110}, {NOTE_C5, 110},
    {NOTE_E4, 110}, {NOTE_A4, 110}, {NOTE_C5, 110}, {NOTE_E5, 110},
    {NOTE_A3, 110}, {NOTE_E4, 110}, {NOTE_A4, 110}, {NOTE_C5, 110},
    {NOTE_E4, 110}, {NOTE_A4, 110}, {NOTE_C5, 110}, {0, 110},

    {NOTE_F3, 110}, {NOTE_C4, 110}, {NOTE_F4, 110}, {NOTE_A4, 110},
    {NOTE_C4, 110}, {NOTE_F4, 110}, {NOTE_A4, 110}, {NOTE_C5, 110},
    {NOTE_F3, 110}, {NOTE_C4, 110}, {NOTE_F4, 110}, {NOTE_A4, 110},
    {NOTE_C4, 110}, {NOTE_F4, 110}, {NOTE_A4, 110}, {0, 110},

    {NOTE_C3, 110}, {NOTE_G3, 110}, {NOTE_C4, 110}, {NOTE_E4, 110},
    {NOTE_G3, 110}, {NOTE_C4, 110}, {NOTE_E4, 110}, {NOTE_G4, 110},
    {NOTE_C3, 110}, {NOTE_G3, 110}, {NOTE_C4, 110}, {NOTE_E4, 110},
    {NOTE_G3, 110}, {NOTE_C4, 110}, {NOTE_E4, 110}, {0, 110},

    {NOTE_G3, 110}, {NOTE_D4, 110}, {NOTE_G4, 110}, {NOTE_B4, 110},
    {NOTE_D4, 110}, {NOTE_G4, 110}, {NOTE_B4, 110}, {NOTE_D5, 110},
    {NOTE_G3, 110}, {NOTE_D4, 110}, {NOTE_G4, 110}, {NOTE_B4, 110},
    {NOTE_D4, 110}, {NOTE_G4, 110}, {NOTE_B4, 110}, {0, 110},

    /* Variation A (keeps the bass feel, adds a lead). */
    {NOTE_A3, 110}, {NOTE_E4, 110}, {NOTE_A4, 110}, {NOTE_E5, 110},
    {NOTE_D5, 110}, {NOTE_C5, 110}, {NOTE_B4, 110}, {NOTE_A4, 110},
    {NOTE_A3, 110}, {NOTE_E4, 110}, {NOTE_A4, 110}, {NOTE_C5, 110},
    {NOTE_D5, 110}, {NOTE_E5, 110}, {NOTE_C5, 110}, {0, 110},

    {NOTE_F3, 110}, {NOTE_C4, 110}, {NOTE_F4, 110}, {NOTE_C5, 110},
    {NOTE_D5, 110}, {NOTE_E5, 110}, {NOTE_F5, 110}, {NOTE_E5, 110},
    {NOTE_F3, 110}, {NOTE_C4, 110}, {NOTE_A4, 110}, {NOTE_C5, 110},
    {NOTE_A4, 110}, {NOTE_F4, 110}, {NOTE_C4, 110}, {0, 110},

    /* Bridge (more motion, a short "climb" then reset). */
    {NOTE_C3, 110}, {NOTE_G3, 110}, {NOTE_C4, 110}, {NOTE_G4, 110},
    {NOTE_E4, 110}, {NOTE_G4, 110}, {NOTE_C5, 110}, {NOTE_E5, 110},
    {NOTE_G3, 110}, {NOTE_B3, 110}, {NOTE_D4, 110}, {NOTE_F4, 110},
    {NOTE_G4, 110}, {NOTE_B4, 110}, {NOTE_D5, 110}, {0, 110},

    {NOTE_G3, 110}, {NOTE_D4, 110}, {NOTE_G4, 110}, {NOTE_D5, 110},
    {NOTE_E5, 110}, {NOTE_F5, 110}, {NOTE_G5, 110}, {NOTE_A5, 110},
    {NOTE_G5, 110}, {NOTE_E5, 110}, {NOTE_C5, 110}, {NOTE_A4, 110},
    {NOTE_G4, 110}, {NOTE_E4, 110}, {NOTE_D4, 110}, {0, 110},

    /* Variation B (alternating bass + lead to highlight mixed waveforms). */
    {NOTE_A3, 110}, {NOTE_C5, 110}, {NOTE_A3, 110}, {NOTE_E5, 110},
    {NOTE_A3, 110}, {NOTE_D5, 110}, {NOTE_A3, 110}, {NOTE_C5, 110},
    {NOTE_F3, 110}, {NOTE_C5, 110}, {NOTE_F3, 110}, {NOTE_F5, 110},
    {NOTE_F3, 110}, {NOTE_E5, 110}, {NOTE_F3, 110}, {NOTE_C5, 110},

    {NOTE_C3, 110}, {NOTE_G4, 110}, {NOTE_C3, 110}, {NOTE_E5, 110},
    {NOTE_C3, 110}, {NOTE_C5, 110}, {NOTE_C3, 110}, {NOTE_G4, 110},
    {NOTE_G3, 110}, {NOTE_B4, 110}, {NOTE_G3, 110}, {NOTE_D5, 110},
    {NOTE_G3, 110}, {NOTE_A4, 110}, {NOTE_G3, 110}, {0, 110},

    /* repeat everything, with just a little bit different the ending */
    {NOTE_A3, 110}, {NOTE_E4, 110}, {NOTE_A4, 110}, {NOTE_C5, 110},
    {NOTE_E4, 110}, {NOTE_A4, 110}, {NOTE_C5, 110}, {NOTE_E5, 110},
    {NOTE_A3, 110}, {NOTE_E4, 110}, {NOTE_A4, 110}, {NOTE_C5, 110},
    {NOTE_E4, 110}, {NOTE_A4, 110}, {NOTE_C5, 110}, {0, 110},

    {NOTE_F3, 110}, {NOTE_C4, 110}, {NOTE_F4, 110}, {NOTE_A4, 110},
    {NOTE_C4, 110}, {NOTE_F4, 110}, {NOTE_A4, 110}, {NOTE_C5, 110},
    {NOTE_F3, 110}, {NOTE_C4, 110}, {NOTE_F4, 110}, {NOTE_A4, 110},
    {NOTE_C4, 110}, {NOTE_F4, 110}, {NOTE_A4, 110}, {0, 110},

    {NOTE_C3, 110}, {NOTE_G3, 110}, {NOTE_C4, 110}, {NOTE_E4, 110},
    {NOTE_G3, 110}, {NOTE_C4, 110}, {NOTE_E4, 110}, {NOTE_G4, 110},
    {NOTE_C3, 110}, {NOTE_G3, 110}, {NOTE_C4, 110}, {NOTE_E4, 110},
    {NOTE_G3, 110}, {NOTE_C4, 110}, {NOTE_E4, 110}, {0, 110},

    {NOTE_G3, 110}, {NOTE_D4, 110}, {NOTE_G4, 110}, {NOTE_B4, 110},
    {NOTE_D4, 110}, {NOTE_G4, 110}, {NOTE_B4, 110}, {NOTE_D5, 110},
    {NOTE_G3, 110}, {NOTE_D4, 110}, {NOTE_G4, 110}, {NOTE_B4, 110},
    {NOTE_D4, 110}, {NOTE_G4, 110}, {NOTE_B4, 110}, {0, 110},

    {NOTE_A3, 110}, {NOTE_E4, 110}, {NOTE_A4, 110}, {NOTE_E5, 110},
    {NOTE_D5, 110}, {NOTE_C5, 110}, {NOTE_B4, 110}, {NOTE_A4, 110},
    {NOTE_A3, 110}, {NOTE_E4, 110}, {NOTE_A4, 110}, {NOTE_C5, 110},
    {NOTE_D5, 110}, {NOTE_E5, 110}, {NOTE_C5, 110}, {0, 110},

    {NOTE_F3, 110}, {NOTE_C4, 110}, {NOTE_F4, 110}, {NOTE_C5, 110},
    {NOTE_D5, 110}, {NOTE_E5, 110}, {NOTE_F5, 110}, {NOTE_E5, 110},
    {NOTE_F3, 110}, {NOTE_C4, 110}, {NOTE_A4, 110}, {NOTE_C5, 110},
    {NOTE_A4, 110}, {NOTE_F4, 110}, {NOTE_C4, 110}, {0, 110},

    {NOTE_C3, 110}, {NOTE_G3, 110}, {NOTE_C4, 110}, {NOTE_G4, 110},
    {NOTE_E4, 110}, {NOTE_G4, 110}, {NOTE_C5, 110}, {NOTE_E5, 110},
    {NOTE_G3, 110}, {NOTE_B3, 110}, {NOTE_D4, 110}, {NOTE_F4, 110},
    {NOTE_G4, 110}, {NOTE_B4, 110}, {NOTE_D5, 110}, {0, 110},

    {NOTE_G3, 110}, {NOTE_D4, 110}, {NOTE_G4, 110}, {NOTE_D5, 110},
    {NOTE_E5, 110}, {NOTE_F5, 110}, {NOTE_G5, 110}, {NOTE_A5, 110},
    {NOTE_G5, 110}, {NOTE_E5, 110}, {NOTE_C5, 110}, {NOTE_A4, 110},
    {NOTE_G4, 110}, {NOTE_E4, 110}, {NOTE_D4, 110}, {0, 110},

    {NOTE_A3, 110}, {NOTE_C5, 110}, {NOTE_A3, 110}, {NOTE_E5, 110},
    {NOTE_A3, 110}, {NOTE_D5, 110}, {NOTE_A3, 110}, {NOTE_C5, 110},
    {NOTE_F3, 110}, {NOTE_C5, 110}, {NOTE_F3, 110}, {NOTE_F5, 110},
    {NOTE_F3, 110}, {NOTE_E5, 110}, {NOTE_F3, 110}, {NOTE_C5, 110},

    /* Variation C (and modified the "looper" notes of the end of this riff). */
    {NOTE_C3, 110}, {NOTE_G4, 110}, {NOTE_C3, 110}, {NOTE_E5, 110},
    {NOTE_C3, 110}, {NOTE_C5, 110}, {NOTE_C3, 110}, {NOTE_G4, 110},
    {NOTE_G3, 110}, {NOTE_B4, 110}, {NOTE_G3, 110}, {NOTE_D5, 110},
    {NOTE_G3, 110}, {NOTE_E4, 110}, {NOTE_D4, 110}, {NOTE_C4, 110},
    {NOTE_A3, 220}, {0, 3000},

    {0, 0}};

static void music_advance_index(void)
{
    music_index++;
    if (music_track[music_index].ms == 0U)
        music_index = 0;
}

/* --- OPL2/OPL3 (AdLib) backend for 2-voice music (SB16) --- */
static int opl_present = 0;
static unsigned int opl_addr0 = 0x388;
static unsigned int opl_data0 = 0x389;
static unsigned int opl_addr1 = 0x38A;
static unsigned int opl_data1 = 0x38B;
static int opl_is_opl3 = 0;

static unsigned char opl_last_b0_ch0 = 0;
static unsigned char opl_last_b0_ch1 = 0;
static unsigned char opl_bd_base = 0x20;

static void opl_io_delay_port(unsigned int addr)
{
    /* Short wait for OPL register timing. */
    (void)inp(addr);
    (void)inp(addr);
    (void)inp(addr);
    (void)inp(addr);
    (void)inp(addr);
    (void)inp(addr);
}

static void opl_write_port(unsigned int addr, unsigned int data, unsigned char reg, unsigned char value)
{
    outp(addr, reg);
    opl_io_delay_port(addr);
    outp(data, value);
    opl_io_delay_port(addr);
}

static void opl_write0(unsigned char reg, unsigned char value)
{
    opl_write_port(opl_addr0, opl_data0, reg, value);
}

static void opl_write1(unsigned char reg, unsigned char value)
{
    if (!opl_is_opl3)
        return;
    opl_write_port(opl_addr1, opl_data1, reg, value);
}

static unsigned char opl_read_status_port(unsigned int addr)
{
    return (unsigned char)inp(addr);
}

static int opl_detect_port(unsigned int addr, unsigned int data)
{
    unsigned char s1;
    unsigned char s2;

    /* Standard AdLib/OPL2 detect via timers. */
    opl_write_port(addr, data, 0x01, 0x00);
    opl_write_port(addr, data, 0x04, 0x60);
    opl_write_port(addr, data, 0x04, 0x80);
    s1 = (unsigned char)(opl_read_status_port(addr) & 0xE0);

    opl_write_port(addr, data, 0x02, 0xFF);
    opl_write_port(addr, data, 0x04, 0x21);
    delay(1);
    s2 = (unsigned char)(opl_read_status_port(addr) & 0xE0);

    opl_write_port(addr, data, 0x04, 0x60);
    opl_write_port(addr, data, 0x04, 0x80);

    return (s1 == 0x00) && (s2 == 0xC0);
}

static void opl_note_off(int ch)
{
    if (ch == 0)
    {
        opl_last_b0_ch0 = (unsigned char)(opl_last_b0_ch0 & (unsigned char)~0x20);
        opl_write0((unsigned char)(0xB0 + ch), opl_last_b0_ch0);
        return;
    }

    opl_last_b0_ch1 = (unsigned char)(opl_last_b0_ch1 & (unsigned char)~0x20);
    opl_write0((unsigned char)(0xB0 + ch), opl_last_b0_ch1);
}

static void opl_stop_internal(void)
{
    if (!opl_present)
        return;

    opl_note_off(0);
    opl_note_off(1);

    /* Clear percussion triggers (keep rhythm enabled). */
    opl_write0(0xBD, opl_bd_base);
}

static void opl_calc_fnum_block(unsigned int freq_hz, unsigned int *out_fnum, unsigned char *out_block)
{
    unsigned char b;

    if (freq_hz < 32U)
        freq_hz = 32U;
    if (freq_hz > 5000U)
        freq_hz = 5000U;

    for (b = 0; b < 8; b++)
    {
        unsigned long fnum = ((unsigned long)freq_hz << (20 - b)) / 49716UL;
        if (fnum > 0UL && fnum <= 1023UL)
        {
            *out_fnum = (unsigned int)fnum;
            *out_block = b;
            return;
        }
    }

    *out_fnum = 1023U;
    *out_block = 7;
}

static void opl_note_on(int ch, unsigned int freq_hz)
{
    unsigned int fnum = 0;
    unsigned char block = 0;
    unsigned char b0;

    opl_calc_fnum_block(freq_hz, &fnum, &block);

    b0 = (unsigned char)(((fnum >> 8) & 0x03U) | (unsigned char)(block << 2));

    opl_write0((unsigned char)(0xA0 + ch), (unsigned char)(fnum & 0xFFU));
    opl_write0((unsigned char)(0xB0 + ch), b0);

    b0 = (unsigned char)(b0 | 0x20);
    opl_write0((unsigned char)(0xB0 + ch), b0);

    if (ch == 0)
        opl_last_b0_ch0 = b0;
    else
        opl_last_b0_ch1 = b0;
}

static void opl_program_channel(int ch, unsigned char carrier_tl, unsigned char pan_mask)
{
    static const unsigned char mod_op[9] = {0, 1, 2, 8, 9, 10, 16, 17, 18};
    static const unsigned char car_op[9] = {3, 4, 5, 11, 12, 13, 19, 20, 21};
    unsigned char mod;
    unsigned char car;

    if (ch < 0 || ch > 8)
        return;

    mod = mod_op[ch];
    car = car_op[ch];

    /* Simple sine-ish voice. */
    opl_write0((unsigned char)(0x20 + mod), 0x01);
    opl_write0((unsigned char)(0x20 + car), 0x01);

    /* TL: higher = quieter. */
    opl_write0((unsigned char)(0x40 + mod), 0x20);
    opl_write0((unsigned char)(0x40 + car), (unsigned char)(carrier_tl & 0x3F));

    /* Attack/Decay, Sustain/Release. */
    opl_write0((unsigned char)(0x60 + mod), 0xF3);
    opl_write0((unsigned char)(0x60 + car), 0xF3);
    opl_write0((unsigned char)(0x80 + mod), 0x74);
    opl_write0((unsigned char)(0x80 + car), 0x74);

    /* Waveform select (sine). */
    opl_write0((unsigned char)(0xE0 + mod), 0x00);
    opl_write0((unsigned char)(0xE0 + car), 0x00);

    /* Feedback and algorithm: mild FM. */
    opl_write0((unsigned char)(0xC0 + ch), (unsigned char)(0x04 | (pan_mask & 0x30)));
}

static void opl_set_freq_no_key(int ch, unsigned int freq_hz)
{
    unsigned int fnum = 0;
    unsigned char block = 0;
    unsigned char b0;

    opl_calc_fnum_block(freq_hz, &fnum, &block);

    b0 = (unsigned char)(((fnum >> 8) & 0x03U) | (unsigned char)(block << 2));
    opl_write0((unsigned char)(0xA0 + ch), (unsigned char)(fnum & 0xFFU));
    opl_write0((unsigned char)(0xB0 + ch), b0);
}

static void opl_program_rhythm(void)
{
    /* TL (total level): higher = quieter (0..63). */
#define DRUM_TL_BD_MOD 0x26
#define DRUM_TL_BD_CAR 0x18
#define DRUM_TL_HH     0x24
#define DRUM_TL_SD     0x22
#define DRUM_TL_TT     0x26
#define DRUM_TL_CY     0x2A

    /* OPL2 rhythm-mode percussion mapping:
     * - BD: ch6 ops 16+19
     * - HH: op17
     * - SD: op20
     * - TT: op18
     * - CY: op21
     */

    /* Bass drum (ch6 ops 16/19). */
    opl_write0(0x20 + 16, 0x01);
    opl_write0(0x20 + 19, 0x01);
    opl_write0(0x40 + 16, DRUM_TL_BD_MOD);
    opl_write0(0x40 + 19, DRUM_TL_BD_CAR);
    opl_write0(0x60 + 16, 0xF2);
    opl_write0(0x60 + 19, 0xF2);
    opl_write0(0x80 + 16, 0x86);
    opl_write0(0x80 + 19, 0x86);
    opl_write0(0xE0 + 16, 0x00);
    opl_write0(0xE0 + 19, 0x00);
    opl_write0(0xC0 + 6, 0x30);

    /* Hi-hat (op17). */
    opl_write0(0x20 + 17, 0x01);
    opl_write0(0x40 + 17, DRUM_TL_HH);
    opl_write0(0x60 + 17, 0xF4);
    opl_write0(0x80 + 17, 0x32);
    opl_write0(0xE0 + 17, 0x01);

    /* Snare (op20). */
    opl_write0(0x20 + 20, 0x01);
    opl_write0(0x40 + 20, DRUM_TL_SD);
    opl_write0(0x60 + 20, 0xF4);
    opl_write0(0x80 + 20, 0x24);
    opl_write0(0xE0 + 20, 0x01);

    /* Tom (op18). */
    opl_write0(0x20 + 18, 0x01);
    opl_write0(0x40 + 18, DRUM_TL_TT);
    opl_write0(0x60 + 18, 0xF2);
    opl_write0(0x80 + 18, 0x54);
    opl_write0(0xE0 + 18, 0x00);

    /* Cymbal (op21). */
    opl_write0(0x20 + 21, 0x01);
    opl_write0(0x40 + 21, DRUM_TL_CY);
    opl_write0(0x60 + 21, 0xF2);
    opl_write0(0x80 + 21, 0x34);
    opl_write0(0xE0 + 21, 0x01);

    /* Ch7/8 algorithm/panning. */
    opl_write0(0xC0 + 7, 0x30);
    opl_write0(0xC0 + 8, 0x30);

    /* Default percussion pitches. */
    opl_set_freq_no_key(6, 110U);
    opl_set_freq_no_key(7, 330U);
    opl_set_freq_no_key(8, 196U);

    /* Rhythm enabled, all percussion triggers off. */
    opl_write0(0xBD, opl_bd_base);
}

static unsigned char opl_drums_for_step(unsigned int step)
{
    unsigned int s = (unsigned int)(step & 15U);
    unsigned int phrase = (unsigned int)((step >> 4) & 3U);
    unsigned char bits = 0;

    if (music_drum_mute_start != 0U && step >= music_drum_mute_start)
        return 0;

    /* Hi-hat on 8ths. */
    if ((s & 1U) == 0U)
        bits |= 0x01;

    /* Kick on 1 and 3 (with some syncopation). */
    if (s == 0U || s == 8U || ((phrase == 1U) && (s == 6U || s == 14U)))
        bits |= 0x10;

    /* Snare on 2 and 4. */
    if (s == 4U || s == 12U)
        bits |= 0x08;

    /* Small tom fill near the end of a phrase. */
    if ((phrase == 2U) && (s == 13U || s == 14U))
        bits |= 0x04;

    /* Crash at the end of a 4-bar phrase. */
    if ((phrase == 3U) && (s == 15U))
        bits |= 0x02;

    return bits;
}

static void opl_rhythm_trigger(unsigned char bits)
{
    if (!opl_present)
        return;

    /* Ensure an off->on transition so repeated hits retrigger. */
    opl_write0(0xBD, opl_bd_base);
    if ((bits & 0x1FU) != 0U)
        opl_write0(0xBD, (unsigned char)(opl_bd_base | (bits & 0x1FU)));
}

static int opl_init_internal(void)
{
    if (!opl_detect_port(opl_addr0, opl_data0))
        return 0;

    opl_is_opl3 = opl_detect_port(opl_addr1, opl_data1);

    if (opl_is_opl3)
    {
        /* Enable OPL3 mode (register 0x105 on the 2nd address port). */
        opl_write1(0x05, 0x01);
        opl_write1(0x04, 0x00);
    }

    /* Enable waveform select. */
    opl_write0(0x01, 0x20);
    opl_write1(0x01, 0x20);

    /* Rhythm mode enabled (percussion triggers still off). */
    opl_bd_base = 0x20;
    opl_write0(0xBD, opl_bd_base);

    /* Two channels: ch0 (bass), ch1 (lead). */
    opl_program_channel(0, 0x18, (unsigned char)(opl_is_opl3 ? 0x10 : 0x00));
    opl_program_channel(1, 0x10, (unsigned char)(opl_is_opl3 ? 0x20 : 0x00));
    opl_program_rhythm();

    opl_last_b0_ch0 = 0;
    opl_last_b0_ch1 = 0;
    opl_note_off(0);
    opl_note_off(1);

    return 1;
}

static void opl_play_music_step(unsigned int freq_hz, unsigned int step)
{
    unsigned int other;
    unsigned char drum_bits;

    if (!opl_present)
        return;

    if (freq_hz == 0U)
    {
        opl_note_off(0);
        opl_note_off(1);
    }
    else
    {
        /* Keep the original note as-is, add an octave companion note. */
        if (freq_hz < 260U)
            other = (unsigned int)(freq_hz * 2U);
        else
            other = (unsigned int)(freq_hz / 2U);

        if (other == 0U)
            other = freq_hz;

        opl_note_on(0, freq_hz);
        opl_note_on(1, other);
    }

    drum_bits = opl_drums_for_step(step);
    opl_rhythm_trigger(drum_bits);
}

/* --- Sound Blaster (DSP + 8-bit DMA) backend --- */
static int sb_present = 0;
static unsigned int sb_base_port = 0x220;
static int sb_irq = 5;
static int sb_dma8 = 1;
static int sb_playing = 0;
static unsigned char sb_dsp_major = 0;
static unsigned char sb_dsp_minor = 0;

static unsigned char far *sb_dma_raw = 0;
static unsigned char far *sb_dma_buf = 0;
static unsigned int sb_dma_buf_size = 0;

static void interrupt (*sb_old_isr)(void) = 0;
static unsigned char sb_pic_mask_master = 0xFF;
static unsigned char sb_pic_mask_slave = 0xFF;

static unsigned int sb_irq_int_vector(void)
{
    if (sb_irq < 8)
        return (unsigned int)(sb_irq + 8);
    return (unsigned int)(sb_irq + 0x68);
}

static int sb_dma_page_port(int channel)
{
    switch (channel)
    {
    case 0:
        return 0x87;
    case 1:
        return 0x83;
    case 2:
        return 0x81;
    case 3:
        return 0x82;
    default:
        return -1;
    }
}

static void sb_dsp_write(unsigned char value)
{
    unsigned int spins = 0;

    while ((inp(sb_base_port + 0x0C) & 0x80) != 0)
    {
        if (++spins > 200000U)
            return;
    }

    outp(sb_base_port + 0x0C, value);
}

static int sb_dsp_can_read(void)
{
    return (inp(sb_base_port + 0x0E) & 0x80) != 0;
}

static unsigned char sb_dsp_read(void)
{
    unsigned int spins = 0;

    while (!sb_dsp_can_read())
    {
        if (++spins > 200000U)
            return 0;
    }

    return (unsigned char)inp(sb_base_port + 0x0A);
}

static int sb_reset_and_detect(void)
{
    unsigned int spins = 0;
    unsigned int reads = 0;

    outp(sb_base_port + 0x06, 1);
    delay(3);
    outp(sb_base_port + 0x06, 0);
    delay(3);

    while (spins < 200000U)
    {
        unsigned char v;

        if (!sb_dsp_can_read())
        {
            spins++;
            continue;
        }

        v = (unsigned char)inp(sb_base_port + 0x0A);
        reads++;
        if (v == 0xAA)
            return 1;

        /* Some emulators/clones may have stale bytes pending; keep draining. */
        if (reads > 32U)
            break;
    }

    return 0;
}

static void interrupt sb_isr(void)
{
    /* Acknowledge DSP 8-bit DMA interrupt. */
    (void)inp(sb_base_port + 0x0E);
    if (sb_dsp_can_read())
        (void)inp(sb_base_port + 0x0A);

    sb_playing = 0;

    if (sb_irq >= 8)
        outp(0xA0, 0x20);
    outp(0x20, 0x20);
}

static void sb_pic_unmask_irq(void)
{
    if (sb_irq < 8)
        outp(0x21, sb_pic_mask_master & (unsigned char)~(1U << sb_irq));
    else
    {
        /* Ensure IRQ2 cascade is unmasked on the master PIC. */
        outp(0x21, sb_pic_mask_master & (unsigned char)~(1U << 2));
        outp(0xA1, sb_pic_mask_slave & (unsigned char)~(1U << (sb_irq - 8)));
    }
}

static void sb_pic_restore_masks(void)
{
    outp(0x21, sb_pic_mask_master);
    outp(0xA1, sb_pic_mask_slave);
}

static void sb_dma_setup_8bit(unsigned long phys_addr, unsigned int length)
{
    int channel = sb_dma8;
    unsigned int addr_port;
    unsigned int count_port;
    int page_port;
    unsigned int offset = (unsigned int)(phys_addr & 0xFFFFUL);
    unsigned int page = (unsigned int)((phys_addr >> 16) & 0xFFUL);
    unsigned int count = (length - 1U);

    addr_port = (unsigned int)(channel * 2);
    count_port = (unsigned int)(channel * 2 + 1);
    page_port = sb_dma_page_port(channel);

    /* Mask channel. */
    outp(0x0A, 0x04 | channel);

    /* Clear flip-flop. */
    outp(0x0C, 0x00);

    /* Address. */
    outp(addr_port, (unsigned char)(offset & 0xFF));
    outp(addr_port, (unsigned char)((offset >> 8) & 0xFF));

    /* Page. */
    if (page_port >= 0)
        outp(page_port, (unsigned char)page);

    /* Clear flip-flop. */
    outp(0x0C, 0x00);

    /* Count. */
    outp(count_port, (unsigned char)(count & 0xFF));
    outp(count_port, (unsigned char)((count >> 8) & 0xFF));

    /* Mode: single transfer, address increment, write (memory->device). */
    outp(0x0B, 0x48 | channel);

    /* Unmask channel. */
    outp(0x0A, channel);
}

static void sb_stop_internal(void)
{
    if (!sb_present)
        return;

    /* Halt 8-bit DMA. */
    sb_dsp_write(0xD0);

    /* Mask DMA channel. */
    outp(0x0A, 0x04 | sb_dma8);

    sb_playing = 0;
}

static unsigned long sb_phys_addr(void far *ptr)
{
    return ((unsigned long)FP_SEG(ptr) << 4) + (unsigned long)FP_OFF(ptr);
}

static unsigned char sb_triangle_u8_from_phase(unsigned int phase)
{
    /* phase is 0..65535. Use the top 8 bits as the triangle index. */
    unsigned char t = (unsigned char)(phase >> 8); /* 0..255 */

    if (t < 128U)
        return (unsigned char)(t << 1); /* 0..254 */

    return (unsigned char)((255U - t) << 1); /* 254..0 */
}

static int sb_triangle_s_from_phase(unsigned int phase)
{
    return (int)sb_triangle_u8_from_phase(phase) - 127;
}

static int sb_square_s_from_phase(unsigned int phase, unsigned char duty)
{
    /* duty is 0..255 (~0%..~100%). */
    unsigned char p = (unsigned char)(phase >> 8); /* 0..255 */
    return (p < duty) ? 127 : -127;
}

static unsigned int sb_env_scale(unsigned int i, unsigned int length)
{
    /* Simple linear attack/release to reduce clicks/harshness. Range: 0..256. */
    const unsigned int env_len = 96U;

    if (length == 0U)
        return 256U;

    if (i < env_len)
        return (unsigned int)((i * 256U) / env_len);

    if (i + env_len >= length)
    {
        unsigned int rem = (unsigned int)(length - 1U - i);
        return (unsigned int)((rem * 256U) / env_len);
    }

    return 256U;
}

static void sb_fill_tone_pcm(unsigned char far *dst, unsigned int length, int freq_hz, int sample_rate)
{
    unsigned int i;
    unsigned int phase;
    unsigned int phase_step;
    unsigned int phase_sub;
    unsigned int phase_step_sub;
    unsigned int amp_main;
    unsigned int amp_sub;
    unsigned int sub_freq;
    int use_bass_triangle;

    if (freq_hz <= 0)
        freq_hz = 440;
    if (sample_rate <= 0)
        sample_rate = 11025;

    /* Softer-than-square synth:
       - Bass notes: triangle (rounder)
       - Melody notes: square (brighter) + triangle sub at 1/2 freq
       - Lower overall amplitude + simple envelope */
    use_bass_triangle = (freq_hz < 260);
    if (use_bass_triangle)
    {
        amp_main = 48U;
        amp_sub = 0U;
    }
    else
    {
        amp_main = 20U;
        amp_sub = 12U;
    }

    /* Turbo C friendly: use a 16-bit phase accumulator (wraps at 65536).
       One full cycle is 65536 phase units. */
    phase = 0U;
    phase_sub = 0U;

    phase_step = (unsigned int)(((unsigned long)freq_hz * 65536UL) / (unsigned long)sample_rate);

    sub_freq = (unsigned int)(freq_hz / 2);
    if (sub_freq == 0U)
        sub_freq = (unsigned int)freq_hz;
    phase_step_sub = (unsigned int)(((unsigned long)sub_freq * 65536UL) / (unsigned long)sample_rate);

    if (phase_step == 0U)
        phase_step = 1U;
    if (phase_step_sub == 0U)
        phase_step_sub = 1U;

    for (i = 0; i < length; i++)
    {
        unsigned int env = sb_env_scale(i, length);
        int main_s;
        int sub_s;
        long mixed;
        int sample;

        if (use_bass_triangle)
            main_s = sb_triangle_s_from_phase(phase);
        else
            main_s = sb_square_s_from_phase(phase, 96); /* ~37.5% duty to reduce buzzy edge */

        sub_s = (amp_sub != 0U) ? sb_triangle_s_from_phase(phase_sub) : 0;

        mixed = (long)main_s * (long)amp_main + (long)sub_s * (long)amp_sub;
        mixed = (mixed * (long)env) / (long)256;
        mixed = mixed / 64L; /* scale back down to sane range */

        sample = 128 + (int)mixed;
        if (sample < 0)
            sample = 0;
        if (sample > 255)
            sample = 255;

        dst[i] = (unsigned char)sample;

        phase = (unsigned int)(phase + phase_step);
        phase_sub = (unsigned int)(phase_sub + phase_step_sub);
    }
}

static int sb_play_tone(int freq, int ms)
{
    const int sample_rate = 11025;
    unsigned int length;
    unsigned int time_constant;
    unsigned long phys;

    if (!sb_present)
        return 0;

    if (ms <= 0)
        return 0;

    length = (unsigned int)(((unsigned long)ms * (unsigned long)sample_rate) / 1000UL);
    if (length < 32U)
        length = 32U;
    if (length > sb_dma_buf_size)
        length = sb_dma_buf_size;

    sb_fill_tone_pcm(sb_dma_buf, length, freq, sample_rate);
    phys = sb_phys_addr(sb_dma_buf);

    sb_stop_internal();
    sb_dma_setup_8bit(phys, length);

    /* Speaker on. */
    sb_dsp_write(0xD1);

    /* Time constant for SB 1.x/2.0. */
    time_constant = (unsigned int)(256U - (1000000UL / (unsigned long)sample_rate));
    sb_dsp_write(0x40);
    sb_dsp_write((unsigned char)time_constant);

    /* 8-bit single-cycle DMA output. */
    sb_dsp_write(0x14);
    sb_dsp_write((unsigned char)((length - 1U) & 0xFF));
    sb_dsp_write((unsigned char)(((length - 1U) >> 8) & 0xFF));

    sb_playing = 1;
    return 1;
}

static int sb_parse_blaster_env(void)
{
    const char *env = getenv("BLASTER");
    const char *p;

    if (!env || !*env)
        return 0;

    p = env;
    while (*p)
    {
        while (*p == ' ' || *p == '\t')
            p++;

        if (*p == 'A' || *p == 'a')
        {
            p++;
            sb_base_port = (unsigned int)strtoul(p, (char **)&p, 16);
            continue;
        }
        if (*p == 'I' || *p == 'i')
        {
            p++;
            sb_irq = (int)strtoul(p, (char **)&p, 10);
            continue;
        }
        if (*p == 'D' || *p == 'd')
        {
            p++;
            sb_dma8 = (int)strtoul(p, (char **)&p, 10);
            continue;
        }

        while (*p && *p != ' ' && *p != '\t')
            p++;
    }

    return 1;
}

static int sb_init_internal(void)
{
    unsigned int vec;
    unsigned int want_size = 8192U;
    unsigned int padded;
    unsigned long phys;
    unsigned int low16;
    int blaster_present;
    static const unsigned int common_ports[] = {0x220, 0x240, 0x260, 0x280};
    unsigned int i;

    blaster_present = sb_parse_blaster_env();

    if (sb_dma8 < 0 || sb_dma8 > 3)
        sb_dma8 = 1;
    if (sb_irq < 2 || sb_irq > 15)
        sb_irq = 5;

    if (!sb_reset_and_detect())
    {
        /* If BLASTER is wrong (or missing), try common SB base ports. */
        for (i = 0; i < (unsigned int)(sizeof(common_ports) / sizeof(common_ports[0])); i++)
        {
            if (blaster_present && sb_base_port == common_ports[i])
                continue;

            sb_base_port = common_ports[i];
            if (sb_reset_and_detect())
                break;
        }

        if (!sb_reset_and_detect())
            return 0;
    }

    /* DSP version. */
    sb_dsp_write(0xE1);
    sb_dsp_major = sb_dsp_read();
    sb_dsp_minor = sb_dsp_read();

    /* Allocate a small DMA buffer and ensure it won't cross a 64K boundary. */
    padded = want_size + 32U;
    sb_dma_raw = (unsigned char far *)farmalloc((unsigned long)padded);
    if (!sb_dma_raw)
        return 0;

    phys = sb_phys_addr(sb_dma_raw);
    low16 = (unsigned int)(phys & 0xFFFFUL);
    if ((unsigned int)(low16 + want_size) < low16 || (unsigned int)(low16 + want_size) >= 0x10000U)
    {
        unsigned int delta = (unsigned int)(0x10000U - low16);
        sb_dma_buf = sb_dma_raw + delta;
    }
    else
    {
        sb_dma_buf = sb_dma_raw;
    }

    sb_dma_buf_size = want_size;

    /* Save PIC masks and hook IRQ handler. */
    sb_pic_mask_master = (unsigned char)inp(0x21);
    sb_pic_mask_slave = (unsigned char)inp(0xA1);

    vec = sb_irq_int_vector();
    disable();
    sb_old_isr = getvect(vec);
    setvect(vec, sb_isr);
    sb_pic_unmask_irq();
    enable();

    sb_present = 1;
    return 1;
}

static void sb_shutdown_internal(void)
{
    unsigned int vec;

    if (!sb_present)
        return;

    sb_stop_internal();

    vec = sb_irq_int_vector();
    disable();
    if (sb_old_isr)
        setvect(vec, sb_old_isr);
    sb_pic_restore_masks();
    enable();

    if (sb_dma_raw)
    {
        farfree(sb_dma_raw);
        sb_dma_raw = 0;
        sb_dma_buf = 0;
        sb_dma_buf_size = 0;
    }

    sb_present = 0;
}

void audio_stop_internal(void)
{
    if (audio_backend == AUDIO_BACKEND_SOUNDBLASTER)
        sb_stop_internal();
    opl_stop_internal();
    nosound();
    audio_active = 0;
    audio_end_clock = 0;
    tone_source = TONE_NONE;
}

static void audio_start_tone_internal(int freq, int ms, ToneSource source)
{
    clock_t now;
    clock_t ticks;
    ToneSource prev_source = tone_source;

    if (!audio_enabled)
        return;

    if (ms <= 0)
        return;

    if (audio_backend == AUDIO_BACKEND_SOUNDBLASTER && opl_present && prev_source == TONE_MUSIC && source != TONE_MUSIC)
        opl_stop_internal();

    now = clock();

    ticks = (clock_t)(((long)ms * (long)CLK_TCK + 999L) / 1000L);
    if (ticks <= 0)
        ticks = 1;

    audio_end_clock = now + ticks;
    audio_active = 1;
    tone_source = source;

    if (source == TONE_SILENCE || freq <= 0)
    {
        if (audio_backend == AUDIO_BACKEND_SOUNDBLASTER && opl_present && source == TONE_SILENCE)
        {
            /* Music rest: keep the groove going with OPL percussion. */
            sb_stop_internal();
            opl_play_music_step(0U, music_index);
            nosound();
            return;
        }

        if (audio_backend == AUDIO_BACKEND_SOUNDBLASTER)
            sb_stop_internal();
        opl_stop_internal();
        nosound();
        return;
    }

    if (audio_backend == AUDIO_BACKEND_SOUNDBLASTER && sb_present)
    {
        if ((source == TONE_MUSIC || source == TONE_SILENCE) && opl_present)
            opl_play_music_step((freq > 0) ? (unsigned int)freq : 0U, music_index);
        else
            (void)sb_play_tone(freq, ms);
        return;
    }

    sound(freq);
}

static void audio_play_tone(int freq, int ms)
{
    if (!audio_enabled)
        return;

    if (music_running && (tone_source == TONE_MUSIC || tone_source == TONE_SILENCE))
        music_advance_index();

    audio_start_tone_internal(freq, ms, TONE_SFX);
}

static void audio_play_silence(int ms, ToneSource source)
{
    if (!audio_enabled)
        return;

    if (ms <= 0)
        return;

    audio_start_tone_internal(0, ms, source);
}

static void music_start_next_note(void)
{
    MusicNote n;

    if (!audio_enabled || !music_enabled || !music_running || audio_active)
        return;

    n = music_track[music_index];
    if (n.ms == 0U)
    {
        music_index = 0;
        n = music_track[music_index];
    }

    if (n.freq == 0U)
        audio_play_silence((int)n.ms, TONE_SILENCE);
    else
        audio_start_tone_internal((int)n.freq, (int)n.ms, TONE_MUSIC);
}

void far audio_init(void)
{
    unsigned int i;
    unsigned int acc_ms;

    audio_enabled = 1;
    music_enabled = 1;
    music_running = 1;
    music_index = 0;

    music_len = 0;
    while (music_track[music_len].ms != 0U)
        music_len++;

    /* Mute drums for ~3000ms at the very end of the loop. */
    music_drum_mute_start = 0;
    acc_ms = 0;
    i = music_len;
    while (i > 0U && acc_ms < 3000U)
    {
        i--;
        acc_ms += music_track[i].ms;
    }
    if (i > 0U)
        music_drum_mute_start = i;
    tone_source = TONE_NONE;

    /* Try to enable Sound Blaster output; fall back to PC speaker. */
    sb_present = 0;
    opl_present = 0;
    audio_backend = AUDIO_BACKEND_SPEAKER;
    if (sb_init_internal()) {
	audio_backend = AUDIO_BACKEND_SOUNDBLASTER;
        opl_present = opl_init_internal();
    }

    audio_stop_internal();
}

void far audio_shutdown(void)
{
    audio_stop_internal();
    opl_present = 0;
    sb_shutdown_internal();
}

int far audio_is_enabled(void)
{
    return audio_enabled;
}

void far audio_toggle(void)
{
    audio_enabled = !audio_enabled;
    if (!audio_enabled)
        audio_stop_internal();
}

int far audio_music_is_enabled(void)
{
    return music_enabled;
}

void far audio_music_restart(void)
{
    music_index = 0;
    music_running = 1;

    if (!audio_enabled || !music_enabled)
        return;

    audio_stop_internal();
    music_start_next_note();
}

void far audio_music_stop(void)
{
    music_running = 0;

    if (tone_source == TONE_MUSIC || tone_source == TONE_SILENCE)
        audio_stop_internal();
}

void far audio_music_toggle(void)
{
    music_enabled = !music_enabled;
    if (!music_enabled && (tone_source == TONE_MUSIC || tone_source == TONE_SILENCE))
        audio_stop_internal();
}

void far audio_update(void)
{
    clock_t now = clock();

    if (audio_active && now >= audio_end_clock)
    {
        if (tone_source == TONE_MUSIC || tone_source == TONE_SILENCE)
            music_advance_index();
        audio_stop_internal();
    }

    music_start_next_note();
}

void far audio_event_paddle(void)
{
    audio_play_tone(1200, 45);
}

void far audio_event_wall(void)
{
    audio_play_tone(800, 35);
}

void far audio_event_brick(int row)
{
    int freq = 1500 - row * 140;
    if (freq < 400)
        freq = 400;
    audio_play_tone(freq, 55);
}

void far audio_event_life_lost_blocking(void)
{
    if (!audio_enabled)
        return;

    audio_stop_internal();

    if (audio_backend == AUDIO_BACKEND_SOUNDBLASTER && sb_present)
    {
        (void)sb_play_tone(900, 110);
        delay(120);
        (void)sb_play_tone(650, 120);
        delay(130);
        (void)sb_play_tone(450, 170);
        delay(180);
        sb_stop_internal();
        return;
    }

    sound(900);
    delay(110);
    sound(650);
    delay(120);
    sound(450);
    delay(170);
    nosound();
}

void far audio_event_life_up_blocking(void)
{
    if (!audio_enabled)
        return;

    audio_stop_internal();

    if (audio_backend == AUDIO_BACKEND_SOUNDBLASTER && sb_present)
    {
        (void)sb_play_tone(659, 70);  /* E */
        delay(80);
        (void)sb_play_tone(784, 70);  /* G */
        delay(80);
        (void)sb_play_tone(880, 70);  /* A */
        delay(80);
        (void)sb_play_tone(1047, 80); /* C */
        delay(90);
        (void)sb_play_tone(1175, 80); /* D */
        delay(90);
        (void)sb_play_tone(1568, 120); /* G */
        delay(130);
        sb_stop_internal();
        return;
    }

    sound(659);
    delay(70);
    sound(784);
    delay(70);
    sound(880);
    delay(70);
    sound(1047);
    delay(80);
    sound(1175);
    delay(80);
    sound(1568);
    delay(120);
    nosound();
}

void far audio_event_level_clear_blocking(void)
{
    if (!audio_enabled)
        return;

    audio_stop_internal();

    if (audio_backend == AUDIO_BACKEND_SOUNDBLASTER && sb_present)
    {
        (void)sb_play_tone(660, 90);
        delay(100);
        (void)sb_play_tone(880, 90);
        delay(100);
        (void)sb_play_tone(990, 110);
        delay(120);
        (void)sb_play_tone(1320, 140);
        delay(150);
        sb_stop_internal();
        return;
    }

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
