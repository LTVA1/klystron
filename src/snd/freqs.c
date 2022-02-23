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

#include "freqs.h"

const Uint32 frequency_table[FREQ_TAB_SIZE] =
{
	(Uint32)(16.35 * 16),
	(Uint32)(17.32 * 16),
	(Uint32)(18.35 * 16),
	(Uint32)(19.45 * 16),
	(Uint32)(20.60 * 16),
	(Uint32)(21.83 * 16),
	(Uint32)(23.12 * 16),
	(Uint32)(24.50 * 16),
	(Uint32)(25.96 * 16),
	(Uint32)(27.50 * 16),
	(Uint32)(29.14 * 16),
	(Uint32)(30.87 * 16),
	(Uint32)(32.70 * 16),
	(Uint32)(34.65 * 16),
	(Uint32)(36.71 * 16),
	(Uint32)(38.89 * 16),
	(Uint32)(41.20 * 16),
	(Uint32)(43.65 * 16),
	(Uint32)(46.25 * 16),
	(Uint32)(49.00 * 16),
	(Uint32)(51.91 * 16),
	(Uint32)(55.00 * 16),
	(Uint32)(58.27 * 16),
	(Uint32)(61.74 * 16),
	(Uint32)(65.41 * 16),
	(Uint32)(69.30 * 16),
	(Uint32)(73.42 * 16),
	(Uint32)(77.78 * 16),
	(Uint32)(82.41 * 16),
	(Uint32)(87.31 * 16),
	(Uint32)(92.50 * 16),
	(Uint32)(98.00 * 16),
	(Uint32)(103.83 * 16),
	(Uint32)(110.00 * 16),
	(Uint32)(116.54 * 16),
	(Uint32)(123.47 * 16),
	(Uint32)(130.81 * 16),
	(Uint32)(138.59 * 16),
	(Uint32)(146.83 * 16),
	(Uint32)(155.56 * 16),
	(Uint32)(164.81 * 16),
	(Uint32)(174.61 * 16),
	(Uint32)(185.00 * 16),
	(Uint32)(196.00 * 16),
	(Uint32)(207.65 * 16),
	(Uint32)(220.00 * 16),
	(Uint32)(233.08 * 16),
	(Uint32)(246.94 * 16),
	(Uint32)(261.63 * 16),
	(Uint32)(277.18 * 16),
	(Uint32)(293.66 * 16),
	(Uint32)(311.13 * 16),
	(Uint32)(329.63 * 16),
	(Uint32)(349.23 * 16),
	(Uint32)(369.99 * 16),
	(Uint32)(392.00 * 16),
	(Uint32)(415.30 * 16),
	(Uint32)(440.00 * 16),
	(Uint32)(466.16 * 16),
	(Uint32)(493.88 * 16),
	(Uint32)(523.25 * 16),
	(Uint32)(554.37 * 16),
	(Uint32)(587.33 * 16),
	(Uint32)(622.25 * 16),
	(Uint32)(659.26 * 16),
	(Uint32)(698.46 * 16),
	(Uint32)(739.99 * 16),
	(Uint32)(783.99 * 16),
	(Uint32)(830.61 * 16),
	(Uint32)(880.00 * 16),
	(Uint32)(932.33 * 16),
	(Uint32)(987.77 * 16),
	(Uint32)(1046.50 * 16),
	(Uint32)(1108.73 * 16),
	(Uint32)(1174.66 * 16),
	(Uint32)(1244.51 * 16),
	(Uint32)(1318.51 * 16),
	(Uint32)(1396.91 * 16),
	(Uint32)(1479.98 * 16),
	(Uint32)(1567.98 * 16),
	(Uint32)(1661.22 * 16),
	(Uint32)(1760.00 * 16),
	(Uint32)(1864.66 * 16),
	(Uint32)(1975.53 * 16),
	(Uint32)(2093.00 * 16),
	(Uint32)(2217.46 * 16),
	(Uint32)(2349.32 * 16),
	(Uint32)(2489.02 * 16),
	(Uint32)(2637.02 * 16),
	(Uint32)(2793.83 * 16),
	(Uint32)(2959.96 * 16),
	(Uint32)(3135.96 * 16),
	(Uint32)(3322.44 * 16),
	(Uint32)(3520.00 * 16),
	(Uint32)(3729.31 * 16),
	(Uint32)(3951.07 * 16),
	
	(Uint32)(4186.01 * 16), //one more octave, C-8 to B-8
	(Uint32)(4434.92 * 16),
	(Uint32)(4698.63 * 16),
	(Uint32)(4978.03 * 16),
	(Uint32)(5274.04 * 16),
	(Uint32)(5587.65 * 16),
	(Uint32)(5919.91 * 16),
	(Uint32)(6271.93 * 16),
	(Uint32)(6644.88 * 16),
	(Uint32)(7040.00 * 16),
	(Uint32)(7458.62 * 16),
	(Uint32)(7902.13 * 16),
	
	(Uint32)(8372.02 * 16), //ok, one even more, C-9 to B-9, because after I'd need to draw special chars for octave numbers (and frequency goes beyond what human can hear)
	(Uint32)(8869.84 * 16),
	(Uint32)(9397.27 * 16),
	(Uint32)(9956.06 * 16),
	(Uint32)(10548.08 * 16),
	(Uint32)(11175.30 * 16),
	(Uint32)(11839.82 * 16),
	(Uint32)(12543.85 * 16),
	(Uint32)(13289.75 * 16),
	(Uint32)(14080.00 * 16),
	(Uint32)(14917.24 * 16),
	(Uint32)(15804.27 * 16),
};


Uint32 get_freq(int note)
{
	if (note <= 0)
	{
		//debug("freq sent %d Hz", frequency_table[0]);
		return frequency_table[0];
	}
		
	if (note >= (FREQ_TAB_SIZE << 8))
	{
		//debug("freq sent %d Hz", frequency_table[FREQ_TAB_SIZE - 1]);
		return frequency_table[FREQ_TAB_SIZE - 1];
	}

	if ((note & 0xff) == 0)
	{
		//debug("freq sent %d Hz", frequency_table[(note >> 8)]);
		return frequency_table[(note >> 8)];
	}
	
	else
	{
		Uint32 f1 = frequency_table[(note >> 8)];
		Uint32 f2 = frequency_table[((note >> 8) + 1)];
		
		//debug("freq sent %d Hz", f1 + ((f2-f1) * (note & 0xff)) / 256);
		
		return f1 + (Uint64)((f2-f1) * (note & 0xff)) / 256;
	}
}

