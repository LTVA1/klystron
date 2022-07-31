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
	/*
	(Uint32)(8.175799 * 64), //-1 (negative) octave (unused)
	(Uint32)(8.661957 * 64),
	(Uint32)(9.177024 * 64),
	(Uint32)(9.722718 * 64),
	(Uint32)(10.30086 * 64),
	(Uint32)(10.91338 * 64),
	(Uint32)(11.56233 * 64),
	(Uint32)(12.24986 * 64),
	(Uint32)(12.97827 * 64),
	(Uint32)(13.75000 * 64),
	(Uint32)(14.56762 * 64),
	(Uint32)(15.43385 * 64),*/
	
	(Uint32)(16.35 * 64), //0 octave
	(Uint32)(17.32 * 64),
	(Uint32)(18.35 * 64),
	(Uint32)(19.45 * 64),
	(Uint32)(20.60 * 64),
	(Uint32)(21.83 * 64),
	(Uint32)(23.12 * 64),
	(Uint32)(24.50 * 64),
	(Uint32)(25.96 * 64),
	(Uint32)(27.50 * 64),
	(Uint32)(29.14 * 64),
	(Uint32)(30.87 * 64),
	
	(Uint32)(32.70 * 64), //1st
	(Uint32)(34.65 * 64),
	(Uint32)(36.71 * 64),
	(Uint32)(38.89 * 64),
	(Uint32)(41.20 * 64),
	(Uint32)(43.65 * 64),
	(Uint32)(46.25 * 64),
	(Uint32)(49.00 * 64),
	(Uint32)(51.91 * 64),
	(Uint32)(55.00 * 64),
	(Uint32)(58.27 * 64),
	(Uint32)(61.74 * 64),
	
	(Uint32)(65.41 * 64), //2nd
	(Uint32)(69.30 * 64),
	(Uint32)(73.42 * 64),
	(Uint32)(77.78 * 64),
	(Uint32)(82.41 * 64),
	(Uint32)(87.31 * 64),
	(Uint32)(92.50 * 64),
	(Uint32)(98.00 * 64),
	(Uint32)(103.83 * 64),
	(Uint32)(110.00 * 64),
	(Uint32)(116.54 * 64),
	(Uint32)(123.47 * 64),
	
	(Uint32)(130.81 * 64), //3rd
	(Uint32)(138.59 * 64),
	(Uint32)(146.83 * 64),
	(Uint32)(155.56 * 64),
	(Uint32)(164.81 * 64),
	(Uint32)(174.61 * 64),
	(Uint32)(185.00 * 64),
	(Uint32)(196.00 * 64),
	(Uint32)(207.65 * 64),
	(Uint32)(220.00 * 64),
	(Uint32)(233.08 * 64),
	(Uint32)(246.94 * 64),
	
	(Uint32)(261.63 * 64), //4th
	(Uint32)(277.18 * 64),
	(Uint32)(293.66 * 64),
	(Uint32)(311.13 * 64),
	(Uint32)(329.63 * 64),
	(Uint32)(349.23 * 64),
	(Uint32)(369.99 * 64),
	(Uint32)(392.00 * 64),
	(Uint32)(415.30 * 64),
	(Uint32)(440.00 * 64),
	(Uint32)(466.16 * 64),
	(Uint32)(493.88 * 64),
	
	(Uint32)(523.25 * 64), //5th
	(Uint32)(554.37 * 64),
	(Uint32)(587.33 * 64),
	(Uint32)(622.25 * 64),
	(Uint32)(659.26 * 64),
	(Uint32)(698.46 * 64),
	(Uint32)(739.99 * 64),
	(Uint32)(783.99 * 64),
	(Uint32)(830.61 * 64),
	(Uint32)(880.00 * 64),
	(Uint32)(932.33 * 64),
	(Uint32)(987.77 * 64),
	
	(Uint32)(1046.50 * 64), //6th
	(Uint32)(1108.73 * 64),
	(Uint32)(1174.66 * 64),
	(Uint32)(1244.51 * 64),
	(Uint32)(1318.51 * 64),
	(Uint32)(1396.91 * 64),
	(Uint32)(1479.98 * 64),
	(Uint32)(1567.98 * 64),
	(Uint32)(1661.22 * 64),
	(Uint32)(1760.00 * 64),
	(Uint32)(1864.66 * 64),
	(Uint32)(1975.53 * 64),
	
	(Uint32)(2093.00 * 64), //7th
	(Uint32)(2217.46 * 64),
	(Uint32)(2349.32 * 64),
	(Uint32)(2489.02 * 64),
	(Uint32)(2637.02 * 64),
	(Uint32)(2793.83 * 64),
	(Uint32)(2959.96 * 64),
	(Uint32)(3135.96 * 64),
	(Uint32)(3322.44 * 64),
	(Uint32)(3520.00 * 64),
	(Uint32)(3729.31 * 64),
	(Uint32)(3951.07 * 64),
	
	(Uint32)(4186.01 * 64), //one more octave, C-8 to B-8
	(Uint32)(4434.92 * 64),
	(Uint32)(4698.63 * 64),
	(Uint32)(4978.03 * 64),
	(Uint32)(5274.04 * 64),
	(Uint32)(5587.65 * 64),
	(Uint32)(5919.91 * 64),
	(Uint32)(6271.93 * 64),
	(Uint32)(6644.88 * 64),
	(Uint32)(7040.00 * 64),
	(Uint32)(7458.62 * 64),
	(Uint32)(7902.13 * 64),
	
	(Uint32)(8372.02 * 64), //ok, one even more, C-9 to B-9, because after I'd need to draw special chars for octave numbers (and frequency goes beyond what human can hear)
	(Uint32)(8869.84 * 64),
	(Uint32)(9397.27 * 64),
	(Uint32)(9956.06 * 64),
	(Uint32)(10548.08 * 64),
	(Uint32)(11175.30 * 64),
	(Uint32)(11839.82 * 64),
	(Uint32)(12543.85 * 64),
	(Uint32)(13289.75 * 64),
	(Uint32)(14080.00 * 64),
	(Uint32)(14917.24 * 64),
	(Uint32)(15804.27 * 64),
	
	/*
	(Uint32)(16744.04 * 64), //10th octave (unused)
	(Uint32)(17739.69 * 64),
	(Uint32)(18794.55 * 64),
	(Uint32)(19912.13 * 64),
	(Uint32)(21096.16 * 64),
	(Uint32)(22350.61 * 64),
	(Uint32)(23679.64 * 64),
	(Uint32)(25087.71 * 64),
	(Uint32)(26579.50 * 64),
	(Uint32)(28160.00 * 64),
	(Uint32)(29834.48 * 64),
	(Uint32)(31608.53 * 64),*/
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
		Uint64 f1 = frequency_table[(note >> 8)];
		Uint64 f2 = frequency_table[((note >> 8) + 1)];
		
		//debug("freq sent %d Hz", f1 + ((f2-f1) * (note & 0xff)) / 256);
		
		return f1 + (Uint64)((f2 - f1) * (note & 0xff)) / 256;
	}
}

