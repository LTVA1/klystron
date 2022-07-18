#pragma once

#include "cydadsr.h"
#include "cydentry.h"
#include "cydtypes.h"
#include "cydwave.h"
#include "cydflt.h"
#include "cyddefs.h"

#include "music_defs.h"

#include "cydoscstate.h" //wasn't there

const static Uint32 coarse_detune_table[] = { 0, 256 * 6 + 10, 256 * 8 - 20, 256 * 10 + 14 }; //0, 6, 8 and 10 (roughly) semitones up respectively

typedef struct //wasn't there
{
	Sint32 input, output;

	Uint8 harmonic; //freq mult
    
    Uint32 flags;
	
    CydFmOpAdsr adsr;
	
	Uint32 frequency;
	
	Uint32 true_freq;
	
	const CydWavetableEntry* wave_entry;
	
	CydOscState osc;
	
	Uint8 wavetable_entry;
	
	Uint8 vol_ksl_level;
	Uint8 env_ksl_level;
	Uint32 freq_for_ksl;
	double vol_ksl_mult;
	double env_ksl_mult;
	
	Sint16 tremolo; //wasn't there
	Sint16 prev_tremolo;
	Uint16 tremolo_interpolation_counter;
	Sint16 curr_tremolo;
	
	CydFilter flts[CYD_NUMBER_OF_FILTER_MODULES];
	
	//Sint32 prev, prev2;
	Uint32 prev, prev2;
	
	Uint32 env_output;
	Uint8 attack_start;
	
    Uint8 sync_source, ring_mod; // 0xff == self, 0xfb-0xfe -- other ops
    Uint16 pw;
    Uint8 volume;
    
    Uint8 mixmode; 
	
	Uint8 program_offset;
	
    Uint8 feedback; //0-F
    Uint16 cutoff;
    Uint8 resonance; //was 0-3, now 0-15
    Uint8 flttype;
	Uint8 flt_slope;
	
	Uint8 base_note;
	Sint8 finetune;
	
	Uint8 ssg_eg_type; //0-7
	
	Sint8 detune; //-7..7, 2 * finetune
	Uint8 coarse_detune; //OPM DT2, 0..3
	
	Sint32 trigger_delay;
	
	Sint32 mod, noise_mod, wave_mod;
	//Uint32 mod, noise_mod, wave_mod;
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
	
	CydFmOp ops[CYD_FM_NUM_OPS];
	
	Uint8 fm_freq_LUT;
	
	Sint16 fm_tremolo; //wasn't there
	Sint16 fm_prev_tremolo;
	Uint8 fm_tremolo_interpolation_counter;
	Sint16 fm_curr_tremolo;
	
	Sint16 fm_vib;
	
	Uint8 alg; //4-op algorithm
	
	Uint8 fm_4op_vol;
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