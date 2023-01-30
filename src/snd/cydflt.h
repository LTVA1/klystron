#pragma once

#ifndef CYDFLT_H
#define CYDFLT_H
#endif
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

#include "cydtypes.h"


// Bi-Quad Module

typedef struct {
	double a0;
	double a1;
	double a2;
	double b0;
	double b1;
	double b2;
	double prev_input_1;
	double prev_input_2;
	double prev_output_1;
	double prev_output_2;
	
	double inputf;
	double outputf;
	
	double cs;
	double alpha;
	
	Uint8 resonance;
	Uint16 frequency;
	
	Sint32 output;
	Sint32 input;
	
	/* OLD FILTER BELOW: */
	
	Sint64 _f, _q, _p;
	Sint64 _b0, _b1, _b2, _b3, _b4; //filter coefficients
} CydFilter;

void cydflt_cycle(CydFilter *flt, Sint32 input);

void cydflt_set_coeff(CydFilter *flt, Uint16 frequency, Uint16 cutoff, Uint32 sample_rate);

Sint32 cydflt_output_lp(CydFilter *flt);
Sint32 cydflt_output_hp(CydFilter *flt);
Sint32 cydflt_output_bp(CydFilter *flt);

//0..4095
void cydflt_set_coeff_old(CydFilter *flt, Uint16 frequency, Uint16 cutoff);

void cydflt_cycle_old(CydFilter *flt, Sint32 input);
Sint32 cydflt_output_lp_old(CydFilter *flt);
Sint32 cydflt_output_hp_old(CydFilter *flt);
Sint32 cydflt_output_bp_old(CydFilter *flt);
