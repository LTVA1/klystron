#pragma once

#include "cydwave.h"

typedef struct
{
	Uint32 frequency;
	Uint64 accumulator;
	
	Uint16 buzz_detune_freq;
	
	Uint32 noise_frequency; //wasn't there
	Uint64 noise_accumulator;
	
	Uint32 random; // random lfsr
	Uint32 lfsr, lfsr_period, lfsr_ctr, lfsr_acc; // lfsr state
	Uint32 reg4, reg5, reg9; // "pokey" lfsr registers
	CydWaveState wave;
} CydOscState;