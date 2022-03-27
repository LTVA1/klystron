#pragma once

#include "cydadsr.h"
#include "cydentry.h"
#include "cydtypes.h"
#include "cydwave.h"

#include "music_defs.h"

typedef struct //wasn't there
{
    Sint64 input, output;
	
	bool is_modulating; //if true modulate input op's accumulator and spit out modulated acc's wave at output. If false, just spit out its own signal.
	
	Uint8 input_op;
	
	bool depends_on_main_freq; //if depends, display mult and detune settings. If no, display standard base note/finetune thing
	
	Uint8 harmonic; //freq mult
	Uint8 alg; //algorithm
    
    Uint32 flags;
	//Uint32 musflags;
    CydAdsr adsr;
	
	Uint32 frequency;
	Uint32 wave_frequency;
	Uint64 accumulator;
	const CydWavetableEntry *wave_entry;
	CydWaveState wave;
	Uint8 wavetable_entry;
	
	Uint8 vol_ksl_level;
	Uint8 env_ksl_level;
	Uint32 freq_for_ksl;
	double vol_ksl_mult;
	double env_ksl_mult;
	
	Uint32 fb1, fb2, env_output;
	Uint32 current_modulation;
	Uint8 attack_start;
	
    Uint8 sync_source, ring_mod; // 0xff == self, 0xfb-0xfe -- other ops
    Uint16 pw;
    Uint8 volume;
    
    Uint8 mixmode; 
    
    //Uint16 program[MUS_PROG_LEN];
	//Uint8 program_unite_bits[MUS_PROG_LEN / 8 + 1];
	
	Uint8 env_offset, program_offset; //<-----
	
    Uint8 feedback;
    Uint16 cutoff;
    Uint8 resonance; //was 0-3, now 0-15
    Uint8 flttype;
} CydFmOp;

typedef struct
{
	Uint32 flags;
	
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
	
	Uint8 fm_vol_ksl_level;
	Uint8 fm_env_ksl_level;
	Uint32 freq_for_fm_ksl;
	double fm_vol_ksl_mult;
	double fm_env_ksl_mult;
	
	CydFmOp ops[MUS_FM_NUM_OPS];
	
	Uint8 fm_freq_LUT;
	
	Sint16 fm_tremolo; //wasn't there
	Sint16 fm_prev_tremolo;
	Uint8 fm_tremolo_interpolation_counter;
	Sint16 fm_curr_tremolo;
	
	Sint16 fm_vib;
	
	//Uint32 counter;
	
} CydFm;

#include "cyd.h"

struct CydEngine_t;

void cydfm_init(CydFm *fm);
void cydfm_cycle(const struct CydEngine_t *cyd, CydFm *fm);
void cydfm_cycle_oversample(const struct CydEngine_t *cyd, CydFm *fm);
void cydfm_set_frequency(const struct CydEngine_t *cyd, CydFm *fm, Uint32 base_frequency);
Uint64 cydfm_modulate(const struct CydEngine_t *cyd, const CydFm *fm, Uint32 accumulator);
CydWaveAcc cydfm_modulate_wave(const struct CydEngine_t *cyd, const CydFm *fm, const CydWavetableEntry *wave, CydWaveAcc accumulator);
void cydfm_set_wave_entry(CydFm *fm, const CydWavetableEntry * entry);