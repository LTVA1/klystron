#ifndef CYD_H
#define CYD_H


/*
Copyright (c) 2009-2010 Tero Lindeman (kometbomb)
Copyright (c) 2021-2022 Georgy Saraykin (LTVA1 a.k.a. LTVA) and contributors

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

#include <signal.h>
#include "cydtypes.h"
#include "cydflt.h"
#include "cydfx.h"
#include "cydentry.h"
#include "cyddefs.h"
#include "cydadsr.h"
#include "cydfm.h"
#include "cydwave.h"

#define envspd(cyd,slope) (slope!=0?(((Uint64)0xff0000 / ((slope) * (slope) * 256 / (ENVELOPE_SCALE * ENVELOPE_SCALE))) * CYD_BASE_FREQ / cyd->sample_rate):((Uint64)0xff0000 * CYD_BASE_FREQ / cyd->sample_rate))

#include "cydoscstate.h"

typedef struct
{
	// ---- interface
	Uint32 flags;
	
	Uint32 musflags;
	
	Uint16 pw; // 0-2047
	Uint8 sync_source, ring_mod; // channel
	Uint8 flttype;
	CydAdsr adsr;
	
	Uint8 vol_ksl_level;
	Uint8 env_ksl_level;
	Uint32 freq_for_ksl;
	
	Uint32 true_freq;
	
	double vol_ksl_mult;
	double env_ksl_mult;
	double fm_env_ksl_mult;
	
	Uint8 ym_env_shape;
#ifdef STEREOOUTPUT
	Uint8 panning; // 0-255, 128 = center
	Sint32 gain_left, gain_right;
#endif
	// ---- internal
	Uint32 sync_bit;
	Uint32 lfsr_type;
	const CydWavetableEntry *wave_entry;
	CydOscState subosc[CYD_SUB_OSCS];
	CydFilter flts[CYD_NUMBER_OF_FILTER_MODULES];
	int fx_bus;
	
#ifndef CYD_DISABLE_FM
	CydFm fm;
#endif

	Uint8 mixmode; //wasn't there
	Uint8 flt_slope;
	
	Sint16 tremolo; //wasn't there
	Sint16 prev_tremolo;
	Uint8 tremolo_interpolation_counter;
	Sint16 curr_tremolo;
	
	Uint8 base_note;
	Sint8 finetune;
	
	Uint8 sine_acc_shift; //0-F
	
	Uint64 counter; //for general debug purposes
} CydChannel;

enum
{
	FLT_LP,
	FLT_HP,
	FLT_BP,
	FLT_LH, 
	FLT_HB, 
	FLT_LB, 
	FLT_ALL,
	FLT_TYPES
};

enum
{
	CYD_CHN_ENABLE_NOISE = 1,
	CYD_CHN_ENABLE_PULSE = 2,
	CYD_CHN_ENABLE_TRIANGLE = 4,
	CYD_CHN_ENABLE_SAW = 8,
	CYD_CHN_ENABLE_SYNC = 16,
	CYD_CHN_ENABLE_GATE = 32,
	CYD_CHN_ENABLE_KEY_SYNC = 64,
	CYD_CHN_ENABLE_METAL = 128,
	CYD_CHN_ENABLE_RING_MODULATION = 256,
	CYD_CHN_ENABLE_FILTER = 512,
	CYD_CHN_ENABLE_FX = 1024,
	CYD_CHN_ENABLE_YM_ENV = 2048,
	CYD_CHN_ENABLE_WAVE = 4096,
	CYD_CHN_WAVE_OVERRIDE_ENV = 8192,
	CYD_CHN_ENABLE_LFSR = 16384,
	CYD_CHN_ENABLE_FM = 32768,
	
	CYD_CHN_ENABLE_VOLUME_KEY_SCALING = 65536,
	CYD_CHN_ENABLE_ENVELOPE_KEY_SCALING = 131072,
	
	CYD_CHN_ENABLE_SINE = 262144,
	//CYD_CHN_ENABLE_WAVE_MORPH = 262144,
	//CYD_CHN_LOOP_WAVE_MORPH = 524288,
	//CYD_CHN_PING_PONG_WAVE_MORPH = 1048576,
	
	CYD_CHN_ENABLE_FILTER_SWEEP = 2097152,
	CYD_CHN_LOOP_FILTER_SWEEP = 4194304,
	CYD_CHN_PING_PONG_FILTER_SWEEP = 8388608,
	
	CYD_CHN_ENABLE_EXPONENTIAL_VOLUME = 16777216,
	CYD_CHN_ENABLE_EXPONENTIAL_ATTACK = 33554432,
	CYD_CHN_ENABLE_EXPONENTIAL_DECAY = 67108864,
	CYD_CHN_ENABLE_EXPONENTIAL_RELEASE = 134217728,
	
	CYD_CHN_ENABLE_AY8930_BUZZ_MODE = 268435456,
	
	CYD_CHN_ENABLE_FIXED_NOISE_PITCH = 536870912,
	CYD_CHN_ENABLE_1_BIT_NOISE = 1073741824,
};

enum
{
	CYD_FM_ENABLE_NOISE = CYD_CHN_ENABLE_NOISE, //1
	CYD_FM_ENABLE_PULSE = CYD_CHN_ENABLE_PULSE, //2
	CYD_FM_ENABLE_TRIANGLE = CYD_CHN_ENABLE_TRIANGLE, //4
	CYD_FM_ENABLE_SAW = CYD_CHN_ENABLE_SAW, //8
	CYD_FM_ENABLE_GATE = CYD_CHN_ENABLE_GATE, //32
	CYD_FM_ENABLE_WAVE = CYD_CHN_ENABLE_WAVE, //4096
	CYD_FM_ENABLE_VOLUME_KEY_SCALING = CYD_CHN_ENABLE_VOLUME_KEY_SCALING, //65536
	CYD_FM_ENABLE_ENVELOPE_KEY_SCALING = CYD_CHN_ENABLE_ENVELOPE_KEY_SCALING, //131072
	
	CYD_FM_ENABLE_3CH_EXP_MODE = 262144,
	
	CYD_FM_FOUROP_USE_MAIN_INST_PROG = 524288,
	CYD_FM_FOUROP_BYPASS_MAIN_INST_FILTER = 1048576,
	
	CYD_FM_SAVE_LFO_SETTINGS = 268435456,
	CYD_FM_ENABLE_4OP = 536870912,
	CYD_FM_ENABLE_ADDITIVE = 1073741824,
	
	CYD_FM_ENABLE_EXPONENTIAL_VOLUME = CYD_CHN_ENABLE_EXPONENTIAL_VOLUME, //16777216
	CYD_FM_ENABLE_EXPONENTIAL_ATTACK = CYD_CHN_ENABLE_EXPONENTIAL_ATTACK, //33554432
	CYD_FM_ENABLE_EXPONENTIAL_DECAY = CYD_CHN_ENABLE_EXPONENTIAL_DECAY, //67108864
	CYD_FM_ENABLE_EXPONENTIAL_RELEASE = CYD_CHN_ENABLE_EXPONENTIAL_RELEASE, //134217728
};

enum
{
	CYD_FM_OP_ENABLE_NOISE = CYD_CHN_ENABLE_NOISE, //1
	CYD_FM_OP_ENABLE_PULSE = CYD_CHN_ENABLE_PULSE, //2
	CYD_FM_OP_ENABLE_TRIANGLE = CYD_CHN_ENABLE_TRIANGLE, //4
	CYD_FM_OP_ENABLE_SAW = CYD_CHN_ENABLE_SAW, //8
	CYD_FM_OP_ENABLE_SYNC = CYD_CHN_ENABLE_SYNC, //16
	CYD_FM_OP_ENABLE_GATE = CYD_CHN_ENABLE_GATE, //32
	CYD_FM_OP_ENABLE_KEY_SYNC = CYD_CHN_ENABLE_KEY_SYNC, //64
	CYD_FM_OP_ENABLE_METAL = CYD_CHN_ENABLE_METAL, //128
	CYD_FM_OP_ENABLE_RING_MODULATION = CYD_CHN_ENABLE_RING_MODULATION, //256
	CYD_FM_OP_ENABLE_FILTER = CYD_CHN_ENABLE_FILTER, //512
	
	CYD_FM_OP_WAVE_OVERRIDE_ENV = 1024,
	
	//CYD_FM_OP_ENABLE_VOLUME_KEY_SCALING = 2048, //DO NOT OCCUPY!!
	
	CYD_FM_OP_ENABLE_WAVE = CYD_CHN_ENABLE_WAVE, //4096
	
	CYD_FM_OP_ENABLE_ENVELOPE_KEY_SCALING = 8192,
	
	CYD_FM_OP_ENABLE_FIXED_NOISE_PITCH = 16384,
	
	CYD_FM_OP_ENABLE_SSG_EG = 65536,
	CYD_FM_OP_ENABLE_VOLUME_KEY_SCALING = 131072,
	
	CYD_FM_OP_ENABLE_SINE = CYD_CHN_ENABLE_SINE, //262144
	CYD_FM_OP_ENABLE_CSM_TIMER = 524288,
	
	CYD_FM_OP_ENABLE_EXPONENTIAL_VOLUME = CYD_CHN_ENABLE_EXPONENTIAL_VOLUME, //16777216
	CYD_FM_OP_ENABLE_EXPONENTIAL_ATTACK = CYD_CHN_ENABLE_EXPONENTIAL_ATTACK, //33554432
	CYD_FM_OP_ENABLE_EXPONENTIAL_DECAY = CYD_CHN_ENABLE_EXPONENTIAL_DECAY, //67108864
	CYD_FM_OP_ENABLE_EXPONENTIAL_RELEASE = CYD_CHN_ENABLE_EXPONENTIAL_RELEASE, //134217728
	
	CYD_FM_OP_ENABLE_1_BIT_NOISE = CYD_CHN_ENABLE_1_BIT_NOISE, //1073741824
	
	//musflags
	
	MUS_FM_OP_PROG_SPEED_MODE = 1, //0 - relative, 1 - absolute
	MUS_FM_OP_INVERT_VIBRATO_BIT = 2,
	MUS_FM_OP_LOCK_NOTE = 4,
	MUS_FM_OP_SET_PW = 8,
	MUS_FM_OP_SET_CUTOFF = 16,
	MUS_FM_OP_RELATIVE_VOLUME = 32,
	MUS_FM_OP_WAVE_LOCK_NOTE = 64,
	MUS_FM_OP_NO_PROG_RESTART = 128,
	MUS_FM_OP_SAVE_LFO_SETTINGS = 256,
	MUS_FM_OP_INVERT_TREMOLO_BIT = 512,
	MUS_FM_OP_DRUM = 1024,
	MUS_FM_OP_QUARTER_FREQ = 2048,
	MUS_FM_OP_SEVERAL_MACROS = 4096, //if operator has more than 1 macro
	MUS_FM_OP_LINK_CSM_TIMER_NOTE = 8192, //if CSM timer note changes with FM operator note
};

enum {
	ATTACK,
	DECAY,
	SUSTAIN,
	RELEASE,
	DONE
};

enum
{
	CYD_LOCK_REQUEST = 1,
	CYD_LOCK_LOCKED = 2,
	CYD_LOCK_CALLBACK = 4
};

#define WAVEFORMS (CYD_CHN_ENABLE_NOISE|CYD_CHN_ENABLE_PULSE|CYD_CHN_ENABLE_TRIANGLE|CYD_CHN_ENABLE_SAW|CYD_CHN_ENABLE_WAVE|CYD_CHN_ENABLE_LFSR|CYD_CHN_ENABLE_SINE)
#define FM_OP_WAVEFORMS (CYD_FM_OP_ENABLE_NOISE|CYD_FM_OP_ENABLE_PULSE|CYD_FM_OP_ENABLE_TRIANGLE|CYD_FM_OP_ENABLE_SAW|CYD_FM_OP_ENABLE_WAVE|CYD_FM_OP_ENABLE_SINE)

#define LUT_SIZE 1024
#define YM_LUT_SIZE 16
#define EXP_LUT_SIZE 8192

#define CYD_NUM_WO_BUFFER_SIZE 2000
#define CYD_NUM_WO_BUFFERS 4

#define CYD_NUM_LFSR 16

extern Uint16 PulseTri_8580[8192];

typedef struct CydEngine_t
{
	CydChannel *channel;
	int n_channels;
	Uint32 sample_rate;
	// ----- internal
	volatile Uint32 flags;
	int (*callback)(void*);
	void *callback_parameter;
	volatile Uint32 callback_period, callback_counter;
	Uint16 *lookup_table, *lookup_table_ym, lookup_table_ay8930[32];
	
	Uint16 lookup_table_exponential[EXP_LUT_SIZE];
	
	CydFx fx[CYD_MAX_FX_CHANNELS];
#ifdef USESDLMUTEXES
	CydMutex mutex;	
#else
	volatile sig_atomic_t lock_request;
	volatile sig_atomic_t lock_locked;
#endif
	size_t samples_output; // bytes in last cyd_output_buffer
	CydWavetableEntry *wavetable_entries;
#ifdef USENATIVEAPIS
# ifdef WIN32
	BOOL thread_running;
	DWORD thread_handle, cb_handle;
	CRITICAL_SECTION thread_lock;
	int buffers_available;
	int waveout_hdr_idx;
	HWAVEOUT hWaveOut;
	WAVEHDR waveout_hdr[CYD_NUM_WO_BUFFERS]; 
# endif
#endif
	Uint64 samples_played;
	int oversample;
	
	Uint16 TriSaw_8580[4096]; //wasn't there
	Uint16 PulseSaw_8580[4096];
	Uint16 PulseTriSaw_8580[4096];
	
	Uint16 sine_table[65536];
} CydEngine;

enum
{
	CYD_PAUSED = 1,
	CYD_CLIPPING = 2,
	CYD_SINGLE_THREAD = 8,
};

// YM2149 envelope shape flags, CONT is assumed to be always set

enum { CYD_YM_ENV_ATT = 1, CYD_YM_ENV_ALT = 2};

void cyd_init(CydEngine *cyd, Uint32 sample_rate, int initial_channels);
void cyd_set_oversampling(CydEngine *cyd, int oversampling);
void cyd_reserve_channels(CydEngine *cyd, int channels);
void cyd_deinit(CydEngine *cyd);
void cyd_reset(CydEngine *cyd);
void cyd_set_frequency(CydEngine *cyd, CydChannel *chn, int subosc, Uint32 frequency);
void cyd_set_wavetable_frequency(CydEngine *cyd, CydChannel *chn, int subosc, Uint32 frequency);
void cyd_reset_wavetable(CydEngine *cyd);
void cyd_set_wavetable_offset(CydChannel *chn, Uint16 offset /* 0..0x1000 = 0-100% */);
void cyd_set_wavetable_end_offset(CydChannel *chn, Uint16 offset /* 0..0x1000 = 0-100% */);

void cyd_set_fm_op_wavetable_offset(CydChannel *chn, Uint16 offset /* 0..0x1000 = 0-100% */, Uint8 i);
void cyd_set_fm_op_wavetable_end_offset(CydChannel *chn, Uint16 offset /* 0..0x1000 = 0-100% */, Uint8 i);

void cyd_set_env_frequency(CydEngine *cyd, CydChannel *chn, Uint32 frequency);
void cyd_set_env_shape(CydChannel *chn, Uint8 shape);
void cyd_enable_gate(CydEngine *cyd, CydChannel *chn, Uint8 enable);
void cyd_set_waveform(CydChannel *chn, Uint32 wave);
void cyd_set_wave_entry(CydChannel *chn, const CydWavetableEntry * entry);
void cyd_set_filter_coeffs(CydEngine * cyd, CydChannel *chn, Uint16 cutoff, Uint8 resonance);
void cyd_pause(CydEngine *cyd, Uint8 enable);
void cyd_set_callback(CydEngine *cyd, int (*callback)(void*), void*param, Uint16 period);
void cyd_set_callback_rate(CydEngine *cyd, Uint16 period);
#ifdef NOSDL_MIXER
int cyd_register(CydEngine * cyd, int buffer_length);
#else
int cyd_register(CydEngine * cyd);
#endif
int cyd_unregister(CydEngine * cyd);
void cyd_lock(CydEngine *cyd, Uint8 enable);
#ifdef ENABLEAUDIODUMP
void cyd_enable_audio_dump(CydEngine *cyd);
void cyd_disable_audio_dump(CydEngine *cyd);
#endif
#ifdef STEREOOUTPUT
void cyd_set_panning(CydEngine *cyd, CydChannel *chn, Uint8 panning);
#endif

#ifdef NOSDL_MIXER
void cyd_output_buffer_stereo(void *udata, Uint8 *_stream, int len);
#else
void cyd_output_buffer_stereo(int chan, void *_stream, int len, void *udata);
#endif

Sint32 cyd_env_output(const CydEngine *cyd, Uint32 channel_flags, const CydAdsr *adsr, Sint32 input);
Sint32 cyd_fm_op_env_output(const CydEngine *cyd, Uint32 channel_flags, const CydFmOpAdsr *adsr, Sint32 input);

Uint32 cyd_cycle_adsr(const CydEngine *eng, Uint32 flags, Uint32 ym_env_shape, CydAdsr *adsr, double env_ksl_mult);
Uint32 cyd_cycle_fm_op_adsr(const CydEngine *eng, Uint32 flags, Uint32 ym_env_shape, CydFmOpAdsr *adsr, double env_ksl_mult, Uint8 ssg_eg);

#endif

long map_Arduino(long x, long in_min, long in_max, long out_min, long out_max);
void createCombinedWF(Uint16 wfarray[], float bitmul, float bitstrength, float treshold); //from jsSID emulator