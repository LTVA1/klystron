#ifndef CYDWAVE_H
#define CYDWAVE_H

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
#include "cydentry.h"
#include <stdbool.h>

typedef struct
{
	int direction; // 0 = forward, 1 = backwards
	bool playing;
	
	bool use_start_track_status_offset, use_end_track_status_offset;
	Uint16 start_offset, end_offset; //counting from set in wavegen sample start and end respectively, 0000-FFFF range
	Uint64 start_point_track_status, end_point_track_status; //for movable start and end positions, final values scaled from values above. These are compared with accumulator.
	
	CydWaveAcc acc; // probably overkill
	Uint32 frequency;
} CydWaveState;

Sint32 cyd_wave_get_sample(const CydWaveState *state, const CydWavetableEntry *wave_entry, CydWaveAcc acc);
void cyd_wave_cycle(CydWaveState *wave, const CydWavetableEntry *wave_entry);

#endif
