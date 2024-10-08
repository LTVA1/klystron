#ifndef CYDENTRY_H
#define CYDENTRY_H

/*
Copyright (c) 2009-2010 Tero Lindeman (kometbomb)
Copyright (c) 2021-2023 Georgy Saraykin (LTVA1 a.k.a. LTVA) and contributors

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

#include "cydtypes.h"

enum
{
	CYD_WAVE_LOOP = 1,
	CYD_WAVE_PINGPONG = 2, // ping-pong loop as in FT2
	CYD_WAVE_NO_INTERPOLATION = 4,
	CYD_WAVE_COMPRESSED_DELTA = 8,
	CYD_WAVE_COMPRESSED_GRAY = 16,
	
	CYD_WAVE_INTERPOLATION_BIT_1 = 32,
	CYD_WAVE_INTERPOLATION_BIT_2 = 64,
	CYD_WAVE_INTERPOLATION_BIT_3 = 128,

	CYD_WAVE_ACC_NO_RESET = 256,
};

typedef enum
{
	CYD_WAVE_TYPE_SINT16, 	// internal format
	CYD_WAVE_TYPE_SINT8, 	// amiga modules?
	CYD_WAVE_TYPE_UINT8 	// atari YM files?
} CydWaveType;

typedef struct
{
	Uint32 flags;
	Uint32 sample_rate;
	Uint32 samples, loop_begin, loop_end; //begin and end
	
	Uint16 base_note;
	Sint16 *data; 
} CydWavetableEntry;

void cyd_wave_entry_init(CydWavetableEntry *entry, const void *data, Uint32 n_samples, CydWaveType sample_type, int channels, int denom, int nom);
void cyd_wave_entry_deinit(CydWavetableEntry *entry);

#endif
