#ifndef MUSIC_H
#define MUSIC_H

#pragma once

/*
Copyright (c) 2009-2011 Tero Lindeman (kometbomb)

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#include "cyd.h"
#include "cydfx.h"
#include <stdio.h>

#include "../../../src/wavegen.h"

#include "music_defs.h"

#define MUS_PROG_LEN 255
#define MUS_MAX_CHANNELS CYD_MAX_CHANNELS

#define MUS_VERSION 33

#define MUS_SONG_TITLE_LEN 255
#define MUS_INSTRUMENT_NAME_LEN 255
#define MUS_WAVETABLE_NAME_LEN MUS_INSTRUMENT_NAME_LEN

#define MUS_MAX_COMMANDS 8

typedef unsigned long char32_t;

typedef struct
{
	Uint8 a, d, s, r; // 0-63 for a, d, r; 0-31 for s
} MusAdsr;

typedef struct
{
	Uint32 flags;
	Uint32 cydflags;
	MusAdsr adsr;
	Uint8 sync_source, ring_mod; // 0xff == self
	Uint16 pw;
	Uint8 volume;
	
	Uint8 vol_ksl_level; //wasn't there
	Uint8 env_ksl_level;
	
	Uint8 mixmode; //wasn't there
	Uint8 slope;
	
	Uint16 program[MUS_PROG_LEN];
	Uint8 program_unite_bits[MUS_PROG_LEN / 8 + 1];
	
	Uint8 prog_period; 
	Uint8 vibrato_speed, vibrato_depth, slide_speed, pwm_speed, pwm_depth;
	
	Uint8 tremolo_speed, tremolo_delay, tremolo_shape, tremolo_depth; //wasn't there
	Uint8 pwm_delay;
	
	Uint8 fm_vibrato_speed, fm_vibrato_delay, fm_vibrato_shape, fm_vibrato_depth; //wasn't there
	Uint8 fm_tremolo_speed, fm_tremolo_delay, fm_tremolo_shape, fm_tremolo_depth; //wasn't there
	
	Uint8 base_note;
	
	Uint8 noise_note; //wasn't there
	
	Uint16 cutoff;
	Uint8 resonance; //was 0-3, now 0-15
	Uint8 flttype;
	Uint8 ym_env_shape;
	Sint16 buzz_offset;
	Uint8 fx_bus, vibrato_shape, vibrato_delay, pwm_shape;
	char name[MUS_INSTRUMENT_NAME_LEN + 1];
	
	Uint8 wavetable_entry;
	
	Uint8 morph_wavetable_entry;
	Uint8 morph_speed, morph_shape, morph_delay;
	
	Uint8 lfsr_type;
	Sint8 finetune;
	Uint32 fm_flags;
	Uint8 fm_modulation, fm_feedback, fm_wave, fm_harmonic, fm_freq_LUT; //last wasn't there
	MusAdsr fm_adsr;
	
	Uint8 fm_vol_ksl_level; //wasn't there
	Uint8 fm_env_ksl_level; //wasn't there
	
	Uint8 fm_attack_start;
	
	Uint8 fm_base_note; //weren't there
	Sint8 fm_finetune; //wasn't there

} MusInstrument;

enum
{
	MUS_INST_PROG_SPEED_RELATIVE = 0, // chn.current_tick / mus.tick_period * ins.prog_period
	MUS_INST_PROG_SPEED_ABSOLUTE = 1, // absolute number of ticks
	MUS_INST_DRUM = 2,
	MUS_INST_INVERT_VIBRATO_BIT = 4,
	MUS_INST_LOCK_NOTE = 8,
	MUS_INST_SET_PW = 16,
	MUS_INST_SET_CUTOFF = 32,
	MUS_INST_YM_BUZZ = 64,
	MUS_INST_RELATIVE_VOLUME = 128,
	MUS_INST_QUARTER_FREQ = 256,
	MUS_INST_WAVE_LOCK_NOTE = 512,
	MUS_INST_NO_PROG_RESTART = 1024,
	MUS_INST_MULTIOSC = 2048,
	
	MUS_INST_SAVE_LFO_SETTINGS = 4096,
	MUS_INST_INVERT_TREMOLO_BIT = 8192,
};

enum
{
	MUS_FX_WAVE_NOISE = 1,
	MUS_FX_WAVE_PULSE = 2,
	MUS_FX_WAVE_TRIANGLE = 4,
	MUS_FX_WAVE_SAW = 8,
	MUS_FX_WAVE_LFSR = 16,
	MUS_FX_WAVE_WAVE = 32,
};

typedef struct
{
	MusInstrument *instrument;
	Uint32 note;
	
	Uint8 noise_note;
	
	Uint8 volume;
	// ------
	Uint8 arpeggio_note;
	Uint32 target_note, last_note, fixed_note;
	volatile Uint32 flags;
	Uint32 current_tick;
	Uint8 program_counter, program_tick, program_loop, prog_period;
	Sint16 buzz_offset;
} MusChannel;

typedef struct
{
	Uint8 note, instrument, ctrl;
	//Uint16 command;
	Uint8 volume;
	
	Uint16 command[MUS_MAX_COMMANDS];
} MusStep;

typedef struct
{
	Uint8 note[MUS_FM_NUM_OPS];
	Uint8 instrument[MUS_FM_NUM_OPS];
	Uint8 ctrl[MUS_FM_NUM_OPS];
	Uint8 volume[MUS_FM_NUM_OPS];
	
	Uint16 commands[MUS_FM_NUM_OPS][MUS_MAX_COMMANDS];
} Mus4opStep;

typedef struct
{
	Uint16 position; 
	Uint16 pattern;
	Sint8 note_offset;
	
	bool is_pattern_there;
} MusSeqPattern;

typedef struct
{
	MusStep *step;
	Uint16 num_steps;
	Uint8 color;
	
	Uint8 command_columns; //wasn't there
	
	bool is_expanded_4op;
	
	Mus4opStep *four_op_step;
	
} MusPattern;

typedef struct
{
	MusInstrument *instrument;
	Uint8 num_instruments;
	MusPattern *pattern;
	Uint16 num_patterns;
	MusSeqPattern *sequence[MUS_MAX_CHANNELS];
	Uint16 num_sequences[MUS_MAX_CHANNELS];
	Uint16 song_length, loop_point;
	Uint8 song_speed, song_speed2;
	Uint16 song_rate;
	Uint16 time_signature, sequence_step;
	Uint32 flags;
	Uint8 num_channels;
	Uint8 multiplex_period, pitch_inaccuracy;
	char title[MUS_SONG_TITLE_LEN + 1];
	
	//char32_t song_info[8388608]; //wasn't there
	
	CydFxSerialized fx[CYD_MAX_FX_CHANNELS];
	Uint8 master_volume;
	Uint8 default_volume[MUS_MAX_CHANNELS];
	Sint8 default_panning[MUS_MAX_CHANNELS];
	char **wavetable_names;
	int num_wavetables;
} MusSong;


typedef struct
{
	MusPattern *pattern;
	Uint8 last_ctrl;
	Uint16 pw, pattern_step, sequence_position, slide_speed;
	Uint16 vibrato_position, pwm_position, tremolo_position;
	Sint8 note_offset;
	Uint16 filter_cutoff;
	Uint8 filter_resonance;
	Uint8 filter_slope; //wasn't there
	Uint8 extarp1, extarp2;
	Uint16 volume, fm_volume; //was Uint8
	Uint8 vibrato_delay;
	
	Uint8 pwm_delay, tremolo_delay; //wasn't there
	
	Uint8 fm_tremolo_delay, fm_vibrato_delay;
	
	Uint8 tremolo_speed, tremolo_depth; //wasn't there
	Uint8 vibrato_speed, vibrato_depth;
	Uint8 pwm_speed, pwm_depth; //wasn't there
	
	Uint8 fm_tremolo_speed, fm_tremolo_depth, fm_tremolo_shape; //wasn't there
	Uint8 fm_vibrato_speed, fm_vibrato_depth, fm_vibrato_shape; //wasn't there
	Uint16 fm_vibrato_position, fm_tremolo_position; //wasn't there
} MusTrackStatus;

typedef struct
{
	MusChannel channel[MUS_MAX_CHANNELS];
	Uint8 tick_period; // 1 = at the rate this is polled
	// ----
	MusTrackStatus song_track[MUS_MAX_CHANNELS];
	MusSong *song;
	Uint8 song_counter;
	int song_position; //was Uint16 song_position;
	CydEngine *cyd;
	Uint8 current_tick;
	Uint8 volume, play_volume; // 0..128
	Uint8 multiplex_ctr;
	Uint32 flags;
	Uint32 ext_sync_ticks;
	Uint32 pitch_mask;
} MusEngine;


enum
{
	MUS_CHN_PLAYING = 1,
	MUS_CHN_PROGRAM_RUNNING = 2,
	MUS_CHN_DISABLED = 4
};

enum
{
	MUS_NOTE_NONE = 0xff,
	MUS_NOTE_RELEASE = 0xfe
};

enum
{
	MUS_PAK_BIT_NOTE = 1,
	MUS_PAK_BIT_INST = 2,
	MUS_PAK_BIT_CTRL = 4,
	MUS_PAK_BIT_CMD = 8,
	/* -- these go in ctrl byte -- */
	MUS_PAK_BIT_NEW_CMDS_1 = 16,
	MUS_PAK_BIT_NEW_CMDS_2 = 32,
	MUS_PAK_BIT_NEW_CMDS_3 = 64,
	
	MUS_PAK_BIT_VOLUME = 128
};

enum
{
	MUS_EXT_SYNC = 1
};

#define MUS_NOTE_VOLUME_SET_PAN 0xa0
#define MUS_NOTE_VOLUME_PAN_LEFT 0xb0
#define MUS_NOTE_VOLUME_PAN_RIGHT 0xc0
#define MUS_NOTE_VOLUME_FADE_UP 0xe0
#define MUS_NOTE_VOLUME_FADE_DN 0xd0
#define MUS_NOTE_NO_VOLUME 0xff
#define MUS_NOTE_NO_INSTRUMENT 0xff
#define MUS_CTRL_BIT 1
#define MAX_VOLUME 128

enum
{
	MUS_FX_ARPEGGIO = 0x0000,
	
	MUS_FX_SET_NOISE_CONSTANT_PITCH = 0x3000, //wasn't there
	MUS_FX_SET_NOISE_MODE = 0x0e50, //wasn't there
	
	MUS_FX_ARPEGGIO_ABS = 0x4000,
	MUS_FX_SET_EXT_ARP = 0x1000,
	MUS_FX_PORTA_UP = 0x0100,
	MUS_FX_PORTA_DN = 0x0200,
	
	MUS_FX_FM_PORTA_UP = 0x4800, //wasn't there
	MUS_FX_FM_PORTA_DN = 0x4900, //wasn't there
	
	MUS_FX_PORTA_UP_LOG = 0x0500,
	MUS_FX_PORTA_DN_LOG = 0x0600,
	MUS_FX_SLIDE = 0x0300,
	MUS_FX_VIBRATO = 0x0400,
	MUS_FX_TREMOLO = 0x2400, //wasn't there
	MUS_FX_PWM = 0x2500, //wasn't there
	MUS_FX_SWEEP = 0x2600, //wasn't there //26xy filter sweep, by default unlooped saw LFO with speed x and depth y
	MUS_FX_FM_VIBRATO = 0x2700, //wasn't there
	MUS_FX_FM_TREMOLO = 0x2800, //wasn't there
	MUS_FX_FADE_VOLUME = 0x0a00,
	
	MUS_FX_FM_FADE_VOLUME = 0x4a00, //wasn't there
	MUS_FX_FM_EXT_FADE_VOLUME_DN = 0x34a0, //wasn't there
	MUS_FX_FM_EXT_FADE_VOLUME_UP = 0x34b0, //wasn't there
	
	MUS_FX_SET_EXPONENTIALS = 0x0e40, //wasn't there
	MUS_FX_FM_SET_EXPONENTIALS = 0x34c0, //wasn't there
	
	MUS_FX_SET_VOLUME = 0x0c00,
	MUS_FX_LOOP_PATTERN = 0x0d00,
	MUS_FX_SKIP_PATTERN = 0x2d00,
	MUS_FX_EXT = 0x0e00,
	MUS_FX_EXT_PORTA_UP = 0x0e10,
	MUS_FX_EXT_PORTA_DN = 0x0e20,
	MUS_FX_EXT_RETRIGGER = 0x0e90,
	MUS_FX_EXT_FADE_VOLUME_DN = 0x0ea0,
	MUS_FX_EXT_FADE_VOLUME_UP = 0x0eb0,
	MUS_FX_EXT_NOTE_CUT = 0x0ec0,
	MUS_FX_EXT_NOTE_DELAY = 0x0ed0,
	MUS_FX_SET_SPEED = 0x0f00,
	MUS_FX_SET_SPEED1 = 0x4100, //wasn't there
	MUS_FX_SET_SPEED2 = 0x4200, //wasn't there
	MUS_FX_SET_RATE = 0x1f00,
	MUS_FX_SET_RATE_HIGHER_BYTE = 0x4300, //wasn't there
	MUS_FX_PORTA_UP_SEMI = 0x1100,
	MUS_FX_PORTA_DN_SEMI = 0x1200,
	MUS_FX_SET_PANNING = 0x1800,
	MUS_FX_PAN_LEFT = 0x1700,
	MUS_FX_PAN_RIGHT = 0x1900,
	MUS_FX_FADE_GLOBAL_VOLUME = 0x1a00,
	MUS_FX_SET_GLOBAL_VOLUME = 0x1d00,
	MUS_FX_SET_CHANNEL_VOLUME = 0x1c00,
	MUS_FX_SET_VOL_KSL_LEVEL = 0x3700, //wasn't there
	MUS_FX_SET_FM_VOL_KSL_LEVEL = 0x3800, //wasn't there
	MUS_FX_SET_ENV_KSL_LEVEL = 0x3c00, //wasn't there
	MUS_FX_SET_FM_ENV_KSL_LEVEL = 0x3d00, //wasn't there
	MUS_FX_CUTOFF_UP = 0x2100,
	MUS_FX_CUTOFF_DN = 0x2200,
	MUS_FX_CUTOFF_SET = 0x2900,
	MUS_FX_RESONANCE_SET = 0x2a00,
	MUS_FX_FILTER_TYPE = 0x2b00,
	MUS_FX_FILTER_SLOPE = 0x0e30, //wasn't there
	MUS_FX_CUTOFF_SET_COMBINED = 0x2c00,
	MUS_FX_BUZZ_UP = 0x3100,
	MUS_FX_BUZZ_DN = 0x3200,
	MUS_FX_BUZZ_SHAPE = 0x3f00,
	MUS_FX_BUZZ_SET = 0x3900,
	MUS_FX_BUZZ_SET_SEMI = 0x3a00,
	MUS_FX_FM_SET_MODULATION = 0x3300,
	MUS_FX_FM_SET_FEEDBACK = 0x3400,
	MUS_FX_FM_SET_HARMONIC = 0x3500,
	MUS_FX_FM_SET_WAVEFORM = 0x3600,
	MUS_FX_OSC_MIX = 0x0ee0, //wasn't there
	MUS_FX_PW_DN = 0x0700,
	MUS_FX_PW_UP = 0x0800,
	MUS_FX_PW_SET = 0x0900,
	MUS_FX_SET_WAVEFORM = 0x0b00,
	MUS_FX_SET_FXBUS = 0x1b00,
	MUS_FX_SET_SYNCSRC = 0x7a00,
	MUS_FX_SET_RINGSRC = 0x7b00,
	MUS_FX_SET_WAVETABLE_ITEM = 0x3b00,
	MUS_FX_SET_DOWNSAMPLE = 0x1e00,
	MUS_FX_WAVETABLE_OFFSET = 0x5000, //if sample is looped, it would set starting point
	MUS_FX_WAVETABLE_OFFSET_UP = 0x4400, //wasn't there
	MUS_FX_WAVETABLE_OFFSET_DOWN = 0x4500, //wasn't there
	MUS_FX_CUTOFF_FINE_SET = 0x6000,
	MUS_FX_PW_FINE_SET = 0x8000, //wasn't there
	MUS_FX_MORPH = 0x9000, //wasn't there //9xxy, morph to wave xx with speed of y
	MUS_FX_WAVETABLE_END_POINT = 0xE000, //wasn't there if sample is looped, it would set ending point. Exxx
	MUS_FX_WAVETABLE_END_POINT_UP = 0x4600, //wasn't there
	MUS_FX_WAVETABLE_END_POINT_DOWN = 0x4700, //wasn't there
	
	MUS_FX_FM_WAVETABLE_OFFSET = 0x4b00, //wasn't there
	MUS_FX_FM_WAVETABLE_OFFSET_UP = 0x4e00, //wasn't there
	MUS_FX_FM_WAVETABLE_OFFSET_DOWN = 0x4f00, //wasn't there
	
	MUS_FX_FM_WAVETABLE_END_POINT = 0x7000, //wasn't there
	MUS_FX_FM_WAVETABLE_END_POINT_UP = 0x7100, //wasn't there
	MUS_FX_FM_WAVETABLE_END_POINT_DOWN = 0x7200, //wasn't there
	
	MUS_FX_END = 0xffff,
	MUS_FX_JUMP = 0xff00,
	MUS_FX_LABEL = 0xfd00,
	MUS_FX_LOOP = 0xfe00,
	MUS_FX_TRIGGER_RELEASE = 0x7c00,
	MUS_FX_TRIGGER_FM_RELEASE = 0x4c00, //wasn't there
	MUS_FX_TRIGGER_CARRIER_RELEASE = 0x4d00, //wasn't there
	MUS_FX_RESTART_PROGRAM = 0x7d00,
	MUS_FX_NOP = 0xfffe,
};

enum
{
	MUS_CTRL_LEGATO = MUS_CTRL_BIT,
	MUS_CTRL_SLIDE = MUS_CTRL_BIT << 1,
	MUS_CTRL_VIB = MUS_CTRL_BIT << 2,
	MUS_CTRL_TREM = MUS_CTRL_BIT << 3 //wasn't there
};

enum //song flags
{
	MUS_ENABLE_REVERB = 1,
	MUS_ENABLE_CRUSH = 2,
	MUS_ENABLE_MULTIPLEX = 4,
	MUS_NO_REPEAT = 8,
	MUS_8_BIT_PATTERN_INDEX = 16, //wasn't there
	
	MUS_SEQ_NO_COMPRESSION = 32,
	MUS_SEQ_COMPRESS_DELTA = 64,
	MUS_SEQ_COMPRESS_GRAY = 128,
	
	MUS_PATTERNS_NO_COMPRESSION = 256,
	MUS_PATTERNS_COMPRESS_DELTA = 512,
	MUS_PATTERNS_COMPRESS_GRAY = 1024,
	
	MUS_16_BIT_RATE = 2048,
	
	MUS_HAS_DESCRIPTION = 4096,
	MUS_SHOW_DESCRIPTION = 8192, //show description automatically when song is loaded, BETTER FUCKING LEAVE IT AS 0!!!
};

enum
{
	MUS_SHAPE_SINE,
	MUS_SHAPE_RAMP_UP,
	MUS_SHAPE_RAMP_DN,
	MUS_SHAPE_RANDOM,
	MUS_SHAPE_SQUARE,
	MUS_NUM_SHAPES
};

#define MUS_INST_SIG "cyd!inst"
#define MUS_SONG_SIG "cyd!song"
#define MUS_FX_SIG "cyd!efex"
#define MUS_WAVEGEN_PATCH_SIG "cyd!wave_patch"

#ifdef USESDL_RWOPS
#include "SDL_rwops.h"
typedef SDL_RWops RWops;
#else

typedef struct RWops
{
	int (*read)(struct RWops *context, void *ptr, int size, int maxnum);
	int (*close)(struct RWops *context);
	union {
		FILE *fp;
		struct
		{
			Uint32 ptr, length;
			void *base;
		} mem;
	};
	int close_fp;
} RWops;

#endif

int mus_advance_tick(void* udata);
int mus_trigger_instrument(MusEngine* mus, int chan, MusInstrument *ins, Uint16 note, int panning);
void mus_set_channel_volume(MusEngine* mus, int chan, int volume);
void mus_release(MusEngine* mus, int chan);
void mus_init_engine(MusEngine *mus, CydEngine *cyd);
void mus_set_song(MusEngine *mus, MusSong *song, Uint16 position);
int mus_poll_status(MusEngine *mus, int *song_position, int *pattern_position, MusPattern **pattern, MusChannel *channel, int *cyd_env, int *mus_note, Uint64 *time_played);
int mus_load_instrument_file(Uint8 version, FILE *f, MusInstrument *inst, CydWavetableEntry *wavetable_entries);
int mus_load_instrument_file2(FILE *f, MusInstrument *inst, CydWavetableEntry *wavetable_entries);
int mus_load_instrument_RW(Uint8 version, RWops *ctx, MusInstrument *inst, CydWavetableEntry *wavetable_entries);
int mus_load_instrument_RW2(RWops *ctx, MusInstrument *inst, CydWavetableEntry *wavetable_entries);
int mus_load_instrument(const char *path, MusInstrument *inst, CydWavetableEntry *wavetable_entries);
void mus_get_default_instrument(MusInstrument *inst);
int mus_load_song(const char *path, MusSong *song, CydWavetableEntry *wavetable_entries);
int mus_load_song_file(FILE *f, MusSong *song, CydWavetableEntry *wavetable_entries);
int mus_load_song_RW(RWops *rw, MusSong *song, CydWavetableEntry *wavetable_entries);
int mus_load_fx_RW(RWops *ctx, CydFxSerialized *fx);
int mus_load_fx_file(FILE *f, CydFxSerialized *fx);

int mus_load_wavepatch(FILE *f, WgSettings *settings); //wasn't there

int mus_load_fx(const char *path, CydFxSerialized *fx);
void mus_free_song(MusSong *song);
void mus_set_fx(MusEngine *mus, MusSong *song);
Uint32 mus_ext_sync(MusEngine *mus);
Uint32 mus_get_playtime_at(MusSong *song, int position);

#endif
