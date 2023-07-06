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

#include "cydwave.h"
#include "cyddefs.h"
#include "macros.h"

#ifndef LOWRESWAVETABLE
typedef Sint64 WaveAccSigned;
#else
typedef Sint32 WaveAccSigned;
#endif

#ifndef CYD_DISABLE_WAVETABLE
static Sint32 cyd_wave_get_sample_no_interpolation(const CydWavetableEntry *entry, CydWaveAcc wave_acc, int direction)
{
	if (entry->data)
	{	
		return entry->data[wave_acc / WAVETABLE_RESOLUTION];
	}
	
	else
		return 0;
}

static Sint32 cyd_wave_get_sample_linear(const CydWavetableEntry *entry, CydWaveAcc wave_acc, int direction)
{
	if (entry->data)
	{
		if (direction == 0) 
		{
			int a = wave_acc / WAVETABLE_RESOLUTION;
			int b = a + 1;
			
			if (a >= entry->samples || a < 0)
				return 0;
			
			if ((entry->flags & CYD_WAVE_LOOP) && b >= entry->loop_end)
			{
				if (!(entry->flags & CYD_WAVE_PINGPONG))
					b = b - entry->loop_end + entry->loop_begin;
				else
					b = entry->loop_end - (b - entry->loop_end);
			}
			
			if (b >= entry->samples)
				return entry->data[a];
			else
				return entry->data[a] + (entry->data[b] - entry->data[a]) * ((CydWaveAccSigned)wave_acc & (WAVETABLE_RESOLUTION - 1)) / WAVETABLE_RESOLUTION;
		}
		
		else
		{
			int a = wave_acc / WAVETABLE_RESOLUTION;
			int b = a - 1;
			
			if (a >= entry->samples || a < 0)
				return 0;
			
			if ((entry->flags & CYD_WAVE_LOOP) && b < (Sint32)entry->loop_begin)
			{
				if (!(entry->flags & CYD_WAVE_PINGPONG))
					b = b - entry->loop_begin + entry->loop_end;
				else
					b = entry->loop_begin - (b - entry->loop_begin);
			}
			
			if (b < 0)
				return entry->data[a];
			else
				return entry->data[a] + (entry->data[b] - entry->data[a]) * (WAVETABLE_RESOLUTION - ((CydWaveAccSigned)wave_acc & (WAVETABLE_RESOLUTION - 1))) / WAVETABLE_RESOLUTION;
		}
	}
	else
		return 0;
}


// Cosine interpolation by System64
static Sint32 cyd_wave_get_sample_cosine(const CydWavetableEntry *entry, CydWaveAcc wave_acc, int direction)
{
	if (entry->data)
	{
		if (direction == 0) 
		{
			int a = wave_acc / WAVETABLE_RESOLUTION;
			int b = a + 1;
			
			if (a >= entry->samples || a < 0)
				return 0;
			
			if ((entry->flags & CYD_WAVE_LOOP) && b >= entry->loop_end)
			{
				if (!(entry->flags & CYD_WAVE_PINGPONG))
					b = b - entry->loop_end + entry->loop_begin;
				
				else
					b = entry->loop_end - (b - entry->loop_end);
			}
			
			if (b >= entry->samples)
				return entry->data[a];
			
			else
				{
					//double x = (double)((CydWaveAccSigned)wave_acc % WAVETABLE_RESOLUTION) / (double) WAVETABLE_RESOLUTION;
					//return (Sint32) (((1 - cos(M_PI * x)) / 2) * (entry->data[b] - entry->data[a]) + entry->data[a]);
					double x = (double)((CydWaveAccSigned)wave_acc & (WAVETABLE_RESOLUTION - 1)) / (double) WAVETABLE_RESOLUTION;
					//mu2 = (1-cos(mu*PI))/2;
					double x2 = (1 - cos(x * M_PI)) / 2.00000000;
					return (Sint32) (entry->data[a] * (1 - x2) + entry->data[b] * x2);
				}
		}
		
		else
		{
			int a = wave_acc / WAVETABLE_RESOLUTION;
			int b = a - 1;
			
			if (a >= entry->samples || a < 0)
				return 0;
			
			if ((entry->flags & CYD_WAVE_LOOP) && b < (Sint32)entry->loop_begin)
			{
				if (!(entry->flags & CYD_WAVE_PINGPONG))
					b = b - entry->loop_begin + entry->loop_end;
				else
					b = entry->loop_begin - (b - entry->loop_begin);
			}
			
			if (b < 0)
				return entry->data[a];
			
			else
			{
				double x = (double)(WAVETABLE_RESOLUTION - ((CydWaveAccSigned)wave_acc & (WAVETABLE_RESOLUTION - 1))) / (double) WAVETABLE_RESOLUTION;
				double x2 = (1 - cos(x * M_PI)) / 2.00000000;
				return (Sint32) (entry->data[a] * (1 - x2) + entry->data[b] * x2);
				// return (Sint32) (((1 - cos(M_PI * x)) / 2) * (entry->data[b] - entry->data[a]) + entry->data[a]);
			}
		}
	}
	
	else
		return 0;
}

// Cubic spline interpolation by System64
static Sint32 cyd_wave_get_sample_cubic(const CydWavetableEntry *entry, CydWaveAcc wave_acc, int direction)
{
	if (entry->data)
	{    
		if (direction == 0) 
		{
			double t = (double)wave_acc / (double)WAVETABLE_RESOLUTION;
			int b = (int)t;
			int a = b - 1;
			int c = b + 1;
			int d = b + 2;
			
			// if (b >= entry->samples || b < 0)
			//     return 0;
			
			if ((entry->flags & CYD_WAVE_LOOP))
			{
				if (!(entry->flags & CYD_WAVE_PINGPONG))
				{
					if (b >= (Sint32)entry->loop_end)
					{
						t = t - (double)(entry->loop_end + entry->loop_begin);
						a = a - entry->loop_end + entry->loop_begin;
						b = b - entry->loop_end + entry->loop_begin;
						c = c - entry->loop_end + entry->loop_begin;
						d = d - entry->loop_end + entry->loop_begin;
					}
				}
					
				else
				{
					if (b >= (Sint32)entry->loop_end)
					{
						t = t - (double)(entry->loop_end + entry->loop_begin);
						a = entry->loop_end - (a - entry->loop_end);
						b = entry->loop_end - (b - entry->loop_end);
						c = entry->loop_end - (c - entry->loop_end);
						d = entry->loop_end - (d - entry->loop_end);
					}
				}
			}

			double mu = t - (double)b;
			double mu2 = mu * mu;

			double a0 = -0.5 * (double)entry->data[a] + 1.5 * (double)entry->data[b] - 1.5 * (double)entry->data[c] + 0.5 * (double)entry->data[d];
			double a1 = (double)entry->data[a] - 2.5 * (double)entry->data[b] + 2 * (double)entry->data[c] - 0.5 * (double)entry->data[d];
			double a2 = -0.5 * (double)entry->data[a] + 0.5 * (double)entry->data[c];
			double a3 = (double)entry->data[b];
			double interpolated = (a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3);
			return (Sint32)interpolated            ;
		}
		
		else
		{
			double t = wave_acc / WAVETABLE_RESOLUTION;
			int b = (int)t;
			int a = b - 1;
			int c = b + 1;
			int d = b + 2;
			
			// if (b >= entry->samples || b < 0)
			//     return 0;
			
			if ((entry->flags & CYD_WAVE_LOOP) && b < (Sint32)entry->loop_begin)
			{
				if (!(entry->flags & CYD_WAVE_PINGPONG))
				{
					t = t - (double)(entry->loop_begin + entry->loop_end);
					a = a - entry->loop_begin + entry->loop_end;
					b = b - entry->loop_begin + entry->loop_end;
					c = c - entry->loop_begin + entry->loop_end;
					d = d - entry->loop_begin + entry->loop_end;
				}
					
				else
				{
					t = t - (double)(entry->loop_begin + entry->loop_end);
					a = entry->loop_end - (a - entry->loop_end);
					b = entry->loop_end - (b - entry->loop_end);
					c = entry->loop_end - (c - entry->loop_end);
					d = entry->loop_end - (d - entry->loop_end);
				}
			}

			double mu = t - (double)b;
			double mu2 = mu * mu;

			double a0 = -0.5 * (double)entry->data[a] + 1.5 * (double)entry->data[b] - 1.5 * (double)entry->data[c] + 0.5 * (double)entry->data[d];
			double a1 = (double)entry->data[a] - 2.5 * (double)entry->data[b] + 2 * (double)entry->data[c] - 0.5 * (double)entry->data[d];
			double a2 = -0.5 * (double)entry->data[a] + 0.5 * (double)entry->data[c];
			double a3 = (double)entry->data[b];
			double interpolated = (a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3);
			return (Sint32)interpolated            ;
		}
	}
	
	else
		return 0;
}

#endif // CYD_DISABLE_WAVETABLE


enum 
{
	CYD_WAVE_INTERPOLATION_LINEAR = 0,
	CYD_WAVE_INTERPOLATION_COSINE = 1,
	CYD_WAVE_INTERPOLATION_CUBIC = 2,
};


Sint32 cyd_wave_get_sample(const CydWaveState *state, const CydWavetableEntry *wave_entry, CydWaveAcc acc)
{
#ifndef CYD_DISABLE_WAVETABLE
	
	if (wave_entry->flags & CYD_WAVE_NO_INTERPOLATION)
	{
		return cyd_wave_get_sample_no_interpolation(wave_entry, acc, state->direction);
	}
	
	else
	{
		int int_type = (wave_entry->flags & (CYD_WAVE_INTERPOLATION_BIT_1|CYD_WAVE_INTERPOLATION_BIT_2|CYD_WAVE_INTERPOLATION_BIT_3)) >> 5;
		
		switch(int_type)
		{
			case CYD_WAVE_INTERPOLATION_LINEAR: 
				return cyd_wave_get_sample_linear(wave_entry, acc, state->direction); break;
				
			case CYD_WAVE_INTERPOLATION_COSINE: 
				return cyd_wave_get_sample_cosine(wave_entry, acc, state->direction); break;

			case CYD_WAVE_INTERPOLATION_CUBIC: 
				return cyd_wave_get_sample_cubic(wave_entry, acc, state->direction); break;
		}
	}
	
#else
	return 0;
#endif // CYD_DISABLE_WAVETABLE
}


void cyd_wave_cycle(CydWaveState *wave, const CydWavetableEntry *wave_entry)
{
#ifndef CYD_DISABLE_WAVETABLE

	if (wave_entry && wave->playing)
	{
		if (wave->direction == 0)
		{
			//wave->acc += wave->frequency;
			wave->acc += (Sint32)((Sint64)wave->frequency * ((Sint64)0x10000 - (Sint64)wave->start_offset - ((Sint64)0x10000 - (Sint64)wave->end_offset)) / (Sint64)0x10000);
			
			if ((wave_entry->flags & CYD_WAVE_LOOP) && wave_entry->loop_end != wave_entry->loop_begin)
			{
				if(!(wave->use_start_track_status_offset) && !(wave->use_end_track_status_offset))
				{
					if (wave->acc / WAVETABLE_RESOLUTION >= (Uint64)wave_entry->loop_end)
					{
						if (wave_entry->flags & CYD_WAVE_PINGPONG) 
						{
							wave->acc = (Uint64)wave_entry->loop_end * WAVETABLE_RESOLUTION - (wave->acc - (Uint64)wave_entry->loop_end * WAVETABLE_RESOLUTION);
							wave->direction = 1;
						}
						
						else
						{
							wave->acc = wave->acc - (Uint64)wave_entry->loop_end * WAVETABLE_RESOLUTION + (Uint64)wave_entry->loop_begin * WAVETABLE_RESOLUTION;
						}
					}
				}
				
				else
				{
					if ((wave->use_end_track_status_offset ? (wave->acc >= wave->end_point_track_status) : (wave->acc / WAVETABLE_RESOLUTION >= (Uint64)wave_entry->loop_end)))
					{
						if (wave_entry->flags & CYD_WAVE_PINGPONG) 
						{
							wave->acc = (wave->use_end_track_status_offset ? wave->end_point_track_status : (wave_entry->loop_end * WAVETABLE_RESOLUTION)) - (wave->acc - (Uint64)(wave->use_end_track_status_offset ? wave->end_point_track_status : (wave_entry->loop_end * WAVETABLE_RESOLUTION)));
							wave->direction = 1;
						}
						
						else
						{
							wave->acc = wave->acc - (Uint64)(wave->use_end_track_status_offset ? wave->end_point_track_status : (wave_entry->loop_end * WAVETABLE_RESOLUTION)) + (Uint64)(wave->use_start_track_status_offset ? wave->start_point_track_status : (wave_entry->loop_begin * WAVETABLE_RESOLUTION));
						}
					}
				}
			}
			
			else
			{
				if (wave->acc / WAVETABLE_RESOLUTION >= (Uint64)wave_entry->samples)
				{
					// stop playback
					wave->playing = false;
				}
			}
		}
		
		else
		{
			wave->acc -= (Sint32)((Sint64)wave->frequency * ((Sint64)0x10000 - (Sint64)wave->start_offset - ((Sint64)0x10000 - (Sint64)wave->end_offset)) / (Sint64)0x10000);
			
			if ((wave_entry->flags & CYD_WAVE_LOOP) && wave_entry->loop_end != wave_entry->loop_begin)
			{
				if(!(wave->use_start_track_status_offset) && !(wave->use_end_track_status_offset))
				{
					if ((WaveAccSigned)wave->acc / WAVETABLE_RESOLUTION < (WaveAccSigned)wave_entry->loop_begin)
					{
						if (wave_entry->flags & CYD_WAVE_PINGPONG) 
						{
							wave->acc = (WaveAccSigned)wave_entry->loop_begin * WAVETABLE_RESOLUTION - ((WaveAccSigned)wave->acc - (WaveAccSigned)wave_entry->loop_begin * WAVETABLE_RESOLUTION);
							wave->direction = 0;
						}
						
						else
						{
							wave->acc = wave->acc - (Uint64)wave_entry->loop_begin * WAVETABLE_RESOLUTION + (Uint64)wave_entry->loop_end * WAVETABLE_RESOLUTION;
						}
					}
				}
				
				else
				{
					if ((wave->use_start_track_status_offset ? ((WaveAccSigned)wave->acc < (WaveAccSigned)wave->start_point_track_status) : ((WaveAccSigned)wave->acc / WAVETABLE_RESOLUTION < (WaveAccSigned)wave_entry->loop_begin)))
					{
						if (wave_entry->flags & CYD_WAVE_PINGPONG) 
						{
							wave->acc = (WaveAccSigned)(wave->use_start_track_status_offset ? wave->start_point_track_status : (wave_entry->loop_begin * WAVETABLE_RESOLUTION)) - ((WaveAccSigned)wave->acc - (WaveAccSigned)(wave->use_start_track_status_offset ? wave->start_point_track_status : (wave_entry->loop_begin * WAVETABLE_RESOLUTION)));
							wave->direction = 0;
						}
						
						else
						{
							wave->acc = wave->acc - (Uint64)(wave->use_start_track_status_offset ? wave->start_point_track_status : (wave_entry->loop_begin * WAVETABLE_RESOLUTION)) + (Uint64)(wave->use_end_track_status_offset ? wave->end_point_track_status : (wave_entry->loop_end * WAVETABLE_RESOLUTION));
						}
					}
				}
			}
			
			else
			{
				if ((WaveAccSigned)wave->acc < 0)
				{
					// stop playback
					wave->playing = false;
				}
			}
		}
	}
	
#endif // CYD_DISABLE_WAVETABLE
}

/*void cyd_wave_cycle(CydWaveState *wave, const CydWavetableEntry *wave_entry)
{
#ifndef CYD_DISABLE_WAVETABLE

	if (wave_entry && wave->playing)
	{
		if (wave->direction == 0)
		{
			wave->acc += wave->frequency;
			
			if ((wave_entry->flags & CYD_WAVE_LOOP) && wave_entry->loop_end != wave_entry->loop_begin)
			{
				if (wave->acc / WAVETABLE_RESOLUTION >= (Uint64)wave_entry->loop_end)
				{
					if (wave_entry->flags & CYD_WAVE_PINGPONG) 
					{
						wave->acc = (Uint64)wave_entry->loop_end * WAVETABLE_RESOLUTION - (wave->acc - (Uint64)wave_entry->loop_end * WAVETABLE_RESOLUTION);
						wave->direction = 1;
					}
					else
					{
						wave->acc = wave->acc - (Uint64)wave_entry->loop_end * WAVETABLE_RESOLUTION + (Uint64)wave_entry->loop_begin * WAVETABLE_RESOLUTION;
					}
				}
			}
			else
			{
				if (wave->acc / WAVETABLE_RESOLUTION >= (Uint64)wave_entry->samples)
				{
					// stop playback
					wave->playing = false;
				}
			}
		}
		else
		{
			wave->acc -= wave->frequency;
			
			if ((wave_entry->flags & CYD_WAVE_LOOP) && wave_entry->loop_end != wave_entry->loop_begin)
			{
				if ((WaveAccSigned)wave->acc / WAVETABLE_RESOLUTION < (WaveAccSigned)wave_entry->loop_begin)
				{
					if (wave_entry->flags & CYD_WAVE_PINGPONG) 
					{
						wave->acc = (WaveAccSigned)wave_entry->loop_begin * WAVETABLE_RESOLUTION - ((WaveAccSigned)wave->acc - (WaveAccSigned)wave_entry->loop_begin * WAVETABLE_RESOLUTION);
						wave->direction = 0;
					}
					else
					{
						wave->acc = wave->acc - (Uint64)wave_entry->loop_begin * WAVETABLE_RESOLUTION + (Uint64)wave_entry->loop_end * WAVETABLE_RESOLUTION;
					}
				}
			}
			else
			{
				if ((WaveAccSigned)wave->acc < 0)
				{
					// stop playback
					wave->playing = false;
				}
			}
		}
	}
	
#endif // CYD_DISABLE_WAVETABLE
}*/