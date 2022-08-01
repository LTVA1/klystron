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
	(Uint32)(0.510987 * 1024), //-5 (negative) octave
	(Uint32)(0.541372 * 1024),
	(Uint32)(0.573564 * 1024),
	(Uint32)(0.60767 * 1024),
	(Uint32)(0.643804 * 1024),
	(Uint32)(0.682086 * 1024),
	(Uint32)(0.722646 * 1024),
	(Uint32)(0.765616 * 1024),
	(Uint32)(0.811142 * 1024),
	(Uint32)(0.859375 * 1024),
	(Uint32)(0.910476 * 1024),
	(Uint32)(0.964616 * 1024),
	
	(Uint32)(1.02197 * 1024), //-4 (negative) octave
	(Uint32)(1.08274 * 1024),
	(Uint32)(1.14713 * 1024),
	(Uint32)(1.21534 * 1024),
	(Uint32)(1.28761 * 1024),
	(Uint32)(1.36417 * 1024),
	(Uint32)(1.44529 * 1024),
	(Uint32)(1.53123 * 1024),
	(Uint32)(1.62228 * 1024),
	(Uint32)(1.71875 * 1024),
	(Uint32)(1.82095 * 1024),
	(Uint32)(1.92923 * 1024),
	
	(Uint32)(2.04395 * 1024), //-3 (negative) octave
	(Uint32)(2.16549 * 1024),
	(Uint32)(2.29426 * 1024),
	(Uint32)(2.43068 * 1024),
	(Uint32)(2.57522 * 1024),
	(Uint32)(2.72835 * 1024),
	(Uint32)(2.89058 * 1024),
	(Uint32)(3.06246 * 1024),
	(Uint32)(3.24457 * 1024),
	(Uint32)(3.4375 * 1024),
	(Uint32)(3.6419 * 1024),
	(Uint32)(3.85846 * 1024),
	
	(Uint32)(4.0879 * 1024), //-2 (negative) octave
	(Uint32)(4.33098 * 1024),
	(Uint32)(4.58851 * 1024),
	(Uint32)(4.86136 * 1024),
	(Uint32)(5.15043 * 1024),
	(Uint32)(5.45669 * 1024),
	(Uint32)(5.78116 * 1024),
	(Uint32)(6.12493 * 1024),
	(Uint32)(6.48914 * 1024),
	(Uint32)(6.875 * 1024),
	(Uint32)(7.28381 * 1024),
	(Uint32)(7.71692 * 1024),
	
	(Uint32)(8.175799 * 1024), //-1 (negative) octave
	(Uint32)(8.661957 * 1024),
	(Uint32)(9.177024 * 1024),
	(Uint32)(9.722718 * 1024),
	(Uint32)(10.30086 * 1024),
	(Uint32)(10.91338 * 1024),
	(Uint32)(11.56233 * 1024),
	(Uint32)(12.24986 * 1024),
	(Uint32)(12.97827 * 1024),
	(Uint32)(13.75000 * 1024),
	(Uint32)(14.56762 * 1024),
	(Uint32)(15.43385 * 1024),
	
	(Uint32)(16.35 * 1024), //0 octave
	(Uint32)(17.32 * 1024),
	(Uint32)(18.35 * 1024),
	(Uint32)(19.45 * 1024),
	(Uint32)(20.60 * 1024),
	(Uint32)(21.83 * 1024),
	(Uint32)(23.12 * 1024),
	(Uint32)(24.50 * 1024),
	(Uint32)(25.96 * 1024),
	(Uint32)(27.50 * 1024),
	(Uint32)(29.14 * 1024),
	(Uint32)(30.87 * 1024),
	
	(Uint32)(32.70 * 1024), //1st
	(Uint32)(34.65 * 1024),
	(Uint32)(36.71 * 1024),
	(Uint32)(38.89 * 1024),
	(Uint32)(41.20 * 1024),
	(Uint32)(43.65 * 1024),
	(Uint32)(46.25 * 1024),
	(Uint32)(49.00 * 1024),
	(Uint32)(51.91 * 1024),
	(Uint32)(55.00 * 1024),
	(Uint32)(58.27 * 1024),
	(Uint32)(61.74 * 1024),
	
	(Uint32)(65.41 * 1024), //2nd
	(Uint32)(69.30 * 1024),
	(Uint32)(73.42 * 1024),
	(Uint32)(77.78 * 1024),
	(Uint32)(82.41 * 1024),
	(Uint32)(87.31 * 1024),
	(Uint32)(92.50 * 1024),
	(Uint32)(98.00 * 1024),
	(Uint32)(103.83 * 1024),
	(Uint32)(110.00 * 1024),
	(Uint32)(116.54 * 1024),
	(Uint32)(123.47 * 1024),
	
	(Uint32)(130.81 * 1024), //3rd
	(Uint32)(138.59 * 1024),
	(Uint32)(146.83 * 1024),
	(Uint32)(155.56 * 1024),
	(Uint32)(164.81 * 1024),
	(Uint32)(174.61 * 1024),
	(Uint32)(185.00 * 1024),
	(Uint32)(196.00 * 1024),
	(Uint32)(207.65 * 1024),
	(Uint32)(220.00 * 1024),
	(Uint32)(233.08 * 1024),
	(Uint32)(246.94 * 1024),
	
	(Uint32)(261.63 * 1024), //4th
	(Uint32)(277.18 * 1024),
	(Uint32)(293.66 * 1024),
	(Uint32)(311.13 * 1024),
	(Uint32)(329.63 * 1024),
	(Uint32)(349.23 * 1024),
	(Uint32)(369.99 * 1024),
	(Uint32)(392.00 * 1024),
	(Uint32)(415.30 * 1024),
	(Uint32)(440.00 * 1024),
	(Uint32)(466.16 * 1024),
	(Uint32)(493.88 * 1024),
	
	(Uint32)(523.25 * 1024), //5th
	(Uint32)(554.37 * 1024),
	(Uint32)(587.33 * 1024),
	(Uint32)(622.25 * 1024),
	(Uint32)(659.26 * 1024),
	(Uint32)(698.46 * 1024),
	(Uint32)(739.99 * 1024),
	(Uint32)(783.99 * 1024),
	(Uint32)(830.61 * 1024),
	(Uint32)(880.00 * 1024),
	(Uint32)(932.33 * 1024),
	(Uint32)(987.77 * 1024),
	
	(Uint32)(1046.50 * 1024), //6th
	(Uint32)(1108.73 * 1024),
	(Uint32)(1174.66 * 1024),
	(Uint32)(1244.51 * 1024),
	(Uint32)(1318.51 * 1024),
	(Uint32)(1396.91 * 1024),
	(Uint32)(1479.98 * 1024),
	(Uint32)(1567.98 * 1024),
	(Uint32)(1661.22 * 1024),
	(Uint32)(1760.00 * 1024),
	(Uint32)(1864.66 * 1024),
	(Uint32)(1975.53 * 1024),
	
	(Uint32)(2093.00 * 1024), //7th
	(Uint32)(2217.46 * 1024),
	(Uint32)(2349.32 * 1024),
	(Uint32)(2489.02 * 1024),
	(Uint32)(2637.02 * 1024),
	(Uint32)(2793.83 * 1024),
	(Uint32)(2959.96 * 1024),
	(Uint32)(3135.96 * 1024),
	(Uint32)(3322.44 * 1024),
	(Uint32)(3520.00 * 1024),
	(Uint32)(3729.31 * 1024),
	(Uint32)(3951.07 * 1024),
	
	(Uint32)(4186.01 * 1024), //one more octave, C-8 to B-8
	(Uint32)(4434.92 * 1024),
	(Uint32)(4698.63 * 1024),
	(Uint32)(4978.03 * 1024),
	(Uint32)(5274.04 * 1024),
	(Uint32)(5587.65 * 1024),
	(Uint32)(5919.91 * 1024),
	(Uint32)(6271.93 * 1024),
	(Uint32)(6644.88 * 1024),
	(Uint32)(7040.00 * 1024),
	(Uint32)(7458.62 * 1024),
	(Uint32)(7902.13 * 1024),
	
	(Uint32)(8372.02 * 1024), //ok, one even more, C-9 to B-9, because after I'd need to draw special chars for octave numbers (and frequency goes beyond what human can hear)
	(Uint32)(8869.84 * 1024),
	(Uint32)(9397.27 * 1024),
	(Uint32)(9956.06 * 1024),
	(Uint32)(10548.08 * 1024),
	(Uint32)(11175.30 * 1024),
	(Uint32)(11839.82 * 1024),
	(Uint32)(12543.85 * 1024),
	(Uint32)(13289.75 * 1024),
	(Uint32)(14080.00 * 1024),
	(Uint32)(14917.24 * 1024),
	(Uint32)(15804.27 * 1024),
	
	/*
	(Uint32)(16744.04 * 1024), //10th octave (unused)
	(Uint32)(17739.69 * 1024),
	(Uint32)(18794.55 * 1024),
	(Uint32)(19912.13 * 1024),
	(Uint32)(21096.16 * 1024),
	(Uint32)(22350.61 * 1024),
	(Uint32)(23679.64 * 1024),
	(Uint32)(25087.71 * 1024),
	(Uint32)(26579.50 * 1024),
	(Uint32)(28160.00 * 1024),
	(Uint32)(29834.48 * 1024),
	(Uint32)(31608.53 * 1024),*/
};


Uint32 get_freq(int note)
{
	if (note <= 0)
	{
		//debug("freq sent 1 %d Hz", frequency_table[0]);
		return frequency_table[0];
	}
		
	if (note >= (FREQ_TAB_SIZE << 8))
	{
		//debug("freq sent 2 %d Hz", frequency_table[FREQ_TAB_SIZE - 1]);
		return frequency_table[FREQ_TAB_SIZE - 1];
	}

	if ((note & 0xff) == 0)
	{
		//debug("freq sent 3 %d Hz", frequency_table[(note >> 8)]);
		return frequency_table[(note >> 8)];
	}
	
	else
	{
		Uint64 f1 = frequency_table[(note >> 8)];
		Uint64 f2 = frequency_table[((note >> 8) + 1)];
		
		//debug("freq sent 4 %d Hz", f1 + ((f2 - f1) * (note & 0xff)) / 256);
		
		return f1 + (Uint64)((f2 - f1) * (note & 0xff)) / 256;
	}
}

