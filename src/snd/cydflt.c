/*
Copyright (c) 2009-2010 Tero Lindeman (kometbomb)

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

#include "macros.h"
#include <assert.h>

#include "cydflt.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI (3.141592654)
#endif

void cydflt_cycle(CydFilter *flt, Sint32 input)
{
	flt->inputf = input / 2147483647.0;
}

void cydflt_set_coeff(CydFilter *flt, Uint16 frequency, Uint16 resonance) 
{
	frequency = (frequency * 20000 / 2048);
	
	flt->resonance = resonance;
	flt->frequency = frequency;
	
	double sample_rate = 44100; //was 44100
	//double dbGain = 1.0; //this was 1.0
	double Q = 0.5 + resonance * 0.25; //this variables may need some tweaking (this was 5.0) 0.7071
	
	//double A = pow(10, dbGain / 40); //convert to db
    double omega = 2 * M_PI * frequency / sample_rate; //I really don't know what the fuck is going on
    double sn = sin(omega);
    flt->cs = cos(omega);
    flt->alpha = sn / (2 * Q);
}

Sint32 cydflt_output_lp(CydFilter *flt)
{
	flt->b0 = (1.0 - flt->cs) / 2.0;
    flt->b1 = 1.0 - flt->cs;
    flt->b2 = (1.0 - flt->cs) / 2.0;
    flt->a0 = 1.0 + flt->alpha;
    flt->a1 = -2.0 * flt->cs; //was -2.0 * flt->cs;
    flt->a2 = 1.0 - flt->alpha;
	
	flt->a1 /= flt->a0;
	flt->a2 /= flt->a0;
	flt->b0 /= flt->a0;
	flt->b1 /= flt->a0;
	flt->b2 /= flt->a0;
	
	flt->outputf = (flt->b0 * flt->inputf) +
					(flt->b1 * flt->prev_input_1) +
					(flt->b2 * flt->prev_input_2) -
					(flt->a1 * flt->prev_output_1) -
					(flt->a2 * flt->prev_output_2);
	flt->prev_input_2 = flt->prev_input_1;
	flt->prev_input_1 = flt->inputf;
	flt->prev_output_2 = flt->prev_output_1;
	flt->prev_output_1 = flt->outputf;
	//update last samples...
	
	return (Sint32)(flt->outputf * 2147483647);
}


Sint32 cydflt_output_hp(CydFilter *flt)
{
	flt->b0 = (1.0 + flt->cs) / 2.0;
    flt->b1 = -(1.0 + flt->cs);
    flt->b2 = (1.0 + flt->cs) / 2.0;
    flt->a0 = 1.0 + flt->alpha;
    flt->a1 = -2.0 * flt->cs;
    flt->a2 = 1.0 - flt->alpha;
	
	flt->a1 /= flt->a0;
	flt->a2 /= flt->a0;
	flt->b0 /= flt->a0;
	flt->b1 /= flt->a0;
	flt->b2 /= flt->a0;
	
	flt->outputf = (flt->b0 * flt->inputf) +
					(flt->b1 * flt->prev_input_1) +
					(flt->b2 * flt->prev_input_2) -
					(flt->a1 * flt->prev_output_1) -
					(flt->a2 * flt->prev_output_2);
	flt->prev_input_2 = flt->prev_input_1;
	flt->prev_input_1 = flt->inputf;
	flt->prev_output_2 = flt->prev_output_1;
	flt->prev_output_1 = flt->outputf;
	//update last samples...
	
	return (Sint32)(flt->outputf * 2147483647);
}


Sint32 cydflt_output_bp(CydFilter *flt)
{
	flt->b0 = flt->alpha;
    flt->b1 = 0;
    flt->b2 = -flt->alpha;
    flt->a0 = 1.0 + flt->alpha;
    flt->a1 = -2.0 * flt->cs;
    flt->a2 = 1.0 - flt->alpha;
	
	flt->a1 /= flt->a0;
	flt->a2 /= flt->a0;
	flt->b0 /= flt->a0;
	flt->b1 /= flt->a0;
	flt->b2 /= flt->a0;
	
	flt->outputf = (flt->b0 * flt->inputf) +
					(flt->b1 * flt->prev_input_1) +
					(flt->b2 * flt->prev_input_2) -
					(flt->a1 * flt->prev_output_1) -
					(flt->a2 * flt->prev_output_2);
	flt->prev_input_2 = flt->prev_input_1;
	flt->prev_input_1 = flt->inputf;
	flt->prev_output_2 = flt->prev_output_1;
	flt->prev_output_1 = flt->outputf;
	//update last samples...
	
	return (Sint32)(flt->outputf * 2147483647);
}

/*void cydflt_set_coeff(CydFilter *flt, Uint16 frequency, Uint16 resonance)
{
	flt->q = 2048 - frequency;
	flt->p = frequency + ((Sint32)(0.8f * 2048.0f) * frequency / 2048 * flt->q) / 2048;
	flt->f = flt->p + flt->p - 2048;
	flt->q = resonance;
}

void cydflt_cycle(CydFilter *flt, Sint32 input)
{
	input -= flt->q * flt->b4 / 2048; //feedback
	Sint32 t1 = flt->b1;  
	flt->b1 = (input + flt->b0) * flt->p / 2048 - flt->b1 * flt->f / 2048;
	Sint32 t2 = flt->b2;  
	flt->b2 = (flt->b1 + t1) * flt->p / 2048 - flt->b2 * flt->f / 2048;
	t1 = flt->b3;
	flt->b3 = (flt->b2 + t2) * flt->p / 2048 - flt->b3 * flt->f / 2048;
	flt->b4 = (flt->b3 + t1) * flt->p / 2048 - flt->b4 * flt->f / 2048;
	
	flt->b4 = my_min(32767, my_max(-32768, flt->b4));    //clipping
	
	flt->b0 = input;
}

Sint32 cydflt_output_lp(CydFilter *flt)
{
	return flt->b4;
}


Sint32 cydflt_output_hp(CydFilter *flt)
{
	return flt->b0 - flt->b4;
}


Sint32 cydflt_output_bp(CydFilter *flt)
{
	return 3 * (flt->b3 - flt->b4);
}*/

