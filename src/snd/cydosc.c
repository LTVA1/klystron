#include "cydosc.h"
#include "cyddefs.h"
#include "cyd.h"

#include "../../../src/combWFgen.h"

#include "../macros.h"

static inline Uint32 cyd_pulse(Uint32 acc, Uint32 pw) 
{
	return (((acc >> ((ACC_BITS - 17))) >= (pw << 4) ? (WAVE_AMP - 1) : 0));
}


static inline Uint32 cyd_saw(Uint32 acc) 
{
	return (acc >> (ACC_BITS - OUTPUT_BITS - 1)) & (WAVE_AMP - 1);
}


static inline Uint32 cyd_triangle(Uint32 acc)
{
	return ((((acc & (ACC_LENGTH / 2)) ? ~acc : acc) >> (ACC_BITS - OUTPUT_BITS - 2)) & (WAVE_AMP * 2 - 1));
}


static inline Uint32 cyd_noise(Uint32 acc) 
{
	return acc & (WAVE_AMP - 1);
}

#ifndef CYD_DISABLE_LFSR
Uint32 cyd_lfsr(Uint32 bits) 
{
	return bits;
}
#endif

Sint32 cyd_osc(Uint32 flags, Uint32 accumulator, Uint32 pw, Uint32 random, Uint32 lfsr_acc, Uint8 mixmode) //Sint32 cyd_osc(Uint32 flags, Uint32 accumulator, Uint32 pw, Uint32 random, Uint32 lfsr_acc)
{
	switch (flags & WAVEFORMS & ~CYD_CHN_ENABLE_WAVE)
	{
		case CYD_CHN_ENABLE_PULSE:
		return cyd_pulse(accumulator, pw);
		break;
		
		case CYD_CHN_ENABLE_SAW:
		return cyd_saw(accumulator);
		break;
		
		case CYD_CHN_ENABLE_TRIANGLE:
		return cyd_triangle(accumulator);
		break;
		
		case CYD_CHN_ENABLE_NOISE:
		return ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random)));
		break;
		
		case CYD_CHN_ENABLE_TRIANGLE|CYD_CHN_ENABLE_PULSE:
		if(mixmode == 0) //bitwise AND (default)
		{
			return cyd_pulse(accumulator, pw) & cyd_triangle(accumulator);
		}
		
		if(mixmode == 1) //sum of oscillators' signals
		{
			return (cyd_pulse(accumulator, pw) + cyd_triangle(accumulator)) / 2;
		}
		
		if(mixmode == 2) //bitwise OR
		{
			return cyd_pulse(accumulator, pw) | cyd_triangle(accumulator);
		}
		
		if(mixmode == 3) //C64 8580 SID combined waves from 4096 samples-long arrays that are generated at every startup of the tracker (except pulse+tri which is sample)
		{
			return (PulseTri_8580[cyd_saw(accumulator) / 8] & cyd_pulse(accumulator, pw));
		}
		
		if(mixmode == 4) //bitwise XOR
		{
			return cyd_pulse(accumulator, pw) ^ cyd_triangle(accumulator);
		}
		break;
		
		case CYD_CHN_ENABLE_SAW|CYD_CHN_ENABLE_PULSE:
		if(mixmode == 0)
		{
			return cyd_saw(accumulator) & cyd_pulse(accumulator, pw);
		}
		
		if(mixmode == 1)
		{
			return (cyd_saw(accumulator) + cyd_pulse(accumulator, pw)) / 2;
		}
		
		if(mixmode == 2)
		{
			return cyd_saw(accumulator) | cyd_pulse(accumulator, pw);
		}
		
		if(mixmode == 3)
		{
			return PulseSaw_8580[cyd_saw(accumulator) / 16];
		}
		
		if(mixmode == 4)
		{
			return cyd_saw(accumulator) ^ cyd_pulse(accumulator, pw);
		}
		break;
		
		case CYD_CHN_ENABLE_NOISE|CYD_CHN_ENABLE_PULSE:
		if(mixmode == 0 || mixmode == 3)
		{
			return ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) & cyd_pulse(accumulator, pw);
		}
		
		if(mixmode == 1)
		{
			return (((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) + cyd_pulse(accumulator, pw)) / 2;
		}
		
		if(mixmode == 2)
		{
			return ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) | cyd_pulse(accumulator, pw);
		}
		
		if(mixmode == 4)
		{
			return ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) ^ cyd_pulse(accumulator, pw);
		}
		break;
		
		case CYD_CHN_ENABLE_TRIANGLE|CYD_CHN_ENABLE_SAW:
		if(mixmode == 0)
		{
			return cyd_triangle(accumulator) & cyd_saw(accumulator);
		}
		
		if(mixmode == 1)
		{
			return (cyd_triangle(accumulator) + cyd_saw(accumulator)) / 2;
		}
		
		if(mixmode == 2)
		{
			return cyd_triangle(accumulator) | cyd_saw(accumulator);
		}
		
		if(mixmode == 3)
		{
			return TriSaw_8580[cyd_saw(accumulator) / 16];
		}
		
		if(mixmode == 4)
		{
			return cyd_triangle(accumulator) ^ cyd_saw(accumulator);
		}
		break;
		
		case CYD_CHN_ENABLE_NOISE|CYD_CHN_ENABLE_SAW:
		if(mixmode == 0 || mixmode == 3)
		{
			return ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) & cyd_saw(accumulator);
		}
		
		if(mixmode == 1)
		{
			return (((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) + cyd_saw(accumulator)) / 2;
		}
		
		if(mixmode == 2)
		{
			return ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) | cyd_saw(accumulator);
		}
		
		if(mixmode == 4)
		{
			return ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) ^ cyd_saw(accumulator);
		}
		break;
		
		case CYD_CHN_ENABLE_NOISE|CYD_CHN_ENABLE_TRIANGLE:
		if(mixmode == 0 || mixmode == 3)
		{
			return ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) & cyd_triangle(accumulator);
		}
		
		if(mixmode == 1)
		{
			return (((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) + cyd_triangle(accumulator)) / 2;
		}
		
		if(mixmode == 2)
		{
			return ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) | cyd_triangle(accumulator);
		}
		
		if(mixmode == 4)
		{
			return ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) ^ cyd_triangle(accumulator);
		}
		break;
		
		case CYD_CHN_ENABLE_TRIANGLE|CYD_CHN_ENABLE_PULSE|CYD_CHN_ENABLE_SAW:
		if(mixmode == 0)
		{
			return cyd_pulse(accumulator, pw) & cyd_triangle(accumulator) & cyd_saw(accumulator);
		}
		
		if(mixmode == 1)
		{
			return (cyd_pulse(accumulator, pw) + cyd_triangle(accumulator) + cyd_saw(accumulator)) / 3;
		}
		
		if(mixmode == 2)
		{
			return cyd_pulse(accumulator, pw) | cyd_triangle(accumulator) | cyd_saw(accumulator);
		}
		
		if(mixmode == 3)
		{
			return PulseTriSaw_8580[cyd_saw(accumulator) / 16];
		}
		
		if(mixmode == 4)
		{
			return cyd_pulse(accumulator, pw) ^ cyd_triangle(accumulator) ^ cyd_saw(accumulator);
		}
		break;
		
		case CYD_CHN_ENABLE_TRIANGLE|CYD_CHN_ENABLE_PULSE|CYD_CHN_ENABLE_NOISE:
		if(mixmode == 0)
		{
			return cyd_pulse(accumulator, pw) & cyd_triangle(accumulator) & ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random)));
		}
		
		if(mixmode == 1)
		{
			return (cyd_pulse(accumulator, pw) + cyd_triangle(accumulator) + ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random)))) / 3;
		}
		
		if(mixmode == 2)
		{
			return cyd_pulse(accumulator, pw) | cyd_triangle(accumulator) | ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random)));
		}
		
		if(mixmode == 3)
		{
			return ((PulseTri_8580[cyd_saw(accumulator) / 8] & cyd_pulse(accumulator, pw))) & ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random)));
		}
		
		if(mixmode == 4)
		{
			return cyd_pulse(accumulator, pw) ^ cyd_triangle(accumulator) ^ ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random)));
		}
		break;
		
		case CYD_CHN_ENABLE_TRIANGLE|CYD_CHN_ENABLE_NOISE|CYD_CHN_ENABLE_SAW:
		if(mixmode == 0)
		{
			return cyd_saw(accumulator) & cyd_triangle(accumulator) & ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random)));
		}
		
		if(mixmode == 1)
		{
			return (cyd_saw(accumulator) + cyd_triangle(accumulator) + ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random)))) / 3;
		}
		
		if(mixmode == 2)
		{
			return cyd_saw(accumulator) | cyd_triangle(accumulator) | ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random)));
		}
		
		if(mixmode == 3)
		{
			return (TriSaw_8580[cyd_saw(accumulator) / 16]) & ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random)));
		}
		
		if(mixmode == 4)
		{
			return cyd_saw(accumulator) ^ cyd_triangle(accumulator) ^ ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random)));
		}
		break;
		
		case CYD_CHN_ENABLE_PULSE|CYD_CHN_ENABLE_NOISE|CYD_CHN_ENABLE_SAW:
		if(mixmode == 0)
		{
			return cyd_pulse(accumulator, pw) & cyd_saw(accumulator) & ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random)));
		}
		
		if(mixmode == 1)
		{
			return (cyd_saw(accumulator) + cyd_pulse(accumulator, pw) + ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random)))) / 3;
		}
		
		if(mixmode == 2)
		{
			return cyd_pulse(accumulator, pw) | cyd_saw(accumulator) | ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random)));
		}
		
		if(mixmode == 3)
		{
			return (PulseSaw_8580[cyd_saw(accumulator) / 16]) & ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random)));
		}
		
		if(mixmode == 4)
		{
			return cyd_pulse(accumulator, pw) ^ cyd_saw(accumulator) ^ ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random)));
		}
		break;
		
		case CYD_CHN_ENABLE_NOISE|CYD_CHN_ENABLE_SAW|CYD_CHN_ENABLE_PULSE|CYD_CHN_ENABLE_TRIANGLE:
		if(mixmode == 0)
		{
			return cyd_saw(accumulator) & cyd_pulse(accumulator, pw) & cyd_triangle(accumulator) & ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random)));
		}
		
		if(mixmode == 1)
		{
			return (cyd_saw(accumulator) + cyd_pulse(accumulator, pw) + cyd_triangle(accumulator) + ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random)))) / 4;
		}
		
		if(mixmode == 2)
		{
			return cyd_saw(accumulator) | cyd_pulse(accumulator, pw) | cyd_triangle(accumulator) | ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random)));
		}
		
		if(mixmode == 3)
		{
			return (PulseTriSaw_8580[cyd_saw(accumulator) / 16]) & ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random)));
		}
		
		if(mixmode == 4)
		{
			return cyd_saw(accumulator) ^ cyd_pulse(accumulator, pw) ^ cyd_triangle(accumulator) ^ ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random)));
		}
		break;
		
#ifndef CYD_DISABLE_LFSR			
		case CYD_CHN_ENABLE_LFSR:
		return cyd_lfsr(lfsr_acc);
		break;
		
		case CYD_CHN_ENABLE_PULSE|CYD_CHN_ENABLE_LFSR:
		if(mixmode == 0 || mixmode == 3)
		{
			return cyd_pulse(accumulator, pw) & cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 1)
		{
			return (cyd_pulse(accumulator, pw) + cyd_lfsr(lfsr_acc)) / 2;
		}
		
		if(mixmode == 2)
		{
			return cyd_pulse(accumulator, pw) | cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 4)
		{
			return cyd_pulse(accumulator, pw) ^ cyd_lfsr(lfsr_acc);
		}
		break;
		
		case CYD_CHN_ENABLE_SAW|CYD_CHN_ENABLE_LFSR:
		if(mixmode == 0 || mixmode == 3)
		{
			return cyd_saw(accumulator) & cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 1)
		{
			return (cyd_saw(accumulator) + cyd_lfsr(lfsr_acc)) / 2;
		}
		
		if(mixmode == 2)
		{
			return cyd_saw(accumulator) | cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 4)
		{
			return cyd_saw(accumulator) ^ cyd_lfsr(lfsr_acc);
		}
		break;
		
		case CYD_CHN_ENABLE_TRIANGLE|CYD_CHN_ENABLE_LFSR:
		if(mixmode == 0 || mixmode == 3)
		{
			return cyd_triangle(accumulator) & cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 1)
		{
			return (cyd_triangle(accumulator) + cyd_lfsr(lfsr_acc)) / 2;
		}
		
		if(mixmode == 2)
		{
			return cyd_triangle(accumulator) | cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 4)
		{
			return cyd_triangle(accumulator) ^ cyd_lfsr(lfsr_acc);
		}
		break;
		
		case CYD_CHN_ENABLE_NOISE|CYD_CHN_ENABLE_LFSR:
		if(mixmode == 0 || mixmode == 3)
		{
			return ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) & cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 1)
		{
			return (((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) + cyd_lfsr(lfsr_acc)) / 2;
		}
		
		if(mixmode == 2)
		{
			return ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) | cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 4)
		{
			return ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) ^ cyd_lfsr(lfsr_acc);
		}
		break;
		
		case CYD_CHN_ENABLE_TRIANGLE|CYD_CHN_ENABLE_PULSE|CYD_CHN_ENABLE_LFSR:
		if(mixmode == 0)
		{
			return cyd_pulse(accumulator, pw) & cyd_triangle(accumulator) & cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 1)
		{
			return (cyd_pulse(accumulator, pw) + cyd_triangle(accumulator) + cyd_lfsr(lfsr_acc)) / 3;
		}
		
		if(mixmode == 2)
		{
			return cyd_pulse(accumulator, pw) | cyd_triangle(accumulator) | cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 3)
		{
			return ((PulseTri_8580[cyd_saw(accumulator) / 8]) & cyd_pulse(accumulator, pw)) & cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 4)
		{
			return cyd_pulse(accumulator, pw) ^ cyd_triangle(accumulator) ^ cyd_lfsr(lfsr_acc);
		}
		break;
		
		case CYD_CHN_ENABLE_SAW|CYD_CHN_ENABLE_PULSE|CYD_CHN_ENABLE_LFSR:
		if(mixmode == 0)
		{
			return cyd_pulse(accumulator, pw) & cyd_saw(accumulator) & cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 1)
		{
			return (cyd_pulse(accumulator, pw) + cyd_saw(accumulator) + cyd_lfsr(lfsr_acc)) / 3;
		}
		
		if(mixmode == 2)
		{
			return cyd_pulse(accumulator, pw) | cyd_saw(accumulator) | cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 3)
		{
			return (PulseSaw_8580[cyd_saw(accumulator) / 16]) & cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 4)
		{
			return cyd_pulse(accumulator, pw) ^ cyd_saw(accumulator) ^ cyd_lfsr(lfsr_acc);
		}
		break;
		
		case CYD_CHN_ENABLE_NOISE|CYD_CHN_ENABLE_PULSE|CYD_CHN_ENABLE_LFSR:
		if(mixmode == 0 || mixmode == 3)
		{
			return cyd_pulse(accumulator, pw) & ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) & cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 1)
		{
			return (cyd_pulse(accumulator, pw) + ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) + cyd_lfsr(lfsr_acc)) / 3;
		}
		
		if(mixmode == 2)
		{
			return cyd_pulse(accumulator, pw) | ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) | cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 4)
		{
			return cyd_pulse(accumulator, pw) ^ ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) ^ cyd_lfsr(lfsr_acc);
		}
		break;
		
		case CYD_CHN_ENABLE_TRIANGLE|CYD_CHN_ENABLE_SAW|CYD_CHN_ENABLE_LFSR:
		if(mixmode == 0)
		{
			return cyd_triangle(accumulator) & cyd_saw(accumulator) & cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 1)
		{
			return (cyd_triangle(accumulator) + cyd_saw(accumulator) + cyd_lfsr(lfsr_acc)) / 3;
		}
		
		if(mixmode == 2)
		{
			return cyd_triangle(accumulator) | cyd_saw(accumulator) | cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 3)
		{
			return (TriSaw_8580[cyd_saw(accumulator) / 16]) & cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 4)
		{
			return cyd_triangle(accumulator) ^ cyd_saw(accumulator) ^ cyd_lfsr(lfsr_acc);
		}
		break;
		
		case CYD_CHN_ENABLE_NOISE|CYD_CHN_ENABLE_SAW|CYD_CHN_ENABLE_LFSR:
		if(mixmode == 0 || mixmode == 3)
		{
			return ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) & cyd_saw(accumulator) & cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 1)
		{
			return (((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) + cyd_saw(accumulator) + cyd_lfsr(lfsr_acc)) / 3;
		}
		
		if(mixmode == 2)
		{
			return ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) | cyd_saw(accumulator) | cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 4)
		{
			return ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) ^ cyd_saw(accumulator) ^ cyd_lfsr(lfsr_acc);
		}
		break;
		
		case CYD_CHN_ENABLE_NOISE|CYD_CHN_ENABLE_TRIANGLE|CYD_CHN_ENABLE_LFSR:
		if(mixmode == 0 || mixmode == 3)
		{
			return ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) & cyd_triangle(accumulator) & cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 1)
		{
			return (((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) + cyd_triangle(accumulator) + cyd_lfsr(lfsr_acc)) / 3;
		}
		
		if(mixmode == 2)
		{
			return ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) | cyd_triangle(accumulator) | cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 4)
		{
			return ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) ^ cyd_triangle(accumulator) ^ cyd_lfsr(lfsr_acc);
		}
		break;
		
		case CYD_CHN_ENABLE_TRIANGLE|CYD_CHN_ENABLE_PULSE|CYD_CHN_ENABLE_SAW|CYD_CHN_ENABLE_LFSR:
		if(mixmode == 0)
		{
			return cyd_pulse(accumulator, pw) & cyd_triangle(accumulator) & cyd_saw(accumulator) & cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 1)
		{
			return (cyd_pulse(accumulator, pw) + cyd_triangle(accumulator) + cyd_saw(accumulator) + cyd_lfsr(lfsr_acc)) / 4;
		}
		
		if(mixmode == 2)
		{
			return cyd_pulse(accumulator, pw) | cyd_triangle(accumulator) | cyd_saw(accumulator) | cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 3)
		{
			return (PulseTriSaw_8580[cyd_saw(accumulator) / 16]) & cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 4)
		{
			return cyd_pulse(accumulator, pw) ^ cyd_triangle(accumulator) ^ cyd_saw(accumulator) ^ cyd_lfsr(lfsr_acc);
		}
		break;
		
		case CYD_CHN_ENABLE_TRIANGLE|CYD_CHN_ENABLE_PULSE|CYD_CHN_ENABLE_NOISE|CYD_CHN_ENABLE_LFSR:
		if(mixmode == 0)
		{
			return cyd_pulse(accumulator, pw) & cyd_triangle(accumulator) & ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) & cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 1)
		{
			return (cyd_pulse(accumulator, pw) + cyd_triangle(accumulator) + ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) + cyd_lfsr(lfsr_acc)) / 4;
		}
		
		if(mixmode == 2)
		{
			return cyd_pulse(accumulator, pw) | cyd_triangle(accumulator) | ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) | cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 3)
		{
			return ((PulseTri_8580[cyd_saw(accumulator) / 8]) & cyd_pulse(accumulator, pw)) & cyd_lfsr(lfsr_acc) & ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random)));
		}
		
		if(mixmode == 4)
		{
			return cyd_pulse(accumulator, pw) ^ cyd_triangle(accumulator) ^ ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) ^ cyd_lfsr(lfsr_acc);
		}
		break;
		
		case CYD_CHN_ENABLE_TRIANGLE|CYD_CHN_ENABLE_NOISE|CYD_CHN_ENABLE_SAW|CYD_CHN_ENABLE_LFSR:
		if(mixmode == 0)
		{
			return cyd_saw(accumulator) & cyd_triangle(accumulator) & ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) & cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 1)
		{
			return (cyd_saw(accumulator) + cyd_triangle(accumulator) + ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) + cyd_lfsr(lfsr_acc)) / 4;
		}
		
		if(mixmode == 2)
		{
			return cyd_saw(accumulator) | cyd_triangle(accumulator) | ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) | cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 3)
		{
			return TriSaw_8580[cyd_saw(accumulator) / 16] & cyd_lfsr(lfsr_acc) & ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random)));
		}
		
		if(mixmode == 4)
		{
			return cyd_saw(accumulator) ^ cyd_triangle(accumulator) ^ ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) ^ cyd_lfsr(lfsr_acc);
		}
		break;
		
		case CYD_CHN_ENABLE_PULSE|CYD_CHN_ENABLE_NOISE|CYD_CHN_ENABLE_SAW|CYD_CHN_ENABLE_LFSR:
		if(mixmode == 0)
		{
			return cyd_pulse(accumulator, pw) & cyd_saw(accumulator) & ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) & cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 1)
		{
			return (cyd_pulse(accumulator, pw) + cyd_saw(accumulator) + ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) + cyd_lfsr(lfsr_acc)) / 4;
		}
		
		if(mixmode == 2)
		{
			return cyd_pulse(accumulator, pw) | cyd_saw(accumulator) | ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) | cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 3)
		{
			return (PulseSaw_8580[cyd_saw(accumulator) / 16]) & cyd_lfsr(lfsr_acc) & ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random)));
		}
		
		if(mixmode == 4)
		{
			return cyd_pulse(accumulator, pw) ^ cyd_saw(accumulator) ^ ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) ^ cyd_lfsr(lfsr_acc);
		}
		break;
		
		case CYD_CHN_ENABLE_NOISE|CYD_CHN_ENABLE_SAW|CYD_CHN_ENABLE_PULSE|CYD_CHN_ENABLE_TRIANGLE|CYD_CHN_ENABLE_LFSR:
		if(mixmode == 0)
		{
			return cyd_saw(accumulator) & cyd_pulse(accumulator, pw) & cyd_triangle(accumulator) & ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) & cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 1)
		{
			return (cyd_saw(accumulator) + cyd_pulse(accumulator, pw) + cyd_triangle(accumulator) + ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) + cyd_lfsr(lfsr_acc)) / 5;
		}
		
		if(mixmode == 2)
		{
			return cyd_saw(accumulator) | cyd_pulse(accumulator, pw) | cyd_triangle(accumulator) | ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) | cyd_lfsr(lfsr_acc);
		}
		
		if(mixmode == 3)
		{
			return (PulseTriSaw_8580[cyd_saw(accumulator) / 16]) & cyd_lfsr(lfsr_acc) & ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random)));
		}
		
		if(mixmode == 4)
		{
			return cyd_saw(accumulator) ^ cyd_pulse(accumulator, pw) ^ cyd_triangle(accumulator) ^ ((flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? ((cyd_noise(random) >= ((flags & CYD_CHN_ENABLE_METAL) ? ((WAVE_AMP - 1) / 4) : (WAVE_AMP - 1) / 2)) ? (WAVE_AMP - 1) : 0) : (cyd_noise(random))) ^ cyd_lfsr(lfsr_acc);
		}
		break;
#endif
		default:
		return WAVE_AMP / 2;
		break;
	}
}
