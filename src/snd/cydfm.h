#pragma once

#include "cydadsr.h"
#include "cydentry.h"
#include "cydtypes.h"
#include "cydwave.h"

#include "music_defs.h"

typedef struct //wasn't there
{
    Uint16 input, output;
    
    Uint32 flags;
    Uint32 cydflags;
    CydAdsr adsr;
    Uint8 sync_source, ring_mod; // 0xff == self, 0xfc-0xfe -- other ops
    Uint16 pw;
    Uint8 volume;
    
    Uint8 mixmode; 
    
    Uint16 program[MUS_PROG_LEN];
	
	Uint8 env_offset, program_offset; //<-----
	
    Uint8 prog_period; 
    Uint8 slide_speed;
    Uint8 tremolo_speed, tremolo_delay, tremolo_shape, tremolo_depth;
    Uint8 vibrato_speed, vibrato_delay, vibrato_shape, vibrato_depth;
    Uint8 pwm_speed, pwm_delay, pwm_shape, pwm_depth;
    Uint8 base_note;
    Sint8 finetune;
    Uint8 mult, feedback;
    Uint16 cutoff;
    Uint8 resonance; //was 0-3, now 0-15
    Uint8 flttype;
    Uint8 fx_bus;
    Uint8 wavetable_entry;
    Uint8 lfsr_type;
} CydFmOp;

typedef struct
{
	Uint32 flags;
	
	Uint8 alg;
	
	Uint8 feedback; // 0-7 
	Uint8 harmonic; // 0-15
	CydAdsr adsr;
	Uint32 period;
	Uint32 wave_period;
	Uint64 accumulator;
	const CydWavetableEntry *wave_entry;
	CydWaveState wave;
	Uint32 fb1, fb2, env_output;
	Uint32 current_modulation;
	Uint8 attack_start;
	
	Uint8 fm_base_note; //weren't there
	Sint8 fm_finetune;
	Uint8 fm_carrier_base_note;
	Sint8 fm_carrier_finetune;
	
	CydFmOp ops[MUS_FM_NUM_OPS];
	
	Uint8 fm_freq_LUT;
	
	Sint16 fm_tremolo; //wasn't there
	Sint16 fm_prev_tremolo;
	Uint8 fm_tremolo_interpolation_counter;
	Sint16 fm_curr_tremolo;
	
	Sint16 fm_vib;
	
} CydFm;

#include "cyd.h"

struct CydEngine_t;

void cydfm_init(CydFm *fm);
void cydfm_cycle(const struct CydEngine_t *cyd, CydFm *fm);
void cydfm_cycle_oversample(const struct CydEngine_t *cyd, CydFm *fm);
void cydfm_set_frequency(const struct CydEngine_t *cyd, CydFm *fm, Uint32 base_frequency);
Uint32 cydfm_modulate(const struct CydEngine_t *cyd, const CydFm *fm, Uint32 accumulator);
CydWaveAcc cydfm_modulate_wave(const struct CydEngine_t *cyd, const CydFm *fm, const CydWavetableEntry *wave, CydWaveAcc accumulator);
void cydfm_set_wave_entry(CydFm *fm, const CydWavetableEntry * entry);
