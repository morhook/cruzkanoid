#include <dos.h>

#include "MUSIC.H"

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

static void music_advance_index(void)
{
    music_index++;
    if (music_track[music_index].ms == 0U)
        music_index = 0;
}

static void opl_io_delay_port(unsigned int addr)
{
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

    opl_write0((unsigned char)(0x20 + mod), 0x01);
    opl_write0((unsigned char)(0x20 + car), 0x01);

    opl_write0((unsigned char)(0x40 + mod), 0x20);
    opl_write0((unsigned char)(0x40 + car), (unsigned char)(carrier_tl & 0x3F));

    opl_write0((unsigned char)(0x60 + mod), 0xF3);
    opl_write0((unsigned char)(0x60 + car), 0xF3);
    opl_write0((unsigned char)(0x80 + mod), 0x74);
    opl_write0((unsigned char)(0x80 + car), 0x74);

    opl_write0((unsigned char)(0xE0 + mod), 0x00);
    opl_write0((unsigned char)(0xE0 + car), 0x00);

    opl_write0((unsigned char)(0xC0 + ch), (unsigned char)(0x04 | (pan_mask & 0x30)));
}

static void opl_set_guitar(int chan)
{
    int op_offset[] = {0x00, 0x01, 0x02, 0x08, 0x09, 0x0A, 0x10, 0x11, 0x12};
    int mod = op_offset[chan];
    int car = mod + 3;

    opl_write0(0xC0 + chan, 0x0B);

    opl_write0(0x40 + mod, 0x02);
    opl_write0(0x60 + mod, 0x88);
    opl_write0(0x80 + mod, 0x00);
    opl_write0(0x20 + mod, 0x20);

    opl_write0(0x20 + car, 0x01);
    opl_write0(0x40 + car, 0x0F);
    opl_write0(0x60 + car, 0xF8);
    opl_write0(0x80 + car, 0x00);

    opl_write0((unsigned char)(0xE0 + mod), 0x00);
    opl_write0((unsigned char)(0xE0 + car), 0x00);
}

static void opl_set_freq_no_key(int ch, unsigned int freq_hz)
{
    unsigned int fnum = 0;
    unsigned char block = 0;
    unsigned char b0;
    unsigned char b;

    if (freq_hz < 32U)
        freq_hz = 32U;
    if (freq_hz > 5000U)
        freq_hz = 5000U;

    for (b = 0; b < 8; b++)
    {
        unsigned long n = ((unsigned long)freq_hz << (20 - b)) / 49716UL;
        if (n > 0UL && n <= 1023UL)
        {
            fnum = (unsigned int)n;
            block = b;
            break;
        }
    }

    if (fnum == 0U)
    {
        fnum = 1023U;
        block = 7;
    }

    b0 = (unsigned char)(((fnum >> 8) & 0x03U) | (unsigned char)(block << 2));
    opl_write0((unsigned char)(0xA0 + ch), (unsigned char)(fnum & 0xFFU));
    opl_write0((unsigned char)(0xB0 + ch), b0);
}

static void opl_program_rhythm(void)
{
#define DRUM_TL_BD_MOD 0x28
#define DRUM_TL_BD_CAR 0x16
#define DRUM_TL_HH 0x63
#define DRUM_TL_SD 0x1C
#define DRUM_TL_TT 0x28
#define DRUM_TL_CY 0x30

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

    opl_write0(0x20 + 17, 0x01);
    opl_write0(0x40 + 17, DRUM_TL_HH);
    opl_write0(0x60 + 17, 0xF6);
    opl_write0(0x80 + 17, 0x32);
    opl_write0(0xE0 + 17, 0x06);

    opl_write0(0x20 + 20, 0x01);
    opl_write0(0x40 + 20, DRUM_TL_SD);
    opl_write0(0x60 + 20, 0xF4);
    opl_write0(0x80 + 20, 0x24);
    opl_write0(0xE0 + 20, 0x01);

    opl_write0(0x20 + 18, 0x01);
    opl_write0(0x40 + 18, DRUM_TL_TT);
    opl_write0(0x60 + 18, 0xF2);
    opl_write0(0x80 + 18, 0x54);
    opl_write0(0xE0 + 18, 0x00);

    opl_write0(0x20 + 21, 0x01);
    opl_write0(0x40 + 21, DRUM_TL_CY);
    opl_write0(0x60 + 21, 0xF2);
    opl_write0(0x80 + 21, 0x34);
    opl_write0(0xE0 + 21, 0x01);

    opl_write0(0xC0 + 7, 0x30);
    opl_write0(0xC0 + 8, 0x30);

    opl_set_freq_no_key(6, 110U);
    opl_set_freq_no_key(7, 330U);
    opl_set_freq_no_key(8, 196U);

    opl_write0(0xBD, opl_bd_base);
}

static unsigned char opl_drums_for_step(unsigned int step)
{
    unsigned int s = (unsigned int)(step & 15U);
    unsigned int phrase = (unsigned int)((step >> 4) & 3U);
    unsigned char bits = 0;

    if (music_drum_mute_start != 0U && step >= music_drum_mute_start)
        return 0;

    if ((s & 1U) == 0U)
        bits |= 0x01;

    if (s == 0U || s == 8U || ((phrase == 1U) && (s == 6U || s == 14U)))
        bits |= 0x10;

    if (s == 4U || s == 12U)
        bits |= 0x08;

    if ((phrase == 2U) && (s == 13U || s == 14U))
        bits |= 0x04;

    if ((phrase == 3U) && (s == 15U))
        bits |= 0x02;

    return bits;
}

static void opl_rhythm_trigger(unsigned char bits)
{
    if (!opl_present)
        return;

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
        opl_write1(0x05, 0x01);
        opl_write1(0x04, 0x00);
    }

    opl_write0(0x01, 0x20);
    opl_write1(0x01, 0x20);

    opl_bd_base = 0x20;
    opl_write0(0xBD, opl_bd_base);

    opl_set_guitar(0);
    opl_set_guitar(1);
    opl_program_rhythm();

    opl_last_b0_ch0 = 0;
    opl_last_b0_ch1 = 0;
    opl_note_off(0);
    opl_note_off(1);

    return 1;
}

static void opl_stop_internal(void)
{
    if (!opl_present)
        return;

    opl_note_off(0);
    opl_note_off(1);
    opl_program_channel(0, 0x18, (unsigned char)(opl_is_opl3 ? 0x10 : 0x00));
    opl_program_channel(1, 0x10, (unsigned char)(opl_is_opl3 ? 0x20 : 0x00));
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

static void opl_play_music_step(unsigned int freq_hz, unsigned int step)
{
    unsigned int other;
    unsigned char drum_bits;

    if (!opl_present)
        return;

    if (freq_hz == 0U)
    {
        if (step == 3000U)
        {
            opl_program_channel(0, 0x18, (unsigned char)(opl_is_opl3 ? 0x10 : 0x00));
            opl_program_channel(1, 0x10, (unsigned char)(opl_is_opl3 ? 0x20 : 0x00));
        }
        opl_note_off(0);
        opl_note_off(1);
    }
    else
    {
        opl_set_guitar(0);
        opl_set_guitar(1);

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

void far music_init(void)
{
    unsigned int i;
    unsigned int acc_ms;

    music_enabled = 1;
    music_running = 1;
    music_index = 0;

    music_len = 0;
    while (music_track[music_len].ms != 0U)
        music_len++;

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
}

void far music_shutdown(void)
{
    music_running = 0;
    music_enabled = 0;
    music_index = 0;
}

int far music_backend_init(int sb_is_present)
{
    opl_present = 0;
    if (!sb_is_present)
        return 0;

    opl_present = opl_init_internal();
    return opl_present;
}

void far music_backend_shutdown(void)
{
    opl_stop_internal();
    opl_present = 0;
}

int far music_backend_has_opl(void)
{
    return opl_present;
}

void far music_backend_stop(void)
{
    opl_stop_internal();
}

void far music_backend_play_step(unsigned int freq_hz)
{
    opl_play_music_step(freq_hz, music_index);
}

int far music_is_enabled(void)
{
    return music_enabled;
}

void far music_toggle(void)
{
    music_enabled = !music_enabled;
}

void far music_restart(void)
{
    music_index = 0;
    music_running = 1;
}

void far music_stop(void)
{
    music_running = 0;
}

void far music_on_note_finished(void)
{
    if (music_running)
        music_advance_index();
}

void far music_on_sfx_preempt(int preempted_music_tone)
{
    if (preempted_music_tone && music_running)
        music_advance_index();
}

int far music_prepare_next_note(int audio_enabled,
                                int audio_active,
                                int life_up_active,
                                int multiball_active,
                                unsigned int *out_freq,
                                unsigned int *out_ms)
{
    MusicNote n;

    if (!out_freq || !out_ms)
        return 0;

    if (!audio_enabled || !music_enabled || !music_running || audio_active || life_up_active || multiball_active)
        return 0;

    n = music_track[music_index];
    if (n.ms == 0U)
    {
        music_index = 0;
        n = music_track[music_index];
    }

    *out_freq = n.freq;
    *out_ms = n.ms;
    return 1;
}
