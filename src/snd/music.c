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

#define GENERATE_VIBRATO_TABLES

#include <time.h>

#include "music.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freqs.h"
#include "macros.h"
#include "pack.h"
#include "cydfm.h"

#include "cydfx.h" //wasn't there
#include "cyd.h"

#ifdef GENERATE_VIBRATO_TABLES

#include <math.h>

#endif

#define VIB_TAB_SIZE 128

#ifndef GENERATE_VIBRATO_TABLES

static const Sint8 rnd_table[VIB_TAB_SIZE] = {
	110, -1, 88, -31, 64,
	-13, 29, -70, -113, 71,
	99, -71, 74, 82, 52,
	-82, -58, 37, 20, -76,
	46, -97, -69, 41, 31,
	-62, -5, 99, -2, -48,
	-89, 17, -19, 4, -27,
	-43, -20, 25, 112, -34,
	78, 26, -56, -54, 72,
	-75, 22, 72, -119, 115,
	56, -66, 25, 87, 93,
	14, 82, 127, 79, -40,
	-100, 21, 17, 17, -116,
	-110, 61, -99, 105, 73,
	116, 53, -9, 105, 91,
	120, -73, 112, -10, 66,
	-10, -30, 99, -67, 60,
	84, 110, 87, -27, -46,
	114, 77, -27, -46, 75,
	-78, 83, -110, 92, -9,
	107, -64, 31, 77, -39,
	115, 126, -7, 121, -2,
	66, 116, -45, 91, 1,
	-96, -27, 17, 76, -82,
	58, -7, 75, -35, 49,
	3, -52, 40
};

static const Sint8 sine_table[VIB_TAB_SIZE] =
{
	0, 6, 12, 18, 24, 31, 37, 43, 48, 54, 60, 65, 71, 76, 81, 85, 90, 94, 98, 102, 106, 109, 112,
	115, 118, 120, 122, 124, 125, 126, 127, 127, 127, 127, 127, 126, 125, 124, 122, 120, 118, 115, 112,
	109, 106, 102, 98, 94, 90, 85, 81, 76, 71, 65, 60, 54, 48, 43, 37, 31, 24, 18, 12, 6,
	0, -6, -12, -18, -24, -31, -37, -43, -48, -54, -60, -65, -71, -76, -81, -85, -90, -94, -98, -102,
	-106, -109, -112, -115, -118, -120, -122, -124, -125, -126, -127, -127, -128, -127, -127, -126, -125, -124, -122,
	-120, -118, -115, -112, -109, -106, -102, -98, -94, -90, -85, -81, -76, -71, -65, -60, -54, -48, -43, -37, -31, -24, -18, -12, -6
};

#else

static Sint8 rnd_table[VIB_TAB_SIZE];
static Sint8 sine_table[VIB_TAB_SIZE];

#endif

static const Uint16 resonance_table[] = {10, 512, 1300, 1950};

static int mus_trigger_instrument_internal(MusEngine* mus, int chan, MusInstrument *ins, Uint16 note, int panning, bool update_adsr);

#ifndef USESDL_RWOPS

static int RWread(struct RWops *context, void *ptr, int size, int maxnum)
{
	return fread(ptr, size, maxnum, context->fp);
}


static int RWclose(struct RWops *context)
{
	if (context->close_fp) fclose(context->fp);
	free(context);
	return 1;
}


#define my_RWread(ctx, ptr, size, maxnum) ctx->read(ctx, ptr, size, maxnum)
#define my_RWclose(ctx) ctx->close(ctx)
#define my_RWtell(ctx) 0


#else

#include "SDL_rwops.h"

#define my_RWread SDL_RWread
#define my_RWclose SDL_RWclose
#define my_RWtell SDL_RWtell


#endif


static RWops * RWFromFP(FILE *f, int close)
{
#ifdef USESDL_RWOPS
	SDL_RWops *rw = SDL_RWFromFP(f, close);

	if (!rw)
	{
		warning("SDL_RWFromFP: %s", SDL_GetError());
	}

	return rw;
#else
	RWops *rw = calloc(sizeof(*rw), 1);

	rw->fp = f;
	rw->close_fp = close;
	rw->read = RWread;
	rw->close = RWclose;

	return rw;
#endif
}


static RWops * RWFromFile(const char *name, const char *mode)
{
#ifdef USESDL_RWOPS
	return SDL_RWFromFile(name, mode);
#else
	FILE *f = fopen(name, mode);

	if (!f) return NULL;

	return RWFromFP(f, 1);
#endif
}

void mus_free_inst_programs(MusInstrument* inst) //because memory for programs is dynamically allocated we need to free() it when we delete/overwrite instrument
{
	for(int i = 0; i < MUS_MAX_MACROS_INST; ++i)
	{
		if(inst->program[i])
		{
			free(inst->program[i]);
		}
		
		if(inst->program_unite_bits[i])
		{
			free(inst->program_unite_bits[i]);
		}
	}
	
	for(int op = 0; op < CYD_FM_NUM_OPS; ++op)
	{
		for(int i = 0; i < MUS_MAX_MACROS_OP; ++i)
		{
			if(inst->ops[op].program[i])
			{
				free(inst->ops[op].program[i]);
			}
			
			if(inst->ops[op].program_unite_bits[i])
			{
				free(inst->ops[op].program_unite_bits[i]);
			}
		}
	}
}

static void update_volumes(MusEngine *mus, MusTrackStatus *ts, MusChannel *chn, CydChannel *cydchn, int volume)
{
	if (chn->instrument && (chn->instrument->flags & MUS_INST_RELATIVE_VOLUME))
	{
		ts->volume = volume;
		cydchn->adsr.volume = (chn->flags & MUS_CHN_DISABLED) ? 0 : (int)chn->instrument->volume * volume / MAX_VOLUME * (int)mus->volume / MAX_VOLUME * (int)mus->play_volume / MAX_VOLUME * (int)chn->volume / MAX_VOLUME;
		
		if((cydchn->fm.flags & CYD_FM_ENABLE_ADDITIVE) && (cydchn->flags & CYD_CHN_ENABLE_FM))
		{
			cydchn->fm.adsr.volume = (chn->flags & MUS_CHN_DISABLED) ? 0 : (int)chn->instrument->fm_modulation * volume / MAX_VOLUME * (int)mus->volume / MAX_VOLUME * (int)mus->play_volume / MAX_VOLUME * (int)chn->volume / MAX_VOLUME;
		}
	}
	
	else
	{
		ts->volume = volume;
		cydchn->adsr.volume = (chn->flags & MUS_CHN_DISABLED) ? 0 : ts->volume * (int)mus->volume / MAX_VOLUME * (int)mus->play_volume / MAX_VOLUME * (int)chn->volume / MAX_VOLUME;
		
		if((cydchn->fm.flags & CYD_FM_ENABLE_ADDITIVE) && (cydchn->flags & CYD_CHN_ENABLE_FM))
		{
			cydchn->fm.adsr.volume = (chn->flags & MUS_CHN_DISABLED) ? 0 : ts->fm_volume * (int)mus->volume / MAX_VOLUME * (int)mus->play_volume / MAX_VOLUME * (int)mus->play_volume / MAX_VOLUME * (int)chn->volume / MAX_VOLUME;
			//debug("2, vol %d", cydchn->fm.adsr.volume);
		}
	}
}

static void update_fm_op_volume(MusEngine *mus, MusTrackStatus *track, MusChannel *chn, CydChannel *cydchn, int volume, Uint8 i /*number of operator*/)
{
	if (chn->instrument && (chn->instrument->ops[i].flags & MUS_FM_OP_RELATIVE_VOLUME))
	{
		track->ops_status[i].volume = volume;
		
		cydchn->fm.ops[i].adsr.volume = (chn->flags & MUS_CHN_DISABLED) ? 0 : ((int)chn->instrument->ops[i].volume * volume / MAX_VOLUME /* * (int)mus->volume / MAX_VOLUME */ * (int)mus->play_volume / MAX_VOLUME * (int)chn->volume / MAX_VOLUME);
	}
	
	else
	{
		track->ops_status[i].volume = volume;
		
		cydchn->fm.ops[i].adsr.volume = (chn->flags & MUS_CHN_DISABLED) ? 0 : (track->ops_status[i].volume /* * (int)mus->volume / MAX_VOLUME */ * (int)mus->play_volume / MAX_VOLUME * (int)chn->volume / MAX_VOLUME);
	}
}

static void update_all_volumes(MusEngine *mus)
{
	for (int i = 0; i < MUS_MAX_CHANNELS && i < mus->cyd->n_channels; ++i)
	{
		update_volumes(mus, &mus->song_track[i], &mus->channel[i], &mus->cyd->channel[i], mus->song_track[i].volume);
	}
}


static void mus_set_buzz_frequency(MusEngine *mus, int chan, Uint16 note)
{
#ifndef CYD_DISABLE_BUZZ
	MusChannel *chn = &mus->channel[chan];
	if (chn->instrument && chn->instrument->flags & MUS_INST_YM_BUZZ)
	{
#ifndef CYD_DISABLE_INACCURACY
		Uint32 buzz_frequency = get_freq(note + chn->buzz_offset) & mus->pitch_mask;
#else
		Uint32 buzz_frequency = get_freq(note + chn->buzz_offset);
#endif
		cyd_set_env_frequency(mus->cyd, &mus->cyd->channel[chan], buzz_frequency);
	}
#endif
}


static void mus_set_wavetable_frequency(MusEngine *mus, int chan, Uint16 note)
{
#ifndef CYD_DISABLE_WAVETABLE
	MusChannel *chn = &mus->channel[chan];
	CydChannel *cydchn = &mus->cyd->channel[chan];
	MusTrackStatus *track_status = &mus->song_track[chan];

	if (chn->instrument && (chn->instrument->cydflags & CYD_CHN_ENABLE_WAVE) && (cydchn->wave_entry))
	{
		for (int s = 0; s < CYD_SUB_OSCS; ++s)
		{
			Uint16 final = 0;

			if (s == 0 || (chn->instrument->flags & MUS_INST_MULTIOSC))
			{
				switch (s)
				{
					default:
					case 0:
						final = note;
						break;

					case 1:
						if (track_status->extarp1 != 0)
							final = note + ((Uint16)track_status->extarp1 << 8);
						else
							final = 0;
						break;

					case 2:
						if (track_status->extarp2 != 0)
							final = note + ((Uint16)track_status->extarp2 << 8);
						else
							final = 0;
						break;
				}
			}

			Uint32 wave_frequency = 0;

			if (final != 0)
			{
#ifndef CYD_DISABLE_INACCURACY
				wave_frequency = get_freq((chn->instrument->flags & MUS_INST_WAVE_LOCK_NOTE) ? cydchn->wave_entry->base_note : (final)) & mus->pitch_mask;
#else
				wave_frequency = get_freq((chn->instrument->flags & MUS_INST_WAVE_LOCK_NOTE) ? cydchn->wave_entry->base_note : (final));
#endif
			}

			cyd_set_wavetable_frequency(mus->cyd, cydchn, s, wave_frequency);
		}
	}
#endif
}


static void mus_set_frequency(MusEngine *mus, int chan, Uint16 note, int divider)
{
	MusChannel *chn = &mus->channel[chan];
	MusTrackStatus *track_status = &mus->song_track[chan];

	for (int s = 0; s < CYD_SUB_OSCS; ++s)
	{
		Uint16 final = 0;

		if (s == 0 || (chn->instrument->flags & MUS_INST_MULTIOSC))
		{
			switch (s)
			{
				default:
				case 0:
					final = note;
					break;

				case 1:
					if (track_status->extarp1 != 0)
						final = note + ((Uint16)track_status->extarp1 << 8);
					else
						final = 0;
					break;

				case 2:
					if (track_status->extarp2 != 0)
						final = note + ((Uint16)track_status->extarp2 << 8);
					else
						final = 0;
					break;
			}
		}

		Uint32 frequency = 0;

		if (final != 0 || (note == 0 && s == 0))
		{
#ifndef CYD_DISABLE_INACCURACY
			frequency = get_freq(final) & mus->pitch_mask;
#else
			frequency = get_freq(final);
#endif
		}

		cyd_set_frequency(mus->cyd, &mus->cyd->channel[chan], s, frequency / divider);
		
		//debug("freq %d Hz, note %d", frequency / 16, note);
		
		if(s == 0)
		{
			mus->cyd->channel[chan].freq_for_ksl = final;
			mus->cyd->channel[chan].fm.freq_for_fm_ksl = (final + ((mus->cyd->channel[chan].fm.fm_base_note - mus->cyd->channel[chan].fm.fm_carrier_base_note) << 8) + mus->cyd->channel[chan].fm.fm_carrier_finetune + mus->cyd->channel[chan].fm.fm_finetune + mus->cyd->channel[chan].fm.fm_vib); //* (Uint64)harmonic[mus->cyd->channel[chan].fm.harmonic & 15] / (Uint64)harmonic[mus->cyd->channel[chan].fm.harmonic >> 4];
		}
	}
}


static void mus_set_note(MusEngine *mus, int chan, Uint16 note, int update_note, int divider)
{
	MusChannel *chn = &mus->channel[chan];

	if (update_note) chn->note = note;

	mus_set_frequency(mus, chan, note, divider);

	mus_set_wavetable_frequency(mus, chan, note);

	mus_set_buzz_frequency(mus, chan, note);
}

#define MUL 2

static Sint32 harmonic1[16] = { (Sint32)(0.5 * MUL), 1 * MUL, 2 * MUL, 3 * MUL, 4 * MUL, 5 * MUL, 6 * MUL, 7 * MUL, 8 * MUL, 9 * MUL, 10 * MUL, 10 * MUL, 12 * MUL, 12 * MUL, 15 * MUL, 15 * MUL };
static Sint32 harmonicOPN1[16] = { (Sint32)(0.5 * MUL), 1 * MUL, 2 * MUL, 3 * MUL, 4 * MUL, 5 * MUL, 6 * MUL, 7 * MUL, 8 * MUL, 9 * MUL, 10 * MUL, 11 * MUL, 12 * MUL, 13 * MUL, 14 * MUL, 15 * MUL };

void mus_set_fm_op_note(MusEngine* mus, int chan, CydFm* fm, Uint32 note, int i /*op number*/, bool update_note, int divider, MusInstrument *ins)
{
	MusChannel *chn = &mus->channel[chan];
	MusTrackStatus *track_status = &mus->song_track[chan];
	
	if (update_note) chn->ops[i].note = note;
	
	for (int s = 0; s < CYD_SUB_OSCS; ++s)
	{
		Uint32 frequency = 0;
		
		Uint32 final = 0;
		
		bool zero_miltiosc_freq = false;
		
		if(chn->instrument)
		{
			if (s == 0 || (chn->instrument->flags & MUS_INST_MULTIOSC))
			{
				switch (s)
				{
					default:
					case 0:
						final = note;
						break;

					case 1:
						if (track_status->extarp1 != 0)
							final = note + ((Uint16)track_status->extarp1 << 8);
						else
						{
							final = 0;
							zero_miltiosc_freq = true;
						}
						break;

					case 2:
						if (track_status->extarp2 != 0)
							final = note + ((Uint16)track_status->extarp2 << 8);
						else
						{
							final = 0;
							zero_miltiosc_freq = true;
						}
						break;
				}
				
				frequency = get_freq(final);
				
				if(s == 0)
				{
					mus->cyd->channel[chan].fm.ops[i].freq_for_ksl = final;
					
					CydChannel* cydchn = &mus->cyd->channel[chan];
					
					cydchn->fm.ops[i].vol_ksl_mult = cydchn->fm.ops[i].env_ksl_mult = 1.0;
		
					if((cydchn->fm.ops[i].flags & CYD_FM_OP_ENABLE_VOLUME_KEY_SCALING) || (cydchn->fm.ops[i].flags & CYD_FM_OP_ENABLE_ENVELOPE_KEY_SCALING))
					{
						Sint16 vol_ksl_level_final = (cydchn->fm.ops[i].flags & CYD_FM_OP_ENABLE_VOLUME_KEY_SCALING) ? cydchn->fm.ops[i].vol_ksl_level : -1;
						
						if(cydchn->fm.flags & CYD_FM_ENABLE_3CH_EXP_MODE)
						{
							cydchn->fm.ops[i].vol_ksl_mult = (vol_ksl_level_final == -1) ? 1.0 : (pow(((Uint64)get_freq((cydchn->fm.ops[i].base_note << 8) + cydchn->fm.ops[i].finetune + cydchn->fm.ops[i].detune * DETUNE + coarse_detune_table[cydchn->fm.ops[i].coarse_detune]) / (get_freq(cydchn->fm.ops[i].freq_for_ksl) + 1.0)), (vol_ksl_level_final == 0 ? 0 : (vol_ksl_level_final / 127.0))));
						}
						
						else
						{
							cydchn->fm.ops[i].vol_ksl_mult = (vol_ksl_level_final == -1) ? 1.0 : (pow(((Uint64)get_freq(((cydchn->base_note << 8) + cydchn->finetune + cydchn->fm.ops[i].detune * DETUNE + coarse_detune_table[cydchn->fm.ops[i].coarse_detune])) / (get_freq(cydchn->fm.ops[i].freq_for_ksl) + 1.0)), (vol_ksl_level_final == 0 ? 0 : (vol_ksl_level_final / 127.0))));
						}
						//cydchn->fm.ops[i].vol_ksl_mult = (vol_ksl_level_final == -1) ? 1.0 : (pow((get_freq((cydchn->fm.ops[i].base_note << 8) + cydchn->fm.ops[i].finetune + cydchn->fm.ops[i].detune * 8 + cydchn->fm.ops[i].coarse_detune * 128) + 1.0) / (get_freq(cydchn->fm.ops[i].freq_for_ksl) + 1.0), (vol_ksl_level_final == 0 ? 0 : (vol_ksl_level_final / 127.0))));
						
						Sint16 env_ksl_level_final = (cydchn->fm.ops[i].flags & CYD_FM_OP_ENABLE_ENVELOPE_KEY_SCALING) ? cydchn->fm.ops[i].env_ksl_level : -1;
						
						if(cydchn->fm.flags & CYD_FM_ENABLE_3CH_EXP_MODE)
						{
							cydchn->fm.ops[i].env_ksl_mult = (env_ksl_level_final == -1) ? 1.0 : (pow(((Uint64)get_freq((cydchn->fm.ops[i].base_note << 8) + cydchn->fm.ops[i].finetune + cydchn->fm.ops[i].detune * DETUNE + coarse_detune_table[cydchn->fm.ops[i].coarse_detune]) / (get_freq(cydchn->fm.ops[i].freq_for_ksl) + 1.0)), (env_ksl_level_final == 0 ? 0 : (env_ksl_level_final / 127.0))));
						}
						
						else
						{
							cydchn->fm.ops[i].env_ksl_mult = (env_ksl_level_final == -1) ? 1.0 : (pow(((Uint64)get_freq(((cydchn->base_note << 8) + cydchn->finetune + cydchn->fm.ops[i].detune * DETUNE + coarse_detune_table[cydchn->fm.ops[i].coarse_detune] * harmonic1[cydchn->fm.ops[i].harmonic & 15] / harmonic1[cydchn->fm.ops[i].harmonic >> 4])) / (get_freq(cydchn->fm.ops[i].freq_for_ksl) + 1.0)), (env_ksl_level_final == 0 ? 0 : (env_ksl_level_final / 127.0))));
						}
						//cydchn->fm.ops[i].env_ksl_mult = (env_ksl_level_final == -1) ? 1.0 : (pow((get_freq((cydchn->fm.ops[i].base_note << 8) + cydchn->fm.ops[i].finetune + cydchn->fm.ops[i].detune * 8 + cydchn->fm.ops[i].coarse_detune * 128) + 1.0) / (get_freq(cydchn->fm.ops[i].freq_for_ksl) + 1.0), (env_ksl_level_final == 0 ? 0 : (env_ksl_level_final / 127.0))));
						cydchn->fm.ops[i].env_ksl_mult = 1.0 / cydchn->fm.ops[i].env_ksl_mult;
					}
				}
			}
		}
		
		if (frequency != 0 && zero_miltiosc_freq == false)
		{
			if(fm->flags & CYD_FM_ENABLE_3CH_EXP_MODE)
			{
				//chn->subosc[subosc].frequency = (Uint64)(ACC_LENGTH >> (cyd->oversample)) / 64 * (Uint64)(frequency) / (Uint64)cyd->sample_rate;
				fm->ops[i].subosc[s].frequency = (Uint64)(ACC_LENGTH >> (mus->cyd->oversample)) / (Uint64)1024 * (Uint64)frequency / (Uint64)mus->cyd->sample_rate / (((ins->fm_flags & CYD_FM_ENABLE_3CH_EXP_MODE) ? (ins->ops[i].flags & MUS_FM_OP_QUARTER_FREQ) : (ins->flags & MUS_INST_QUARTER_FREQ)) ? 4 : 1);
				//fm->ops[i].osc.frequency = (Uint64)(ACC_LENGTH >> (mus->cyd->oversample)) / 64 * (Uint64)frequency / (Uint64)mus->cyd->sample_rate;
				//fm->ops[i].scale_freq = (Uint64)(ACC_LENGTH >> (mus->cyd->oversample)) / 64 * (Uint64)(get_freq((Uint32)(((Uint16)ins->ops[i].base_note) << 8) + fm->ops[i].finetune)) / (Uint64)mus->cyd->sample_rate;
			}
			
			else
			{
				if(fm->fm_freq_LUT == 0)
				{
					fm->ops[i].subosc[s].frequency = (Uint64)(ACC_LENGTH >> (mus->cyd->oversample)) / (Uint64)1024 * (Uint64)(frequency) / (Uint64)mus->cyd->sample_rate * (Uint64)harmonic1[fm->ops[i].harmonic & 15] / (Uint64)harmonic1[fm->ops[i].harmonic >> 4] / (((ins->fm_flags & CYD_FM_ENABLE_3CH_EXP_MODE) ? (ins->ops[i].flags & MUS_FM_OP_QUARTER_FREQ) : (ins->flags & MUS_INST_QUARTER_FREQ)) ? 4 : 1);
					
					//fm->ops[i].osc.frequency = (Uint64)(ACC_LENGTH >> (mus->cyd->oversample)) / 64 * (Uint64)(frequency) / (Uint64)mus->cyd->sample_rate * (Uint64)harmonic1[fm->ops[i].harmonic & 15] / (Uint64)harmonic1[fm->ops[i].harmonic >> 4];
					
					//fm->ops[i].scale_freq = (Uint64)(ACC_LENGTH >> (mus->cyd->oversample)) / 64 * (Uint64)(get_freq((Uint32)(((Uint16)ins->base_note) << 8) + fm->ops[i].finetune)) / (Uint64)mus->cyd->sample_rate * (Uint64)harmonic1[fm->ops[i].harmonic & 15] / (Uint64)harmonic1[fm->ops[i].harmonic >> 4];
				}
				
				else
				{
					fm->ops[i].subosc[s].frequency = (Uint64)(ACC_LENGTH >> (mus->cyd->oversample)) / (Uint64)1024 * (Uint64)(frequency) / (Uint64)mus->cyd->sample_rate * (Uint64)harmonicOPN1[fm->ops[i].harmonic & 15] / (Uint64)harmonicOPN1[fm->ops[i].harmonic >> 4] / (((ins->fm_flags & CYD_FM_ENABLE_3CH_EXP_MODE) ? (ins->ops[i].flags & MUS_FM_OP_QUARTER_FREQ) : (ins->flags & MUS_INST_QUARTER_FREQ)) ? 4 : 1);
					
					//fm->ops[i].osc.frequency = (Uint64)(ACC_LENGTH >> (mus->cyd->oversample)) / 64 * (Uint64)(frequency) / (Uint64)mus->cyd->sample_rate * (Uint64)harmonicOPN1[fm->ops[i].harmonic & 15] / (Uint64)harmonicOPN1[fm->ops[i].harmonic >> 4];
					//fm->ops[i].scale_freq = (Uint64)(ACC_LENGTH >> (mus->cyd->oversample)) / 64 * (Uint64)(get_freq((Uint32)(((Uint16)ins->base_note) << 8) + fm->ops[i].finetune)) / (Uint64)mus->cyd->sample_rate * (Uint64)harmonicOPN1[fm->ops[i].harmonic & 15] / (Uint64)harmonicOPN1[fm->ops[i].harmonic >> 4];
				}	
			}
		}
		
		else
		{
			fm->ops[i].subosc[s].frequency = 0;
			fm->ops[i].subosc[s].wave.frequency = 0;
		}
		
		//fm->ops[i].osc.wave.frequency = fm->ops[i].osc.frequency / 4;
		
		if (fm->ops[i].flags & CYD_FM_OP_ENABLE_WAVE)
		{
			Uint32 wave_frequency = get_freq((ins->ops[i].flags & MUS_FM_OP_WAVE_LOCK_NOTE) ? fm->ops[i].wave_entry->base_note : final);
			
			if(frequency != 0 && wave_frequency != 0 && zero_miltiosc_freq == false)
			{
				if(fm->fm_freq_LUT == 0)
				{
					//debug("test");
					fm->ops[i].subosc[s].wave.frequency = (Uint64)WAVETABLE_RESOLUTION * (Uint64)fm->ops[i].wave_entry->sample_rate / (Uint64)mus->cyd->sample_rate * (Uint64)wave_frequency / (Uint64)get_freq(fm->ops[i].wave_entry->base_note) * (Uint64)harmonic1[fm->ops[i].harmonic & 15] / (Uint64)harmonic1[fm->ops[i].harmonic >> 4] / (((ins->fm_flags & CYD_FM_ENABLE_3CH_EXP_MODE) ? (ins->ops[i].flags & MUS_FM_OP_QUARTER_FREQ) : (ins->flags & MUS_INST_QUARTER_FREQ)) ? 4 : 1);
				}
				
				else
				{
					fm->ops[i].subosc[s].wave.frequency = (Uint64)WAVETABLE_RESOLUTION * (Uint64)fm->ops[i].wave_entry->sample_rate / (Uint64)mus->cyd->sample_rate * (Uint64)wave_frequency / (Uint64)get_freq(fm->ops[i].wave_entry->base_note) * (Uint64)harmonicOPN1[fm->ops[i].harmonic & 15] / (Uint64)harmonicOPN1[fm->ops[i].harmonic >> 4] / (((ins->fm_flags & CYD_FM_ENABLE_3CH_EXP_MODE) ? (ins->ops[i].flags & MUS_FM_OP_QUARTER_FREQ) : (ins->flags & MUS_INST_QUARTER_FREQ)) ? 4 : 1);
				}
			}
			
			else
			{
				fm->ops[i].subosc[s].wave.frequency = 0;
			}
		}
		
		fm->ops[i].true_freq = frequency;
	}
}

static void mus_set_noise_fixed_pitch_note(MusEngine *mus, int chan, Uint16 note, int update_note, int divider)
{
	CydChannel *chn = &mus->cyd->channel[chan];

	Uint32 frequency = (get_freq(note) & mus->pitch_mask) / divider;
	
	for(int i = 0; i < CYD_SUB_OSCS; ++i)
	{
		if (frequency != 0)
		{
			chn->subosc[i].noise_frequency = (Uint64)(ACC_LENGTH >> (mus->cyd->oversample)) / (Uint64)1024 * (Uint64)(frequency) / (Uint64)mus->cyd->sample_rate;
		}
	}
}

static void mus_set_slide(MusEngine *mus, int chan, Uint32 note)
{
	MusChannel *chn = &mus->channel[chan];
	chn->target_note = note;
	//if (update_note) chn->note = note;
}


void mus_init_engine(MusEngine *mus, CydEngine *cyd)
{
	memset(mus, 0, sizeof(*mus));
	mus->cyd = cyd;
	mus->volume = MAX_VOLUME;
	mus->play_volume = MAX_VOLUME;

	for (int i = 0; i < cyd->n_channels; ++i)
		mus->channel[i].volume = MAX_VOLUME;

#ifndef CYD_DISABLE_INACCURACY
	mus->pitch_mask = ~0;
#endif

#ifdef GENERATE_VIBRATO_TABLES
	for (int i = 0; i < VIB_TAB_SIZE; ++i)
	{
		sine_table[i] = sin((float)i / VIB_TAB_SIZE * M_PI * 2) * 127;
		rnd_table[i] = rand();
	}
#endif
}

static void do_command(MusEngine *mus, int chan, int tick, Uint16 inst, int from_program, int ops_index, int prog_number) //ops_index 0 - do for main instrument and fm mod. only, 1-FE - do for op (ops_index - 1), FF - do for all ops and main instrument
{
	MusChannel *chn = &mus->channel[chan];
	CydChannel *cydchn = &mus->cyd->channel[chan];
	CydEngine *cyd = mus->cyd;
	MusTrackStatus *track_status = &mus->song_track[chan];
	
	switch (inst & 0xfff0)
	{
		case MUS_FX_PHASE_RESET:
		{
			if ((inst & 0xf) > 0 && (tick % (inst & 0xf)) == 0)
			{
				if(ops_index == 0 || ops_index == 0xFF)
				{
					for(int s = 0; s < CYD_SUB_OSCS; ++s)
					{
						cydchn->subosc[s].accumulator = 0;
					}
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							for(int s = 0; s < CYD_SUB_OSCS; ++s)
							{
								cydchn->fm.ops[i].subosc[s].accumulator = 0;
							}
						}
					}
				}
				
				else
				{
					for(int s = 0; s < CYD_SUB_OSCS; ++s)
					{
						cydchn->fm.ops[ops_index - 1].subosc[s].accumulator = 0;
					}
				}
			}
			
			break;
		}
		
		case MUS_FX_NOISE_PHASE_RESET:
		{
			if ((inst & 0xf) > 0 && (tick % (inst & 0xf)) == 0)
			{
				if(ops_index == 0 || ops_index == 0xFF)
				{
					for(int s = 0; s < CYD_SUB_OSCS; ++s)
					{
						cydchn->subosc[s].noise_accumulator = 0;
					}
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							for(int s = 0; s < CYD_SUB_OSCS; ++s)
							{
								cydchn->fm.ops[i].subosc[s].noise_accumulator = 0;
							}
						}
					}
				}
				
				else
				{
					for(int s = 0; s < CYD_SUB_OSCS; ++s)
					{
						cydchn->fm.ops[ops_index - 1].subosc[s].noise_accumulator = 0;
					}
				}
			}
			
			break;
		}
		
		case MUS_FX_WAVE_PHASE_RESET:
		{
			if ((inst & 0xf) > 0 && (tick % (inst & 0xf)) == 0)
			{
				if(ops_index == 0 || ops_index == 0xFF)
				{
					for(int s = 0; s < CYD_SUB_OSCS; ++s)
					{
						cydchn->subosc[s].wave.acc = 0;
					}
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							for(int s = 0; s < CYD_SUB_OSCS; ++s)
							{
								cydchn->fm.ops[i].subosc[s].wave.acc = 0;
							}
						}
					}
				}
				
				else
				{
					for(int s = 0; s < CYD_SUB_OSCS; ++s)
					{
						cydchn->fm.ops[ops_index - 1].subosc[s].wave.acc = 0;
					}
				}
			}
			
			break;
		}
		
		case MUS_FX_FM_PHASE_RESET:
		{
			if ((inst & 0xf) > 0 && (tick % (inst & 0xf)) == 0)
			{
				if(ops_index == 0 || ops_index == 0xFF)
				{
					cydchn->fm.accumulator = 0;
					cydchn->fm.wave.acc = 0;
				}
			}
			
			break;
		}
		
		case MUS_FX_FM_4OP_MASTER_FADE_VOLUME_UP:
		{
			if(track_status->fm_4op_vol + (inst & 0xf) > 0xff)
			{
				track_status->fm_4op_vol = 0xff;
			}
			
			else
			{
				track_status->fm_4op_vol += (inst & 0xf);
			}
		}
		break;
		
		case MUS_FX_FM_4OP_MASTER_FADE_VOLUME_DOWN:
		{
			if((int)track_status->fm_4op_vol - (int)(inst & 0xf) < 0)
			{
				track_status->fm_4op_vol = 0;
			}
			
			else
			{
				track_status->fm_4op_vol -= (inst & 0xf);
			}
		}
		break;
		
		case MUS_FX_FM_WAVETABLE_OFFSET_UP_FINE:
		{
			if(ops_index == 0 || ops_index == 0xFF)
			{
				if (cydchn->fm.wave_entry)
				{
					//if(cydchn->fm.wave_entry->flags & CYD_WAVE_LOOP)
					if ((cydchn->fm.wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->fm.wave.start_offset + (cydchn->fm.wave.start_offset + (inst & 0xf) > 0xffff ? 0xffff : (inst & 0xf)) < cydchn->fm.wave.end_offset))
					{
						cydchn->fm.wave.start_offset += (cydchn->fm.wave.start_offset + (inst & 0xf) > 0xffff ? 0xffff : (inst & 0xf));
						
						cydchn->fm.wave.start_point_track_status = (Uint64)cydchn->fm.wave.start_offset * WAVETABLE_RESOLUTION * cydchn->fm.wave_entry->samples / 0x10000;
						
						cydchn->fm.wave.use_start_track_status_offset = true;
					}
				}
			}
		}
		break;
		
		case MUS_FX_FM_WAVETABLE_OFFSET_DOWN_FINE:
		{
			if(ops_index == 0 || ops_index == 0xFF)
			{
				if (cydchn->fm.wave_entry)
				{
					//if(cydchn->fm.wave_entry->flags & CYD_WAVE_LOOP)
					if ((cydchn->fm.wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->fm.wave.start_offset - ((Sint32)cydchn->fm.wave.start_offset - (inst & 0xf) < 0 ? 0 : (inst & 0xf)) < cydchn->fm.wave.end_offset))
					{
						cydchn->fm.wave.start_offset -= ((Sint32)cydchn->fm.wave.start_offset - (inst & 0xf) < 0 ? 0 : (inst & 0xf));
						
						cydchn->fm.wave.start_point_track_status = (Uint64)cydchn->fm.wave.start_offset * WAVETABLE_RESOLUTION * cydchn->fm.wave_entry->samples / 0x10000;
						
						cydchn->fm.wave.use_start_track_status_offset = true;
					}
				}
			}
		}
		break;
		
		case MUS_FX_FM_WAVETABLE_END_POINT_UP_FINE:
		{
			if(ops_index == 0 || ops_index == 0xFF)
			{
				if (cydchn->fm.wave_entry)
				{
					//if(cydchn->fm.wave_entry->flags & CYD_WAVE_LOOP)
					if ((cydchn->fm.wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->fm.wave.end_offset - ((Sint32)cydchn->fm.wave.end_offset - (inst & 0xf) < 0 ? 0 : (inst & 0xf)) > cydchn->fm.wave.start_offset))
					{
						cydchn->fm.wave.end_offset -= ((Sint32)cydchn->fm.wave.end_offset - (inst & 0xf) < 0 ? 0 : (inst & 0xf));
						
						cydchn->fm.wave.end_point_track_status = (Uint64)cydchn->fm.wave.end_offset * WAVETABLE_RESOLUTION * cydchn->fm.wave_entry->samples / 0x10000;
						
						cydchn->fm.wave.use_end_track_status_offset = true;
					}
				}
			}
		}
		break;
		
		case MUS_FX_FM_WAVETABLE_END_POINT_DOWN_FINE:
		{
			if(ops_index == 0 || ops_index == 0xFF)
			{
				if (cydchn->fm.wave_entry)
				{
					//if(cydchn->fm.wave_entry->flags & CYD_WAVE_LOOP)
					if ((cydchn->fm.wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->fm.wave.end_offset + (cydchn->fm.wave.end_offset + (inst & 0xf) > 0xffff ? 0xffff : (inst & 0xf)) > cydchn->fm.wave.start_offset))
					{
						cydchn->fm.wave.end_offset += (cydchn->fm.wave.end_offset + (inst & 0xf) > 0xffff ? 0xffff : (inst & 0xf));
						
						cydchn->fm.wave.end_point_track_status = (Uint64)cydchn->fm.wave.end_offset * WAVETABLE_RESOLUTION * cydchn->fm.wave_entry->samples / 0x10000;
						
						cydchn->fm.wave.use_end_track_status_offset = true;
					}
				}
			}
		}
		break;
		
		case MUS_FX_FM_SET_OP1_DETUNE:
		case MUS_FX_FM_SET_OP2_DETUNE:
		case MUS_FX_FM_SET_OP3_DETUNE:
		case MUS_FX_FM_SET_OP4_DETUNE:
		{
			if(ops_index == 0xFF || ops_index == 0)
			{
				Uint8 op = (((inst & 0xF000) - (MUS_FX_FM_SET_OP1_DETUNE & 0xF000)) >> 12);
				
				Sint8 temp_detune = cydchn->fm.ops[op].detune;
				
				cydchn->fm.ops[op].detune = my_min(7, my_max((inst & 0xF) - 7, -7));
				
				mus->channel[chan].ops[op].note = mus->channel[chan].ops[op].note - temp_detune * DETUNE + cydchn->fm.ops[op].detune * DETUNE;
			}
		}
		break;
		
		case MUS_FX_FM_SET_OP1_COARSE_DETUNE:
		case MUS_FX_FM_SET_OP2_COARSE_DETUNE:
		case MUS_FX_FM_SET_OP3_COARSE_DETUNE:
		case MUS_FX_FM_SET_OP4_COARSE_DETUNE:
		{
			if(ops_index == 0xFF || ops_index == 0)
			{
				Uint8 op = (((inst & 0xF000) - (MUS_FX_FM_SET_OP1_DETUNE & 0xF000)) >> 12);
				
				Uint8 temp_detune = cydchn->fm.ops[op].coarse_detune;
				
				cydchn->fm.ops[op].coarse_detune = (inst & 3);
				
				mus->channel[chan].ops[op].note = mus->channel[chan].ops[op].note - coarse_detune_table[temp_detune] + coarse_detune_table[cydchn->fm.ops[op].coarse_detune];
			}
		}
		break;
		
		case MUS_FX_FM_SET_OP1_FEEDBACK:
		case MUS_FX_FM_SET_OP2_FEEDBACK:
		case MUS_FX_FM_SET_OP3_FEEDBACK:
		case MUS_FX_FM_SET_OP4_FEEDBACK:
		{
			if(ops_index == 0xFF || ops_index == 0)
			{
				Uint8 op = (((inst & 0xF000) - (MUS_FX_FM_SET_OP1_FEEDBACK & 0xF000)) >> 12);
				
				cydchn->fm.ops[op].feedback = (inst & 15);
			}
		}
		break;
		
		case MUS_FX_FM_SET_OP1_SSG_EG_TYPE:
		case MUS_FX_FM_SET_OP2_SSG_EG_TYPE:
		case MUS_FX_FM_SET_OP3_SSG_EG_TYPE:
		case MUS_FX_FM_SET_OP4_SSG_EG_TYPE:
		{
			if(ops_index == 0xFF || ops_index == 0)
			{
				Uint8 op = (((inst & 0xF000) - (MUS_FX_FM_SET_OP1_SSG_EG_TYPE & 0xF000)) >> 12);
				
				cydchn->fm.ops[op].ssg_eg_type = (inst & 7);
				
				cydchn->fm.ops[op].flags &= ~(CYD_FM_OP_ENABLE_SSG_EG);
				
				if(inst & 8) cydchn->fm.ops[op].flags |= CYD_FM_OP_ENABLE_SSG_EG;
			}
		}
		break;
		
		case MUS_FX_SET_EXPONENTIALS:
		{
			switch(ops_index)
			{
				case 0:
				case 0xFF:
				{
					cydchn->flags &= ~(0xf << 24);
					cydchn->flags |= ((inst & 0xf) << 24);
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							cydchn->fm.ops[i].flags &= ~(0xf << 24);
							cydchn->fm.ops[i].flags |= ((inst & 0xf) << 24);
						}
					}
					
					break;
				}
				
				default:
				{
					cydchn->fm.ops[ops_index - 1].flags &= ~(0xf << 24);
					cydchn->fm.ops[ops_index - 1].flags |= ((inst & 0xf) << 24);
					
					break;
				}
			}
		}
		break;
		
		case MUS_FX_FM_SET_EXPONENTIALS:
		{
			if(ops_index == 0 || ops_index == 0xFF)
			{
				cydchn->fm.flags &= ~(0xf << 24);
				cydchn->fm.flags |= ((inst & 0xf) << 24);
			}
		}
		break;
		
		case MUS_FX_FM_4OP_SET_DETUNE:
		{
			if(ops_index != 0 && ops_index != 0xFF)
			{
				Sint8 temp_detune = cydchn->fm.ops[ops_index - 1].detune;
				
				cydchn->fm.ops[ops_index - 1].detune = my_min(7, my_max((inst & 0xF) - 7, -7));
				
				mus->channel[chan].ops[ops_index - 1].note = mus->channel[chan].ops[ops_index - 1].note - temp_detune * DETUNE + cydchn->fm.ops[ops_index - 1].detune * DETUNE;
			}
		}
		break;
		
		case MUS_FX_FM_4OP_SET_COARSE_DETUNE:
		{
			if(ops_index != 0 && ops_index != 0xFF)
			{
				Uint8 temp_detune = cydchn->fm.ops[ops_index - 1].coarse_detune;
				
				cydchn->fm.ops[ops_index - 1].coarse_detune = (inst & 3);
				
				mus->channel[chan].ops[ops_index - 1].note = mus->channel[chan].ops[ops_index - 1].note - coarse_detune_table[temp_detune] + coarse_detune_table[cydchn->fm.ops[ops_index - 1].coarse_detune];
			}
		}
		break;
		
		case MUS_FX_FM_4OP_SET_SSG_EG_TYPE:
		{
			if(ops_index != 0 && ops_index != 0xFF)
			{
				cydchn->fm.ops[ops_index - 1].ssg_eg_type = (inst & 7);
				cydchn->fm.ops[ops_index - 1].flags &= ~(CYD_FM_OP_ENABLE_SSG_EG);
				
				if(inst & 8) cydchn->fm.ops[ops_index - 1].flags |= CYD_FM_OP_ENABLE_SSG_EG;
			}
		}
		break;
		
		case MUS_FX_FM_SET_FEEDBACK:
		{
			switch(ops_index)
			{
				case 0:
				case 0xFF:
				{
					cydchn->fm.feedback = inst & 7;
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							cydchn->fm.ops[i].feedback = inst & 15;
						}
					}
					
					break;
				}
				
				default:
				{
					cydchn->fm.ops[ops_index - 1].feedback = inst & 15;
					break;
				}
			}
		}
		break;
		
		case MUS_FX_FM_EXT_FADE_VOLUME_DN: //wasn't there
		{
			if(ops_index == 0 || ops_index == 0xFF)
			{
				if (!(chn->flags & MUS_CHN_DISABLED))
				{
					cydchn->fm.adsr.volume -= inst & 0xf;
					if (cydchn->fm.adsr.volume > MAX_VOLUME) cydchn->fm.adsr.volume = 0;
				}
			}
		}
		break;
		
		case MUS_FX_FM_EXT_FADE_VOLUME_UP: //wasn't there
		{
			if(ops_index == 0 || ops_index == 0xFF)
			{
				if (!(chn->flags & MUS_CHN_DISABLED))
				{
					cydchn->fm.adsr.volume += inst & 0xf;
					if (cydchn->fm.adsr.volume > MAX_VOLUME) cydchn->fm.adsr.volume = MAX_VOLUME;
				}
			}
		}
		break;
	}

	switch (inst & 0xff00)
	{
		case MUS_FX_CSM_TIMER_PORTA_UP:
		{
			if(ops_index == 0xFF || ops_index == 0)
			{
				if(ops_index == 0xFF)
				{
					for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
					{
						if(cydchn->fm.ops[i].flags & CYD_FM_OP_ENABLE_CSM_TIMER)
						{
							if((Uint32)chn->ops[i].CSM_timer_note + (inst & 0xff) <= (FREQ_TAB_SIZE << 8))
							{
								chn->ops[i].CSM_timer_note += (inst & 0xff);
							}
							
							else
							{
								chn->ops[i].CSM_timer_note = (FREQ_TAB_SIZE << 8);
							}
						}
					}
				}
			}
			
			else
			{
				if(cydchn->fm.ops[ops_index - 1].flags & CYD_FM_OP_ENABLE_CSM_TIMER)
				{
					if((Uint32)chn->ops[ops_index - 1].CSM_timer_note + (inst & 0xff) <= (FREQ_TAB_SIZE << 8))
					{
						chn->ops[ops_index - 1].CSM_timer_note += (inst & 0xff);
					}
					
					else
					{
						chn->ops[ops_index - 1].CSM_timer_note = (FREQ_TAB_SIZE << 8);
					}
				}
			}
		}
		break;
		
		case MUS_FX_CSM_TIMER_PORTA_DN:
		{
			if(ops_index == 0xFF || ops_index == 0)
			{
				if(ops_index == 0xFF)
				{
					for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
					{
						if(cydchn->fm.ops[i].flags & CYD_FM_OP_ENABLE_CSM_TIMER)
						{
							if((Sint32)chn->ops[i].CSM_timer_note - (inst & 0xff) > 0)
							{
								chn->ops[i].CSM_timer_note -= (inst & 0xff);
							}
							
							else
							{
								chn->ops[i].CSM_timer_note = 0;
							}
						}
					}
				}
			}
			
			else
			{
				if(cydchn->fm.ops[ops_index - 1].flags & CYD_FM_OP_ENABLE_CSM_TIMER)
				{
					if((Sint32)chn->ops[ops_index - 1].CSM_timer_note - (inst & 0xff) > 0)
					{
						chn->ops[ops_index - 1].CSM_timer_note -= (inst & 0xff);
					}
					
					else
					{
						chn->ops[ops_index - 1].CSM_timer_note = 0;
					}
				}
			}
		}
		break;
		
		case MUS_FX_FM_TRIGGER_OP1_RELEASE:
		case MUS_FX_FM_TRIGGER_OP2_RELEASE:
		case MUS_FX_FM_TRIGGER_OP3_RELEASE:
		case MUS_FX_FM_TRIGGER_OP4_RELEASE:
		{
			if (tick == (inst & 0xff))
			{
				if(ops_index == 0xFF || ops_index == 0)
				{
					Uint8 op = (((inst & 0xF000) - (MUS_FX_FM_TRIGGER_OP1_RELEASE & 0xF000)) >> 12);
					
					cydchn->fm.ops[op].flags &= ~CYD_FM_OP_WAVE_OVERRIDE_ENV;
					cydchn->fm.ops[op].adsr.envelope_state = RELEASE;
					
					cydchn->fm.ops[op].flags &= ~CYD_FM_OP_ENABLE_GATE;
					
					cydchn->fm.ops[op].adsr.env_speed = envspd(cyd, cydchn->fm.ops[op].adsr.r);
					
					if(cydchn->fm.ops[op].env_ksl_mult != 0.0 && cydchn->fm.ops[op].env_ksl_mult != 1.0)
					{
						cydchn->fm.ops[op].adsr.env_speed = (int)((double)envspd(cyd, cydchn->fm.ops[op].adsr.r) * cydchn->fm.ops[op].env_ksl_mult);
					}
					
					if(chn->instrument != NULL)
					{
						for(int pr = 0; pr < chn->instrument->ops[op].num_macros; ++pr)
						{
							for(int j = 0; j < MUS_PROG_LEN; ++j)
							{
								if((chn->instrument->ops[op].program[pr][j] & 0xff00) == MUS_FX_RELEASE_POINT)
								{
									chn->ops[op].program_tick[pr] = j + 1;
									break;
								}
							}
						}
					}
				}
			}
		}
		break;
		
		case MUS_FX_SET_ATTACK_RATE:
		{
			if(ops_index == 0xFF || ops_index == 0)
			{
				if((inst & 0xff) <= 0x3f) //for current channel only
				{
					cydchn->adsr.a = (inst & 0x3f);
					
					if (cydchn->adsr.envelope_state == ATTACK)
					{
						cydchn->adsr.env_speed = envspd(cyd, cydchn->adsr.a);
					}
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							cydchn->fm.ops[i].adsr.a = (inst & 0x3f);
							
							if (cydchn->fm.ops[i].adsr.envelope_state == ATTACK)
							{
								cydchn->fm.ops[i].adsr.env_speed = envspd(cyd, cydchn->fm.ops[i].adsr.a);
							}
						}
					}
				}
				
				else //for all channels that use this instrument
				{
					for (int chann = 0; chann < cyd->n_channels; ++chann)
					{
						if(mus->channel[chann].instrument == chn->instrument)
						{
							cyd->channel[chann].adsr.a = (inst & 0x3f);
							
							if (cyd->channel[chann].adsr.envelope_state == ATTACK)
							{
								cyd->channel[chann].adsr.env_speed = envspd(cyd, cyd->channel[chann].adsr.a);
							}
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									cyd->channel[chann].fm.ops[i].adsr.a = (inst & 0x3f);
									
									if (cyd->channel[chann].fm.ops[i].adsr.envelope_state == ATTACK)
									{
										cyd->channel[chann].fm.ops[i].adsr.env_speed = envspd(cyd, cyd->channel[chann].fm.ops[i].adsr.a);
									}
								}
							}
						}
					}
				}
			}
			
			else
			{
				cydchn->fm.ops[ops_index - 1].adsr.a = (inst & 0x3f);
				
				if (cydchn->fm.ops[ops_index - 1].adsr.envelope_state == ATTACK)
				{
					cydchn->fm.ops[ops_index - 1].adsr.env_speed = envspd(cyd, cydchn->fm.ops[ops_index - 1].adsr.a);
				}
			}
		}
		break;
		
		case MUS_FX_SET_DECAY_RATE:
		{
			if(ops_index == 0xFF || ops_index == 0)
			{
				if((inst & 0xff) <= 0x3f) //for current channel only
				{
					cydchn->adsr.d = (inst & 0x3f);
					
					if (cydchn->adsr.envelope_state == DECAY)
					{
						cydchn->adsr.env_speed = envspd(cyd, cydchn->adsr.d);
					}
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							if (cydchn->fm.ops[i].adsr.envelope_state == DECAY)
							{
								cydchn->fm.ops[i].adsr.env_speed = envspd(cyd, cydchn->fm.ops[i].adsr.d);
							}
						}
					}
				}
				
				else //for all channels that use this instrument
				{
					for (int chann = 0; chann < cyd->n_channels; ++chann)
					{
						if(mus->channel[chann].instrument == chn->instrument)
						{
							cyd->channel[chann].adsr.d = (inst & 0x3f);
							
							if (cyd->channel[chann].adsr.envelope_state == DECAY)
							{
								cyd->channel[chann].adsr.env_speed = envspd(cyd, cyd->channel[chann].adsr.d);
							}
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									if (cyd->channel[chann].fm.ops[i].adsr.envelope_state == DECAY)
									{
										cyd->channel[chann].fm.ops[i].adsr.env_speed = envspd(cyd, cyd->channel[chann].fm.ops[i].adsr.d);
									}
								}
							}
						}
					}
				}
			}
			
			else
			{
				cydchn->fm.ops[ops_index - 1].adsr.d = (inst & 0x3f);
				
				if (cydchn->fm.ops[ops_index - 1].adsr.envelope_state == DECAY)
				{
					cydchn->fm.ops[ops_index - 1].adsr.env_speed = envspd(cyd, cydchn->fm.ops[ops_index - 1].adsr.d);
				}
			}
		}
		break;
		
		case MUS_FX_SET_SUSTAIN_LEVEL:
		{
			if(ops_index == 0xFF || ops_index == 0)
			{
				if((inst & 0xff) <= 0x1f) //for current channel only
				{
					cydchn->adsr.s = (inst & 0x1f);
					
					if (cydchn->adsr.envelope_state == SUSTAIN)
					{
						cydchn->adsr.envelope = (Uint32)cydchn->adsr.s << 19;
					}
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							cydchn->fm.ops[i].adsr.s = (inst & 0x1f);
							
							if (cydchn->fm.ops[i].adsr.envelope_state == SUSTAIN)
							{
								cydchn->fm.ops[i].adsr.envelope = (Uint32)cydchn->fm.ops[i].adsr.s << 19;
							}
						}
					}
				}
				
				else //for all channels that use this instrument
				{
					for (int chann = 0; chann < cyd->n_channels; ++chann)
					{
						if(mus->channel[chann].instrument == chn->instrument)
						{
							cyd->channel[chann].adsr.s = (inst & 0x1f);
							
							if (cyd->channel[chann].adsr.envelope_state == SUSTAIN)
							{
								cyd->channel[chann].adsr.envelope = (Uint32)cyd->channel[chann].adsr.s << 19;
							}
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									cyd->channel[chann].fm.ops[i].adsr.s = (inst & 0x1f);
									
									if (cyd->channel[chann].fm.ops[i].adsr.envelope_state == SUSTAIN)
									{
										cyd->channel[chann].fm.ops[i].adsr.envelope = (Uint32)cyd->channel[chann].fm.ops[i].adsr.s << 19;
									}
								}
							}
						}
					}
				}
			}
			
			else
			{
				if((inst & 0xff) <= 0x1f) //for current channel only
				{
					cydchn->fm.ops[ops_index - 1].adsr.s = (inst & 0x1f);
					
					if (cydchn->fm.ops[ops_index - 1].adsr.envelope_state == SUSTAIN)
					{
						cydchn->fm.ops[ops_index - 1].adsr.envelope = (Uint32)cydchn->fm.ops[ops_index - 1].adsr.s << 19;
					}
				}
				
				else //for all channels that use this instrument
				{
					for (int chann = 0; chann < cyd->n_channels; ++chann)
					{
						if(mus->channel[chann].instrument == chn->instrument)
						{
							cyd->channel[chann].fm.ops[ops_index - 1].adsr.s = (inst & 0x1f);
							
							if (cyd->channel[chann].fm.ops[ops_index - 1].adsr.envelope_state == SUSTAIN)
							{
								cyd->channel[chann].fm.ops[ops_index - 1].adsr.envelope = (Uint32)cyd->channel[chann].fm.ops[ops_index - 1].adsr.s << 19;
							}
						}
					}
				}
			}
		}
		break;
		
		case MUS_FX_SET_SUSTAIN_RATE:
		{
			if(ops_index != 0xFF && ops_index != 0) //only in FM op program
			{
				cydchn->fm.ops[ops_index - 1].adsr.sr = (inst & 0x3f);
				
				if (cydchn->fm.ops[ops_index - 1].adsr.envelope_state == SUSTAIN)
				{
					cydchn->fm.ops[ops_index - 1].adsr.env_speed = cydchn->fm.ops[ops_index - 1].adsr.sr == 0 ? 0 : envspd(cyd, 64 - cydchn->fm.ops[ops_index - 1].adsr.sr);
				}
			}
		}
		break;
		
		case MUS_FX_SET_RELEASE_RATE:
		{
			if(ops_index == 0xFF || ops_index == 0)
			{
				if((inst & 0xff) <= 0x3f) //for current channel only
				{
					cydchn->adsr.r = (inst & 0x3f);
					
					if (cydchn->adsr.envelope_state == RELEASE)
					{
						cydchn->adsr.env_speed = envspd(cyd, cydchn->adsr.r);
					}
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							if (cydchn->fm.ops[i].adsr.envelope_state == RELEASE)
							{
								cydchn->fm.ops[i].adsr.env_speed = envspd(cyd, cydchn->fm.ops[i].adsr.r);
							}
						}
					}
				}
				
				else //for all channels that use this instrument
				{
					for (int chann = 0; chann < cyd->n_channels; ++chann)
					{
						if(mus->channel[chann].instrument == chn->instrument)
						{
							cyd->channel[chann].adsr.r = (inst & 0x3f);
							
							if (cyd->channel[chann].adsr.envelope_state == RELEASE)
							{
								cyd->channel[chann].adsr.env_speed = envspd(cyd, cyd->channel[chann].adsr.r);
							}
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									cyd->channel[chann].fm.ops[i].adsr.r = (inst & 0x3f);
									
									if (cyd->channel[chann].fm.ops[i].adsr.envelope_state == RELEASE)
									{
										cyd->channel[chann].fm.ops[i].adsr.env_speed = envspd(cyd, cyd->channel[chann].fm.ops[i].adsr.r);
									}
								}
							}
						}
					}
				}
			}
			
			else
			{
				cydchn->fm.ops[ops_index - 1].adsr.r = (inst & 0x3f);
				
				if (cydchn->fm.ops[ops_index - 1].adsr.envelope_state == RELEASE)
				{
					cydchn->fm.ops[ops_index - 1].adsr.env_speed = envspd(cyd, cydchn->fm.ops[ops_index - 1].adsr.r);
				}
			}
		}
		break;
		
		case MUS_FX_FM_SET_OP1_ATTACK_RATE:
		case MUS_FX_FM_SET_OP2_ATTACK_RATE:
		case MUS_FX_FM_SET_OP3_ATTACK_RATE:
		case MUS_FX_FM_SET_OP4_ATTACK_RATE:
		{
			if(ops_index == 0xFF || ops_index == 0)
			{
				Uint8 op = (((inst & 0xF000) - (MUS_FX_FM_SET_OP1_ATTACK_RATE & 0xF000)) >> 12);
				
				if((inst & 0xff) <= 0x3f) //for current channel only
				{
					cydchn->fm.ops[op].adsr.a = (inst & 0x3f);
					
					if (cydchn->fm.ops[op].adsr.envelope_state == ATTACK)
					{
						cydchn->fm.ops[op].adsr.env_speed = envspd(cyd, cydchn->fm.ops[op].adsr.a);
					}
				}
				
				else //for all channels that use this instrument
				{
					for (int chann = 0; chann < cyd->n_channels; ++chann)
					{
						if(mus->channel[chann].instrument == chn->instrument)
						{
							cyd->channel[chann].fm.ops[op].adsr.a = (inst & 0x3f);
							
							if (cyd->channel[chann].fm.ops[op].adsr.envelope_state == ATTACK)
							{
								cyd->channel[chann].fm.ops[op].adsr.env_speed = envspd(cyd, cyd->channel[chann].fm.ops[op].adsr.a);
							}
						}
					}
				}
			}
		}
		break;
		
		case MUS_FX_FM_SET_OP1_DECAY_RATE:
		case MUS_FX_FM_SET_OP2_DECAY_RATE:
		case MUS_FX_FM_SET_OP3_DECAY_RATE:
		case MUS_FX_FM_SET_OP4_DECAY_RATE:
		{
			if(ops_index == 0xFF || ops_index == 0)
			{
				Uint8 op = (((inst & 0xF000) - (MUS_FX_FM_SET_OP1_DECAY_RATE & 0xF000)) >> 12);
				
				if((inst & 0xff) <= 0x3f) //for current channel only
				{
					cydchn->fm.ops[op].adsr.d = (inst & 0x3f);
					
					if (cydchn->fm.ops[op].adsr.envelope_state == DECAY)
					{
						cydchn->fm.ops[op].adsr.env_speed = envspd(cyd, cydchn->fm.ops[op].adsr.d);
					}
				}
				
				else //for all channels that use this instrument
				{
					for (int chann = 0; chann < cyd->n_channels; ++chann)
					{
						if(mus->channel[chann].instrument == chn->instrument)
						{
							cyd->channel[chann].fm.ops[op].adsr.d = (inst & 0x3f);
							
							if (cyd->channel[chann].fm.ops[op].adsr.envelope_state == DECAY)
							{
								cyd->channel[chann].fm.ops[op].adsr.env_speed = envspd(cyd, cyd->channel[chann].fm.ops[op].adsr.d);
							}
						}
					}
				}
			}
		}
		break;
		
		case MUS_FX_FM_SET_OP1_SUSTAIN_LEVEL:
		case MUS_FX_FM_SET_OP2_SUSTAIN_LEVEL:
		case MUS_FX_FM_SET_OP3_SUSTAIN_LEVEL:
		case MUS_FX_FM_SET_OP4_SUSTAIN_LEVEL:
		{
			if(ops_index == 0xFF || ops_index == 0)
			{
				Uint8 op = (((inst & 0xF000) - (MUS_FX_FM_SET_OP1_SUSTAIN_LEVEL & 0xF000)) >> 12);
				
				if((inst & 0xff) <= 0x1f) //for current channel only
				{
					cydchn->fm.ops[op].adsr.s = (inst & 0x1f);
					
					if (cydchn->fm.ops[op].adsr.envelope_state == SUSTAIN)
					{
						cydchn->fm.ops[op].adsr.envelope = (Uint32)cydchn->fm.ops[op].adsr.s << 19;
					}
				}
				
				else //for all channels that use this instrument
				{
					for (int chann = 0; chann < cyd->n_channels; ++chann)
					{
						if(mus->channel[chann].instrument == chn->instrument)
						{
							cyd->channel[chann].fm.ops[op].adsr.s = (inst & 0x1f);
							
							if (cyd->channel[chann].fm.ops[op].adsr.envelope_state == SUSTAIN)
							{
								cyd->channel[chann].fm.ops[op].adsr.envelope = (Uint32)cyd->channel[chann].fm.ops[op].adsr.s << 19;
							}
						}
					}
				}
			}
		}
		break;
		
		case MUS_FX_FM_SET_OP1_SUSTAIN_RATE:
		case MUS_FX_FM_SET_OP2_SUSTAIN_RATE:
		case MUS_FX_FM_SET_OP3_SUSTAIN_RATE:
		case MUS_FX_FM_SET_OP4_SUSTAIN_RATE:
		{
			if(ops_index == 0xFF || ops_index == 0)
			{
				Uint8 op = (((inst & 0xF000) - (MUS_FX_FM_SET_OP1_SUSTAIN_RATE & 0xF000)) >> 12);
				
				if((inst & 0xff) <= 0x3f) //for current channel only
				{
					cydchn->fm.ops[op].adsr.sr = (inst & 0x3f);
					
					if (cydchn->fm.ops[op].adsr.envelope_state == SUSTAIN)
					{
						cydchn->fm.ops[op].adsr.env_speed = envspd(cyd, 64 - cydchn->fm.ops[op].adsr.sr);
					}
				}
				
				else //for all channels that use this instrument
				{
					for (int chann = 0; chann < cyd->n_channels; ++chann)
					{
						if(mus->channel[chann].instrument == chn->instrument)
						{
							cyd->channel[chann].fm.ops[op].adsr.sr = (inst & 0x3f);
							
							if (cyd->channel[chann].fm.ops[op].adsr.envelope_state == SUSTAIN)
							{
								cyd->channel[chann].fm.ops[op].adsr.env_speed = cyd->channel[chann].fm.ops[op].adsr.sr == 0 ? 0 : envspd(cyd, 64 - cyd->channel[chann].fm.ops[op].adsr.sr);
							}
						}
					}
				}
			}
		}
		break;
		
		case MUS_FX_FM_SET_OP1_RELEASE_RATE:
		case MUS_FX_FM_SET_OP2_RELEASE_RATE:
		case MUS_FX_FM_SET_OP3_RELEASE_RATE:
		case MUS_FX_FM_SET_OP4_RELEASE_RATE:
		{
			if(ops_index == 0xFF || ops_index == 0)
			{
				Uint8 op = (((inst & 0xF000) - (MUS_FX_FM_SET_OP1_RELEASE_RATE & 0xF000)) >> 12);
				
				if((inst & 0xff) <= 0x3f) //for current channel only
				{
					cydchn->fm.ops[op].adsr.r = (inst & 0x3f);
					
					if (cydchn->fm.ops[op].adsr.envelope_state == RELEASE)
					{
						cydchn->fm.ops[op].adsr.env_speed = envspd(cyd, cydchn->fm.ops[op].adsr.r);
					}
				}
				
				else //for all channels that use this instrument
				{
					for (int chann = 0; chann < cyd->n_channels; ++chann)
					{
						if(mus->channel[chann].instrument == chn->instrument)
						{
							cyd->channel[chann].fm.ops[op].adsr.r = (inst & 0x3f);
							
							if (cyd->channel[chann].fm.ops[op].adsr.envelope_state == RELEASE)
							{
								cyd->channel[chann].fm.ops[op].adsr.env_speed = envspd(cyd, cyd->channel[chann].fm.ops[op].adsr.r);
							}
						}
					}
				}
			}
		}
		break;
		
		case MUS_FX_FM_SET_OP1_MULT:
		case MUS_FX_FM_SET_OP2_MULT:
		case MUS_FX_FM_SET_OP3_MULT:
		case MUS_FX_FM_SET_OP4_MULT:
		{
			if(ops_index == 0xFF || ops_index == 0)
			{
				Uint8 op = (((inst & 0xF000) - (MUS_FX_FM_SET_OP1_MULT & 0xF000)) >> 12);
				
				cydchn->fm.ops[op].harmonic = (inst & 0xff);
			}
		}
		break;
		
		case MUS_FX_FM_SET_OP1_VOL:
		case MUS_FX_FM_SET_OP2_VOL:
		case MUS_FX_FM_SET_OP3_VOL:
		case MUS_FX_FM_SET_OP4_VOL:
		{
			if(ops_index == 0xFF || ops_index == 0)
			{
				Uint8 op = (((inst & 0xF000) - (MUS_FX_FM_SET_OP1_VOL & 0xF000)) >> 12);
				
				track_status->ops_status[op].volume = my_min(MAX_VOLUME, inst & 0xff);
				update_fm_op_volume(mus, track_status, chn, cydchn, track_status->ops_status[op].volume, op);
			}
		}
		break;
		
#ifndef CYD_DISABLE_WAVETABLE
		case MUS_FX_WAVETABLE_OFFSET_UP: //wasn't there
		{
			switch(ops_index)
			{
				case 0:
				case 0xFF:
				{
					if (cydchn->wave_entry)
					{
						if ((cydchn->wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->subosc[0].wave.start_offset + (cydchn->subosc[0].wave.start_offset + (inst & 0xff) * 16 > 0xffff ? 0xffff : (inst & 0xff) * 16) < cydchn->subosc[0].wave.end_offset))
						{
							for (int s = 0; s < CYD_SUB_OSCS; ++s)
							{
								cydchn->subosc[s].wave.start_offset += (cydchn->subosc[s].wave.start_offset + (inst & 0xff) * 16 > 0xffff ? 0xffff : (inst & 0xff) * 16);
								
								cydchn->subosc[s].wave.start_point_track_status = (Uint64)cydchn->subosc[s].wave.start_offset * WAVETABLE_RESOLUTION * cydchn->wave_entry->samples / 0x10000;
								
								cydchn->subosc[s].wave.use_start_track_status_offset = true;
							}	
						}
					}
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							if (cydchn->fm.ops[i].wave_entry)
							{
								for (int s = 0; s < CYD_SUB_OSCS; ++s)
								{
									if ((cydchn->fm.ops[i].wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->fm.ops[i].subosc[s].wave.start_offset + (cydchn->fm.ops[i].subosc[s].wave.start_offset + (inst & 0xff) * 16 > 0xffff ? 0xffff : (inst & 0xff) * 16) < cydchn->fm.ops[i].subosc[s].wave.end_offset))
									{
										
										cydchn->fm.ops[i].subosc[s].wave.start_offset += (cydchn->fm.ops[i].subosc[s].wave.start_offset + (inst & 0xff) * 16 > 0xffff ? 0xffff : (inst & 0xff) * 16);
										
										cydchn->fm.ops[i].subosc[s].wave.start_point_track_status = (Uint64)cydchn->fm.ops[i].subosc[s].wave.start_offset * WAVETABLE_RESOLUTION * cydchn->fm.ops[i].wave_entry->samples / 0x10000;
										
										cydchn->fm.ops[i].subosc[s].wave.use_start_track_status_offset = true;
									}
								}
							}
						}
					}
					
					break;
				}
				
				default:
				{
					if (cydchn->fm.ops[ops_index - 1].wave_entry)
					{
						for (int s = 0; s < CYD_SUB_OSCS; ++s)
						{
							if ((cydchn->fm.ops[ops_index - 1].wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_offset + (cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_offset + (inst & 0xff) * 16 > 0xffff ? 0xffff : (inst & 0xff) * 16) < cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_offset))
							{
								cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_offset += (cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_offset + (inst & 0xff) * 16 > 0xffff ? 0xffff : (inst & 0xff) * 16);
								
								cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_point_track_status = (Uint64)cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_offset * WAVETABLE_RESOLUTION * cydchn->fm.ops[ops_index - 1].wave_entry->samples / 0x10000;
								
								cydchn->fm.ops[ops_index - 1].subosc[s].wave.use_start_track_status_offset = true;
							}
						}
					}
				}
			}
		}
		break;
		
		case MUS_FX_WAVETABLE_OFFSET_DOWN: //wasn't there
		{
			switch(ops_index)
			{
				case 0:
				case 0xFF:
				{
					if (cydchn->wave_entry)
					{
						if ((cydchn->wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->subosc[0].wave.start_offset - ((Sint32)cydchn->subosc[0].wave.start_offset - (inst & 0xff) * 16 < 0 ? 0 : (inst & 0xff) * 16) < cydchn->subosc[0].wave.end_offset))
						{
							for (int s = 0; s < CYD_SUB_OSCS; ++s)
							{
								cydchn->subosc[s].wave.start_offset -= ((Sint32)cydchn->subosc[s].wave.start_offset - (inst & 0xff) * 16 < 0 ? 0 : (inst & 0xff) * 16);
								
								cydchn->subosc[s].wave.start_point_track_status = (Uint64)cydchn->subosc[s].wave.start_offset * WAVETABLE_RESOLUTION * cydchn->wave_entry->samples / 0x10000;
								
								cydchn->subosc[s].wave.use_start_track_status_offset = true;
							}	
						}
					}
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							if (cydchn->fm.ops[i].wave_entry)
							{
								for (int s = 0; s < CYD_SUB_OSCS; ++s)
								{
									if ((cydchn->fm.ops[i].wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->fm.ops[i].subosc[s].wave.start_offset - ((Sint32)cydchn->fm.ops[i].subosc[s].wave.start_offset - (inst & 0xff) * 16 < 0 ? 0 : (inst & 0xff) * 16) < cydchn->fm.ops[i].subosc[s].wave.end_offset))
									{
										cydchn->fm.ops[i].subosc[s].wave.start_offset -= ((Sint32)cydchn->fm.ops[i].subosc[s].wave.start_offset - (inst & 0xff) * 16 < 0 ? 0 : (inst & 0xff) * 16);
										
										cydchn->fm.ops[i].subosc[s].wave.start_point_track_status = (Uint64)cydchn->fm.ops[i].subosc[s].wave.start_offset * WAVETABLE_RESOLUTION * cydchn->fm.ops[i].wave_entry->samples / 0x10000;
										
										cydchn->fm.ops[i].subosc[s].wave.use_start_track_status_offset = true;
									}
								}
							}
						}
					}
					
					break;
				}
				
				default:
				{
					if (cydchn->fm.ops[ops_index - 1].wave_entry)
					{
						for (int s = 0; s < CYD_SUB_OSCS; ++s)
						{
							if ((cydchn->fm.ops[ops_index - 1].wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_offset - ((Sint32)cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_offset - (inst & 0xff) * 16 < 0 ? 0 : (inst & 0xff) * 16) < cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_offset))
							{
								cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_offset -= ((Sint32)cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_offset - (inst & 0xff) * 16 < 0 ? 0 : (inst & 0xff) * 16);
								
								cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_point_track_status = (Uint64)cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_offset * WAVETABLE_RESOLUTION * cydchn->fm.ops[ops_index - 1].wave_entry->samples / 0x10000;
								
								cydchn->fm.ops[ops_index - 1].subosc[s].wave.use_start_track_status_offset = true;
							}
						}
					}
					
					break;
				}
			}
		}
		break;
		
		case MUS_FX_WAVETABLE_END_POINT_UP: //wasn't there
		{
			switch(ops_index)
			{
				case 0:
				case 0xFF:
				{
					if (cydchn->wave_entry)
					{
						if ((cydchn->wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->subosc[0].wave.end_offset - ((Sint32)cydchn->subosc[0].wave.end_offset - (inst & 0xff) * 16 < 0 ? 0 : (inst & 0xff) * 16) > cydchn->subosc[0].wave.start_offset))
						{
							for (int s = 0; s < CYD_SUB_OSCS; ++s)
							{
								cydchn->subosc[s].wave.end_offset -= ((Sint32)cydchn->subosc[s].wave.end_offset - (inst & 0xff) * 16 < 0 ? 0 : (inst & 0xff) * 16);
								
								cydchn->subosc[s].wave.end_point_track_status = (Uint64)cydchn->subosc[s].wave.end_offset * WAVETABLE_RESOLUTION * cydchn->wave_entry->samples / 0x10000;
								
								cydchn->subosc[s].wave.use_end_track_status_offset = true;
							}	
						}
					}
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							if (cydchn->fm.ops[i].wave_entry)
							{
								for (int s = 0; s < CYD_SUB_OSCS; ++s)
								{
									if ((cydchn->fm.ops[i].wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->fm.ops[i].subosc[s].wave.end_offset - ((Sint32)cydchn->fm.ops[i].subosc[s].wave.end_offset - (inst & 0xff) * 16 < 0 ? 0 : (inst & 0xff) * 16) > cydchn->fm.ops[i].subosc[s].wave.start_offset))
									{
										cydchn->fm.ops[i].subosc[s].wave.end_offset -= ((Sint32)cydchn->fm.ops[i].subosc[s].wave.end_offset - (inst & 0xff) * 16 < 0 ? 0 : (inst & 0xff) * 16);
										
										cydchn->fm.ops[i].subosc[s].wave.end_point_track_status = (Uint64)cydchn->fm.ops[i].subosc[s].wave.end_offset * WAVETABLE_RESOLUTION * cydchn->fm.ops[i].wave_entry->samples / 0x10000;
										
										cydchn->fm.ops[i].subosc[s].wave.use_end_track_status_offset = true;
									}
								}
							}
						}
					}
					
					break;
				}
				
				default:
				{
					if (cydchn->fm.ops[ops_index - 1].wave_entry)
					{
						for (int s = 0; s < CYD_SUB_OSCS; ++s)
						{
							if ((cydchn->fm.ops[ops_index - 1].wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_offset - ((Sint32)cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_offset - (inst & 0xff) * 16 < 0 ? 0 : (inst & 0xff) * 16) > cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_offset))
							{
								cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_offset -= ((Sint32)cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_offset - (inst & 0xff) * 16 < 0 ? 0 : (inst & 0xff) * 16);
								
								cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_point_track_status = (Uint64)cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_offset * WAVETABLE_RESOLUTION * cydchn->fm.ops[ops_index - 1].wave_entry->samples / 0x10000;
								
								cydchn->fm.ops[ops_index - 1].subosc[s].wave.use_end_track_status_offset = true;
							}
						}
					}
					
					break;
				}
			}
		}
		break;
		
		case MUS_FX_WAVETABLE_END_POINT_DOWN: //wasn't there
		{
			switch(ops_index)
			{
				case 0:
				case 0xFF:
				{
					if (cydchn->wave_entry)
					{
						if ((cydchn->wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->subosc[0].wave.end_offset + (cydchn->subosc[0].wave.end_offset + (inst & 0xff) * 16 > 0xffff ? 0xffff : (inst & 0xff) * 16) > cydchn->subosc[0].wave.start_offset))
						{
							for (int s = 0; s < CYD_SUB_OSCS; ++s)
							{
								cydchn->subosc[s].wave.end_offset += (cydchn->subosc[s].wave.end_offset + (inst & 0xff) * 16 > 0xffff ? 0xffff : (inst & 0xff) * 16);
								
								cydchn->subosc[s].wave.end_point_track_status = (Uint64)cydchn->subosc[s].wave.end_offset * WAVETABLE_RESOLUTION * cydchn->wave_entry->samples / 0x10000;
								
								cydchn->subosc[s].wave.use_end_track_status_offset = true;
							}	
						}
					}
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							if (cydchn->fm.ops[i].wave_entry)
							{
								for (int s = 0; s < CYD_SUB_OSCS; ++s)
								{
									if ((cydchn->fm.ops[i].wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->fm.ops[i].subosc[s].wave.end_offset + (cydchn->fm.ops[i].subosc[s].wave.end_offset + (inst & 0xff) * 16 > 0xffff ? 0xffff : (inst & 0xff) * 16) > cydchn->fm.ops[i].subosc[s].wave.start_offset))
									{
										cydchn->fm.ops[i].subosc[s].wave.end_offset += (cydchn->fm.ops[i].subosc[s].wave.end_offset + (inst & 0xff) * 16 > 0xffff ? 0xffff : (inst & 0xff) * 16);
										
										cydchn->fm.ops[i].subosc[s].wave.end_point_track_status = (Uint64)cydchn->fm.ops[i].subosc[s].wave.end_offset * WAVETABLE_RESOLUTION * cydchn->fm.ops[i].wave_entry->samples / 0x10000;
										
										cydchn->fm.ops[i].subosc[s].wave.use_end_track_status_offset = true;
									}
								}
							}
						}
					}
					
					break;
				}
				
				default:
				{
					if (cydchn->fm.ops[ops_index - 1].wave_entry)
					{
						for (int s = 0; s < CYD_SUB_OSCS; ++s)
						{
							if ((cydchn->fm.ops[ops_index - 1].wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_offset + (cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_offset + (inst & 0xff) * 16 > 0xffff ? 0xffff : (inst & 0xff) * 16) > cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_offset))
							{
								cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_offset += (cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_offset + (inst & 0xff) * 16 > 0xffff ? 0xffff : (inst & 0xff) * 16);
								
								cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_point_track_status = (Uint64)cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_offset * WAVETABLE_RESOLUTION * cydchn->fm.ops[ops_index - 1].wave_entry->samples / 0x10000;
								
								cydchn->fm.ops[ops_index - 1].subosc[s].wave.use_end_track_status_offset = true;
							}
						}
					}
					
					break;
				}
			}
		}
		break;
		
		case MUS_FX_FM_WAVETABLE_OFFSET: //wasn't there
		{
			if(ops_index == 0 || ops_index == 0xFF)
			{
				int offset = ((inst & 0xff) << 4);
				
				if (cydchn->fm.wave_entry)
				{
					if(cydchn->fm.wave_entry->flags & CYD_WAVE_LOOP)
					{
						cydchn->fm.wave.start_offset = offset * 16;
						
						cydchn->fm.wave.start_point_track_status = (Uint64)cydchn->fm.wave.start_offset * WAVETABLE_RESOLUTION * cydchn->fm.wave_entry->samples / 0x10000;
						
						cydchn->fm.wave.use_start_track_status_offset = true;
					}
				}
			}
		}
		break;
		
		case MUS_FX_FM_WAVETABLE_END_POINT: //wasn't there
		{
			if(ops_index == 0 || ops_index == 0xFF)
			{
				int offset = ((inst & 0xff) << 4);
				
				if (cydchn->fm.wave_entry)
				{
					if(cydchn->fm.wave_entry->flags & CYD_WAVE_LOOP)
					{
						cydchn->fm.wave.end_offset = (0x10000 - offset * 16) > 0xFFFF ? 0xffff : (0x10000 - offset * 16);
						
						cydchn->fm.wave.end_point_track_status = (Uint64)cydchn->fm.wave.end_offset * WAVETABLE_RESOLUTION * cydchn->fm.wave_entry->samples / 0x10000;
						
						cydchn->fm.wave.use_end_track_status_offset = true;
					}
				}
			}
		}
		break;
		
		case MUS_FX_FM_WAVETABLE_OFFSET_UP: //wasn't there
		{
			if(ops_index == 0 || ops_index == 0xFF)
			{
				if (cydchn->fm.wave_entry)
				{
					//if(cydchn->fm.wave_entry->flags & CYD_WAVE_LOOP)
					if ((cydchn->fm.wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->fm.wave.start_offset + (cydchn->fm.wave.start_offset + (inst & 0xff) * 16 > 0xffff ? 0xffff : (inst & 0xff) * 16) < cydchn->fm.wave.end_offset))
					{
						cydchn->fm.wave.start_offset += (cydchn->fm.wave.start_offset + (inst & 0xff) * 16 > 0xffff ? 0xffff : (inst & 0xff) * 16);
						
						cydchn->fm.wave.start_point_track_status = (Uint64)cydchn->fm.wave.start_offset * WAVETABLE_RESOLUTION * cydchn->fm.wave_entry->samples / 0x10000;
						
						cydchn->fm.wave.use_start_track_status_offset = true;
					}
				}
			}
		}
		break;
		
		case MUS_FX_FM_WAVETABLE_OFFSET_DOWN: //wasn't there
		{
			if(ops_index == 0 || ops_index == 0xFF)
			{
				if (cydchn->fm.wave_entry)
				{
					//if(cydchn->fm.wave_entry->flags & CYD_WAVE_LOOP)
					if ((cydchn->fm.wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->fm.wave.start_offset - ((Sint32)cydchn->fm.wave.start_offset - (inst & 0xff) * 16 < 0 ? 0 : (inst & 0xff) * 16) < cydchn->fm.wave.end_offset))
					{
						cydchn->fm.wave.start_offset -= ((Sint32)cydchn->fm.wave.start_offset - (inst & 0xff) * 16 < 0 ? 0 : (inst & 0xff) * 16);
						
						cydchn->fm.wave.start_point_track_status = (Uint64)cydchn->fm.wave.start_offset * WAVETABLE_RESOLUTION * cydchn->fm.wave_entry->samples / 0x10000;
						
						cydchn->fm.wave.use_start_track_status_offset = true;
					}
				}
			}
		}
		break;
		
		case MUS_FX_FM_WAVETABLE_END_POINT_UP: //wasn't there
		{
			if(ops_index == 0 || ops_index == 0xFF)
			{
				if (cydchn->fm.wave_entry)
				{
					//if(cydchn->fm.wave_entry->flags & CYD_WAVE_LOOP)
					if ((cydchn->fm.wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->fm.wave.end_offset - ((Sint32)cydchn->fm.wave.end_offset - (inst & 0xff) * 16 < 0 ? 0 : (inst & 0xff) * 16) > cydchn->fm.wave.start_offset))
					{
						cydchn->fm.wave.end_offset -= ((Sint32)cydchn->fm.wave.end_offset - (inst & 0xff) * 16 < 0 ? 0 : (inst & 0xff) * 16);
						
						cydchn->fm.wave.end_point_track_status = (Uint64)cydchn->fm.wave.end_offset * WAVETABLE_RESOLUTION * cydchn->fm.wave_entry->samples / 0x10000;
						
						cydchn->fm.wave.use_end_track_status_offset = true;
					}
				}
			}
		}
		break;
		
		case MUS_FX_FM_WAVETABLE_END_POINT_DOWN: //wasn't there
		{
			if(ops_index == 0 || ops_index == 0xFF)
			{
				if (cydchn->fm.wave_entry)
				{
					//if(cydchn->fm.wave_entry->flags & CYD_WAVE_LOOP)
					if ((cydchn->fm.wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->fm.wave.end_offset + (cydchn->fm.wave.end_offset + (inst & 0xff) * 16 > 0xffff ? 0xffff : (inst & 0xff) * 16) > cydchn->fm.wave.start_offset))
					{
						cydchn->fm.wave.end_offset += (cydchn->fm.wave.end_offset + (inst & 0xff) * 16 > 0xffff ? 0xffff : (inst & 0xff) * 16);
						
						cydchn->fm.wave.end_point_track_status = (Uint64)cydchn->fm.wave.end_offset * WAVETABLE_RESOLUTION * cydchn->fm.wave_entry->samples / 0x10000;
						
						cydchn->fm.wave.use_end_track_status_offset = true;
					}
				}
			}
		}
		break;
#endif
		case MUS_FX_SET_VOL_KSL_LEVEL: //wasn't there
		{
			switch(ops_index)
			{
				case 0:
				case 0xFF:
				{
					cydchn->vol_ksl_level = inst & 0xff;
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							cydchn->fm.ops[i].vol_ksl_level = inst & 0xff;
						}
					}
					
					break;
				}
				
				default:
				{
					cydchn->fm.ops[ops_index - 1].vol_ksl_level = inst & 0xff;
					
					break;
				}
			}
		}
		break;
		
		case MUS_FX_SET_FM_VOL_KSL_LEVEL: //wasn't there
		{
			if(ops_index == 0 || ops_index == 0xFF)
			{
				cydchn->fm.fm_vol_ksl_level = inst & 0xff;
			}
		}
		break;
		
		case MUS_FX_SET_ENV_KSL_LEVEL: //wasn't there
		{
			switch(ops_index)
			{
				case 0:
				case 0xFF:
				{
					cydchn->env_ksl_level = inst & 0xff;
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							cydchn->fm.ops[i].env_ksl_level = inst & 0xff;
						}
					}
					
					break;
				}
				
				default:
				{
					cydchn->fm.ops[ops_index - 1].env_ksl_level = inst & 0xff;
					
					break;
				}
			}
		}
		break;
		
		case MUS_FX_SET_FM_ENV_KSL_LEVEL: //wasn't there
		{
			if(ops_index == 0 || ops_index == 0xFF)
			{
				cydchn->fm.fm_env_ksl_level = inst & 0xff;
			}
		}
		break;
		
		case MUS_FX_PORTA_UP:
		{
			switch(ops_index)
			{
				case 0:
				case 0xFF:
				{
					Uint32 prev = chn->note;
					chn->note += ((inst & 0xff) << 2);
					if (prev > chn->note) chn->note = 0xffff;

					mus_set_slide(mus, chan, chn->note);
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							Uint32 prev = chn->ops[i].note;
							chn->ops[i].note += ((inst & 0xff) << 2);
							if (prev > chn->ops[i].note) chn->ops[i].note = 0xffff;
							
							chn->ops[i].target_note = chn->ops[i].note;
						}
					}
					
					break;
				}
				
				default:
				{
					Uint32 prev = chn->ops[ops_index - 1].note;
					chn->ops[ops_index - 1].note += ((inst & 0xff) << 2);
					if (prev > chn->ops[ops_index - 1].note) chn->ops[ops_index - 1].note = 0xffff;
					
					chn->ops[ops_index - 1].target_note = chn->ops[ops_index - 1].note;
					
					//debug("chn->ops[ops_index - 1].target_note %d", chn->ops[ops_index - 1].target_note);
					
					break;
				}
			}
		}
		break;

		case MUS_FX_PORTA_DN:
		{
			switch(ops_index)
			{
				case 0:
				case 0xFF:
				{
					Sint32 prev = chn->note;
					chn->note -= ((inst & 0xff) << 2);
					if (prev < chn->note) chn->note = 0x0;

					mus_set_slide(mus, chan, chn->note);
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							Uint32 prev = chn->ops[i].note;
							chn->ops[i].note -= ((inst & 0xff) << 2);
							if (prev < chn->ops[i].note) chn->ops[i].note = 0x0;
							
							chn->ops[i].target_note = chn->ops[i].note;
						}
					}
					
					break;
				}
				
				default:
				{
					Uint32 prev = chn->ops[ops_index - 1].note;
					chn->ops[ops_index - 1].note -= ((inst & 0xff) << 2);
					if (prev < chn->ops[ops_index - 1].note) chn->ops[ops_index - 1].note = 0x0;
					
					chn->ops[ops_index - 1].target_note = chn->ops[ops_index - 1].note;
					
					break;
				}
			}
		}
		break;

		case MUS_FX_PORTA_UP_LOG:
		{
			switch(ops_index)
			{
				case 0:
				case 0xFF:
				{
					Uint32 prev = chn->note;
					chn->note += my_max(1, ((Uint64)frequency_table[MIDDLE_C] * (Uint64)(inst & 0xff) / (Uint64)get_freq(chn->note)));
					if (prev > chn->note) chn->note = 0xffff;
					
					mus_set_slide(mus, chan, chn->note);
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							Uint32 prev = chn->ops[i].note;
							chn->ops[i].note += my_max(1, ((Uint64)frequency_table[MIDDLE_C] * (Uint64)(inst & 0xff) / (Uint64)get_freq(chn->ops[i].note)));
							if (prev > chn->ops[i].note) chn->ops[i].note = 0xffff;
							
							chn->ops[i].target_note = chn->ops[i].note;
						}
					}
					
					break;
				}
				
				default:
				{
					Uint32 prev = chn->ops[ops_index - 1].note;
					chn->ops[ops_index - 1].note += my_max(1, ((Uint64)frequency_table[MIDDLE_C] * (Uint64)(inst & 0xff) / (Uint64)get_freq(chn->ops[ops_index - 1].note)));
					if (prev > chn->ops[ops_index - 1].note) chn->ops[ops_index - 1].note = 0xffff;
					
					chn->ops[ops_index - 1].target_note = chn->ops[ops_index - 1].note;
					
					break;
				}
			}
		}
		break;

		case MUS_FX_PORTA_DN_LOG:
		{
			switch(ops_index)
			{
				case 0:
				case 0xFF:
				{
					Uint32 prev = chn->note;
					chn->note -= my_max(1, ((Uint64)frequency_table[MIDDLE_C] * (Uint64)(inst & 0xff) / (Uint64)get_freq(chn->note)));
					if (prev < chn->note) chn->note = 0x0;

					mus_set_slide(mus, chan, chn->note);
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							Uint32 prev = chn->ops[i].note;
							chn->ops[i].note -= my_max(1, ((Uint64)frequency_table[MIDDLE_C] * (Uint64)(inst & 0xff) / (Uint64)get_freq(chn->ops[i].note)));
							if (prev < chn->ops[i].note) chn->ops[i].note = 0x0;

							chn->ops[i].target_note = chn->ops[i].note;
						}
					}
					
					break;
				}
				
				default:
				{
					Uint32 prev = chn->ops[ops_index - 1].note;
					chn->ops[ops_index - 1].note -= my_max(1, ((Uint64)frequency_table[MIDDLE_C] * (Uint64)(inst & 0xff) / (Uint64)get_freq(chn->ops[ops_index - 1].note)));
					if (prev < chn->ops[ops_index - 1].note) chn->ops[ops_index - 1].note = 0x0;

					chn->ops[ops_index - 1].target_note = chn->ops[ops_index - 1].note;
					
					break;
				}
			}
		}
		break;

		case MUS_FX_PW_DN:
		{
			switch(ops_index)
			{
				case 0:
				case 0xFF:
				{
					track_status->pw -= inst & 0xff;
					if (track_status->pw > 0xf000) track_status->pw = 0;
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							track_status->ops_status[i].pw -= inst & 0xff;
							if (track_status->ops_status[i].pw > 0xf000) track_status->ops_status[i].pw = 0;
						}
					}
					
					break;
				}
				
				default:
				{
					track_status->ops_status[ops_index - 1].pw -= inst & 0xff;
					if (track_status->ops_status[ops_index - 1].pw > 0xf000) track_status->ops_status[ops_index - 1].pw = 0;
					
					break;
				}
			}
		}
		break;

		case MUS_FX_PW_UP:
		{
			switch(ops_index)
			{
				case 0:
				case 0xFF:
				{
					track_status->pw += inst & 0xff;
					if (track_status->pw > 0xfff) track_status->pw = 0xfff;
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							track_status->ops_status[i].pw += inst & 0xff;
							if (track_status->ops_status[i].pw > 0xfff) track_status->ops_status[i].pw = 0xfff;
						}
					}
					
					break;
				}
				
				default:
				{
					track_status->ops_status[ops_index - 1].pw += inst & 0xff;
					if (track_status->ops_status[ops_index - 1].pw > 0xfff) track_status->ops_status[ops_index - 1].pw = 0xfff;
					
					break;
				}
			}
		}
		break;

#ifndef CYD_DISABLE_FILTER
		case MUS_FX_CUTOFF_DN:
		{
			switch(ops_index)
			{
				case 0:
				case 0xFF:
				{
					track_status->filter_cutoff -= (inst & 0xff) * 2;
					if (track_status->filter_cutoff > 0xf000) track_status->filter_cutoff = 0;
					cyd_set_filter_coeffs(mus->cyd, cydchn, track_status->filter_cutoff, track_status->filter_resonance);
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							track_status->ops_status[i].filter_cutoff -= (inst & 0xff) * 2;
							if (track_status->ops_status[i].filter_cutoff > 0xf000) track_status->ops_status[i].filter_cutoff = 0;
							
							for(int j = 0; j < (int)pow(2, cydchn->fm.ops[i].flt_slope); ++j)
							{
								for (int sub = 0; sub < CYD_SUB_OSCS; ++sub)
								{
									if(mus->cyd->flags & CYD_USE_OLD_FILTER)
									{
										cydflt_set_coeff_old(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, resonance_table[track_status->ops_status[i].filter_resonance & 3]);
									}
									
									else
									{
										cydflt_set_coeff(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, track_status->ops_status[i].filter_resonance, mus->cyd->sample_rate);
									}
								}
							}
						}
					}
					
					break;
				}
				
				default:
				{
					track_status->ops_status[ops_index - 1].filter_cutoff -= (inst & 0xff) * 2;
					if (track_status->ops_status[ops_index - 1].filter_cutoff > 0xf000) track_status->ops_status[ops_index - 1].filter_cutoff = 0;
					
					for(int j = 0; j < (int)pow(2, cydchn->fm.ops[ops_index - 1].flt_slope); ++j)
					{
						for (int sub = 0; sub < CYD_SUB_OSCS; ++sub)
						{
							if(mus->cyd->flags & CYD_USE_OLD_FILTER)
							{
								cydflt_set_coeff_old(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, resonance_table[track_status->ops_status[ops_index - 1].filter_resonance & 3]);
							}
							
							else
							{
								cydflt_set_coeff(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, track_status->ops_status[ops_index - 1].filter_resonance, mus->cyd->sample_rate);
							}
						}
					}
					
					break;
				}
			}
		}
		break;

		case MUS_FX_CUTOFF_UP:
		{
			switch(ops_index)
			{
				case 0:
				case 0xFF:
				{
					track_status->filter_cutoff += (inst & 0xff) * 2;
					if (track_status->filter_cutoff > 0xfff) track_status->filter_cutoff = 0xfff;
					cyd_set_filter_coeffs(mus->cyd, cydchn, track_status->filter_cutoff, track_status->filter_resonance);
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							track_status->ops_status[i].filter_cutoff += (inst & 0xff) * 2;
							if (track_status->ops_status[i].filter_cutoff > 0xfff) track_status->ops_status[i].filter_cutoff = 0xfff;
							
							for(int j = 0; j < (int)pow(2, cydchn->fm.ops[i].flt_slope); ++j)
							{
								for (int sub = 0; sub < CYD_SUB_OSCS; ++sub)
								{
									if(mus->cyd->flags & CYD_USE_OLD_FILTER)
									{
										cydflt_set_coeff_old(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, resonance_table[track_status->ops_status[i].filter_resonance & 3]);
									}
									
									else
									{
										cydflt_set_coeff(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, track_status->ops_status[i].filter_resonance, mus->cyd->sample_rate);
									}
								}
							}
						}
					}
					
					break;
				}
				
				default:
				{
					track_status->ops_status[ops_index - 1].filter_cutoff += (inst & 0xff) * 2;
					if (track_status->ops_status[ops_index - 1].filter_cutoff > 0xfff) track_status->ops_status[ops_index - 1].filter_cutoff = 0xfff;
					
					for(int j = 0; j < (int)pow(2, cydchn->fm.ops[ops_index - 1].flt_slope); ++j)
					{
						for (int sub = 0; sub < CYD_SUB_OSCS; ++sub)
						{
							if(mus->cyd->flags & CYD_USE_OLD_FILTER)
							{
								cydflt_set_coeff_old(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, resonance_table[track_status->ops_status[ops_index - 1].filter_resonance & 3]);
							}
							
							else
							{
								cydflt_set_coeff(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, track_status->ops_status[ops_index - 1].filter_resonance, mus->cyd->sample_rate);
							}
						}
					}
					
					break;
				}
			}
		}
		break;
#endif

#ifndef CYD_DISABLE_BUZZ
		case MUS_FX_BUZZ_DN:
		{
			if(ops_index == 0 || ops_index == 0xFF)
			{
				if (chn->buzz_offset >= -32768 + (inst & 0xff))
				{
					chn->buzz_offset -= inst & 0xff;
				}

				mus_set_buzz_frequency(mus, chan, chn->note);
			}
		}
		break;

		case MUS_FX_BUZZ_UP:
		{
			if(ops_index == 0 || ops_index == 0xFF)
			{
				if (chn->buzz_offset <= 32767 - (inst & 0xff))
				{
					chn->buzz_offset += inst & 0xff;
				}

				mus_set_buzz_frequency(mus, chan, chn->note);
			}
		}
		break;
#endif
		case MUS_FX_TRIGGER_RELEASE:
		{
			if (tick == (inst & 0xff))
			{
				if(ops_index == 0xFF)
				{
					cyd_enable_gate(mus->cyd, cydchn, 0);
					
					if(chn->instrument != NULL)
					{
						for(int pr = 0; pr < chn->instrument->num_macros; ++pr)
						{
							for(int i = 0; i < MUS_PROG_LEN; ++i)
							{
								if((chn->instrument->program[pr][i] & 0xff00) == MUS_FX_RELEASE_POINT)
								{
									chn->program_tick[pr] = i + 1;
									break;
								}
							}
						}
						
						if(chn->instrument->fm_flags & CYD_FM_ENABLE_4OP)
						{
							for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
							{
								for(int pr = 0; pr < chn->instrument->ops[i].num_macros; ++pr)
								{
									for(int j = 0; j < MUS_PROG_LEN; ++j)
									{
										if((chn->instrument->ops[i].program[pr][j] & 0xff00) == MUS_FX_RELEASE_POINT)
										{
											chn->ops[i].program_tick[pr] = j + 1;
											break;
										}
									}
								}
								
								cydchn->fm.ops[i].adsr.passes = 0;
							}
						}
					}
				}
				
				else if(ops_index == 0)
				{
					if(chn->instrument != NULL)
					{
						for(int pr = 0; pr < chn->instrument->num_macros; ++pr)
						{
							for(int i = 0; i < MUS_PROG_LEN; ++i)
							{
								if((chn->instrument->program[pr][i] & 0xff00) == MUS_FX_RELEASE_POINT)
								{
									chn->program_tick[pr] = i + 1;
									break;
								}
							}
						}
					}
					
					cydchn->flags &= ~CYD_CHN_WAVE_OVERRIDE_ENV;
					cydchn->adsr.envelope_state = RELEASE;
					
					if(cydchn->env_ksl_mult == 0.0 || cydchn->env_ksl_mult == 1.0)
					{
						cydchn->adsr.env_speed = envspd(cyd, cydchn->adsr.r);
					}
					
					else
					{
						cydchn->adsr.env_speed = (int)((double)envspd(cyd, cydchn->adsr.r) * cydchn->env_ksl_mult);
					}
				}
				
				else
				{
					cydchn->fm.ops[ops_index - 1].flags &= ~CYD_FM_OP_WAVE_OVERRIDE_ENV;
					cydchn->fm.ops[ops_index - 1].adsr.envelope_state = RELEASE;
					
					cydchn->fm.ops[ops_index - 1].flags &= ~CYD_FM_OP_ENABLE_GATE;
					
					cydchn->fm.ops[ops_index - 1].adsr.env_speed = envspd(cyd, cydchn->fm.ops[ops_index - 1].adsr.r);
					
					if(cydchn->fm.ops[ops_index - 1].env_ksl_mult != 0.0 && cydchn->fm.ops[ops_index - 1].env_ksl_mult != 1.0)
					{
						cydchn->fm.ops[ops_index - 1].adsr.env_speed = (int)((double)envspd(cyd, cydchn->fm.ops[ops_index - 1].adsr.r) * cydchn->fm.ops[ops_index - 1].env_ksl_mult);
					}
					
					cydchn->fm.ops[ops_index - 1].adsr.passes = 0;
					
					if(chn->instrument != NULL)
					{
						for(int pr = 0; pr < chn->instrument->ops[ops_index - 1].num_macros; ++pr)
						{
							for(int j = 0; j < MUS_PROG_LEN; ++j)
							{
								if((chn->instrument->ops[ops_index - 1].program[pr][j] & 0xff00) == MUS_FX_RELEASE_POINT)
								{
									chn->ops[ops_index - 1].program_tick[pr] = j + 1;
								}
							}
						}
					}
				}
			}
		}
		break;
		
		case MUS_FX_TRIGGER_MACRO_RELEASE:
		{
			if (tick == (inst & 0xff))
			{
				if(ops_index == 0xFF)
				{
					if(chn->instrument != NULL)
					{
						for(int pr = 0; pr < chn->instrument->num_macros; ++pr)
						{
							for(int i = 0; i < MUS_PROG_LEN; ++i)
							{
								if((chn->instrument->program[pr][i] & 0xff00) == MUS_FX_RELEASE_POINT)
								{
									chn->program_tick[pr] = i + 1;
									break;
								}
							}
						}
						
						if(chn->instrument->fm_flags & CYD_FM_ENABLE_4OP)
						{
							for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
							{
								for(int pr = 0; pr < chn->instrument->ops[i].num_macros; ++pr)
								{
									for(int j = 0; j < MUS_PROG_LEN; ++j)
									{
										if((chn->instrument->ops[i].program[pr][j] & 0xff00) == MUS_FX_RELEASE_POINT)
										{
											chn->ops[i].program_tick[pr] = j + 1;
											break;
										}
									}
								}
								
								cydchn->fm.ops[i].adsr.passes = 0;
							}
						}
					}
				}
				
				else if(ops_index == 0)
				{
					if(chn->instrument != NULL)
					{
						for(int pr = 0; pr < chn->instrument->num_macros; ++pr)
						{
							for(int i = 0; i < MUS_PROG_LEN; ++i)
							{
								if((chn->instrument->program[pr][i] & 0xff00) == MUS_FX_RELEASE_POINT)
								{
									chn->program_tick[pr] = i + 1;
									break;
								}
							}
						}
					}
				}
				
				else
				{
					if(chn->instrument != NULL)
					{
						for(int pr = 0; pr < chn->instrument->ops[ops_index - 1].num_macros; ++pr)
						{
							for(int j = 0; j < MUS_PROG_LEN; ++j)
							{
								if((chn->instrument->ops[ops_index - 1].program[pr][j] & 0xff00) == MUS_FX_RELEASE_POINT)
								{
									chn->ops[ops_index - 1].program_tick[pr] = j + 1;
								}
							}
						}
					}
				}
			}
		}
		break;
		
		case MUS_FX_TRIGGER_FM_RELEASE: //wasn't there
		{
			if(ops_index == 0 || ops_index == 0xFF)
			{
				if (tick == (inst & 0xff))
				{
					cydchn->fm.adsr.envelope_state = RELEASE;
					cydchn->fm.adsr.env_speed = (int)((double)envspd(cyd, cydchn->fm.adsr.r) * (cydchn->fm.fm_env_ksl_mult == 0.0 ? 1 : cydchn->fm.fm_env_ksl_mult));
				}
			}
		}
		break;
		
		case MUS_FX_TRIGGER_CARRIER_RELEASE: //wasn't there
		{
			if(ops_index == 0 || ops_index == 0xFF)
			{
				if (tick == (inst & 0xff))
				{
					cydchn->flags &= ~CYD_CHN_WAVE_OVERRIDE_ENV;
					cydchn->adsr.envelope_state = RELEASE;
					
					if(cydchn->env_ksl_mult == 0.0 || cydchn->env_ksl_mult == 1.0)
					{
						cydchn->adsr.env_speed = envspd(cyd, cydchn->adsr.r);
					}
					
					else
					{
						cydchn->adsr.env_speed = (int)((double)envspd(cyd, cydchn->adsr.r) * cydchn->env_ksl_mult);
					}
				}
			}
		}
		break;

		case MUS_FX_FADE_VOLUME:
		{
			switch(ops_index)
			{
				case 0:
				case 0xFF:
				{
					if (!(chn->flags & MUS_CHN_DISABLED))
					{
						track_status->volume -= inst & 0xf;
						if (track_status->volume > MAX_VOLUME) track_status->volume = 0;
						track_status->volume += (inst >> 4) & 0xf;
						if (track_status->volume > MAX_VOLUME) track_status->volume = MAX_VOLUME;

						update_volumes(mus, track_status, chn, cydchn, track_status->volume);
					}
					
					if(ops_index == 0xFF)
					{
						if(track_status->fm_4op_vol + ((inst >> 4) & 0xf) > 0xff)
						{
							track_status->fm_4op_vol = 0xff;
						}
						
						else
						{
							track_status->fm_4op_vol += ((inst >> 4) & 0xf);
						}
						
						if((int)track_status->fm_4op_vol - (int)(inst & 0xf) < 0)
						{
							track_status->fm_4op_vol = 0;
						}
						
						else
						{
							track_status->fm_4op_vol -= (inst & 0xf);
						}
					}
					
					break;
				}
				
				default:
				{
					if (!(chn->ops[ops_index - 1].flags & MUS_FM_OP_DISABLED))
					{
						track_status->ops_status[ops_index - 1].volume -= inst & 0xf;
						if (track_status->ops_status[ops_index - 1].volume > MAX_VOLUME) track_status->ops_status[ops_index - 1].volume = 0;
						track_status->ops_status[ops_index - 1].volume += (inst >> 4) & 0xf;
						if (track_status->ops_status[ops_index - 1].volume > MAX_VOLUME) track_status->ops_status[ops_index - 1].volume = MAX_VOLUME;
						
						update_fm_op_volume(mus, track_status, chn, cydchn, track_status->ops_status[ops_index - 1].volume, ops_index - 1);
					}
					
					break;
				}
			}
		}
		break;
		
		case MUS_FX_FM_FADE_VOLUME: //wasn't there
		{
			if(ops_index == 0 || ops_index == 0xFF)
			{
				if (!(chn->flags & MUS_CHN_DISABLED))
				{
					cydchn->fm.adsr.volume -= inst & 0xf;
					if (track_status->volume > MAX_VOLUME) cydchn->fm.adsr.volume = 0;
					cydchn->fm.adsr.volume += (inst >> 4) & 0xf;
					if (track_status->volume > MAX_VOLUME) cydchn->fm.adsr.volume = MAX_VOLUME;

					update_volumes(mus, track_status, chn, cydchn, track_status->volume);
				}
			}
		}
		break;
#ifdef STEREOOUTPUT
		case MUS_FX_PAN_RIGHT:
		case MUS_FX_PAN_LEFT:
		{
			if(ops_index == 0 || ops_index == 0xFF)
			{
				int p = cydchn->panning;
				
				if ((inst & 0xff00) == MUS_FX_PAN_LEFT)
				{
					p -= inst & 0x00ff;
				}
				
				else
				{
					p += inst & 0x00ff;
				}

				p = my_min(CYD_PAN_RIGHT, my_max(CYD_PAN_LEFT, p));
				
				cydchn->init_panning = p;

				cyd_set_panning(mus->cyd, cydchn, p);
			}
		}
		break;
#endif
		case MUS_FX_EXT:
		{
			// Protracker style Exy commands

			switch (inst & 0xfff0)
			{
				case MUS_FX_WAVETABLE_OFFSET_UP_FINE:
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							if (cydchn->wave_entry)
							{
								if ((cydchn->wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->subosc[0].wave.start_offset + (cydchn->subosc[0].wave.start_offset + (inst & 0xf) > 0xffff ? 0xffff : (inst & 0xf)) < cydchn->subosc[0].wave.end_offset))
								{
									for (int s = 0; s < CYD_SUB_OSCS; ++s)
									{
										cydchn->subosc[s].wave.start_offset += (cydchn->subosc[s].wave.start_offset + (inst & 0xf) > 0xffff ? 0xffff : (inst & 0xf));
										
										cydchn->subosc[s].wave.start_point_track_status = (Uint64)cydchn->subosc[s].wave.start_offset * WAVETABLE_RESOLUTION * cydchn->wave_entry->samples / 0x10000;
										
										cydchn->subosc[s].wave.use_start_track_status_offset = true;
									}	
								}
							}
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									if (cydchn->fm.ops[i].wave_entry)
									{
										for (int s = 0; s < CYD_SUB_OSCS; ++s)
										{
											if ((cydchn->fm.ops[i].wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->fm.ops[i].subosc[s].wave.start_offset + (cydchn->fm.ops[i].subosc[s].wave.start_offset + (inst & 0xf) > 0xffff ? 0xffff : (inst & 0xf)) < cydchn->fm.ops[i].subosc[s].wave.end_offset))
											{
												cydchn->fm.ops[i].subosc[s].wave.start_offset += (cydchn->fm.ops[i].subosc[s].wave.start_offset + (inst & 0xf) > 0xffff ? 0xffff : (inst & 0xf));
												
												cydchn->fm.ops[i].subosc[s].wave.start_point_track_status = (Uint64)cydchn->fm.ops[i].subosc[s].wave.start_offset * WAVETABLE_RESOLUTION * cydchn->fm.ops[i].wave_entry->samples / 0x10000;
												
												cydchn->fm.ops[i].subosc[s].wave.use_start_track_status_offset = true;
											}
										}
									}
								}
							}
							
							break;
						}
						
						default:
						{
							if (cydchn->fm.ops[ops_index - 1].wave_entry)
							{
								for (int s = 0; s < CYD_SUB_OSCS; ++s)
								{
									if ((cydchn->fm.ops[ops_index - 1].wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_offset + (cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_offset + (inst & 0xf) > 0xffff ? 0xffff : (inst & 0xf)) < cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_offset))
									{
										cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_offset += (cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_offset + (inst & 0xf) > 0xffff ? 0xffff : (inst & 0xf));
										
										cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_point_track_status = (Uint64)cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_offset * WAVETABLE_RESOLUTION * cydchn->fm.ops[ops_index - 1].wave_entry->samples / 0x10000;
										
										cydchn->fm.ops[ops_index - 1].subosc[s].wave.use_start_track_status_offset = true;
									}
								}
							}
						}
					}
				}
				break;
				
				case MUS_FX_WAVETABLE_OFFSET_DOWN_FINE:
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							if (cydchn->wave_entry)
							{
								if ((cydchn->wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->subosc[0].wave.start_offset - ((Sint32)cydchn->subosc[0].wave.start_offset - (inst & 0xf) < 0 ? 0 : (inst & 0xf)) < cydchn->subosc[0].wave.end_offset))
								{
									for (int s = 0; s < CYD_SUB_OSCS; ++s)
									{
										cydchn->subosc[s].wave.start_offset -= ((Sint32)cydchn->subosc[s].wave.start_offset - (inst & 0xf) < 0 ? 0 : (inst & 0xf));
										
										cydchn->subosc[s].wave.start_point_track_status = (Uint64)cydchn->subosc[s].wave.start_offset * WAVETABLE_RESOLUTION * cydchn->wave_entry->samples / 0x10000;
										
										cydchn->subosc[s].wave.use_start_track_status_offset = true;
									}	
								}
							}
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									if (cydchn->fm.ops[i].wave_entry)
									{
										for (int s = 0; s < CYD_SUB_OSCS; ++s)
										{
											if ((cydchn->fm.ops[i].wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->fm.ops[i].subosc[s].wave.start_offset - ((Sint32)cydchn->fm.ops[i].subosc[s].wave.start_offset - (inst & 0xf) < 0 ? 0 : (inst & 0xf)) < cydchn->fm.ops[i].subosc[s].wave.end_offset))
											{
												cydchn->fm.ops[i].subosc[s].wave.start_offset -= ((Sint32)cydchn->fm.ops[i].subosc[s].wave.start_offset - (inst & 0xf) < 0 ? 0 : (inst & 0xf));
												
												cydchn->fm.ops[i].subosc[s].wave.start_point_track_status = (Uint64)cydchn->fm.ops[i].subosc[s].wave.start_offset * WAVETABLE_RESOLUTION * cydchn->fm.ops[i].wave_entry->samples / 0x10000;
												
												cydchn->fm.ops[i].subosc[s].wave.use_start_track_status_offset = true;
											}
										}
									}
								}
							}
							
							break;
						}
						
						default:
						{
							if (cydchn->fm.ops[ops_index - 1].wave_entry)
							{
								for (int s = 0; s < CYD_SUB_OSCS; ++s)
								{
									if ((cydchn->fm.ops[ops_index - 1].wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_offset - ((Sint32)cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_offset - (inst & 0xf) < 0 ? 0 : (inst & 0xf)) < cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_offset))
									{
										cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_offset -= ((Sint32)cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_offset - (inst & 0xf) < 0 ? 0 : (inst & 0xf));
										
										cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_point_track_status = (Uint64)cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_offset * WAVETABLE_RESOLUTION * cydchn->fm.ops[ops_index - 1].wave_entry->samples / 0x10000;
										
										cydchn->fm.ops[ops_index - 1].subosc[s].wave.use_start_track_status_offset = true;
									}
								}
							}
							
							break;
						}
					}
				}
				break;
				
				case MUS_FX_WAVETABLE_END_POINT_UP_FINE:
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							if (cydchn->wave_entry)
							{
								if ((cydchn->wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->subosc[0].wave.end_offset - ((Sint32)cydchn->subosc[0].wave.end_offset - (inst & 0xf) < 0 ? 0 : (inst & 0xf)) > cydchn->subosc[0].wave.start_offset))
								{
									for (int s = 0; s < CYD_SUB_OSCS; ++s)
									{
										cydchn->subosc[s].wave.end_offset -= ((Sint32)cydchn->subosc[s].wave.end_offset - (inst & 0xf) < 0 ? 0 : (inst & 0xf));
										
										cydchn->subosc[s].wave.end_point_track_status = (Uint64)cydchn->subosc[s].wave.end_offset * WAVETABLE_RESOLUTION * cydchn->wave_entry->samples / 0x10000;
										
										cydchn->subosc[s].wave.use_end_track_status_offset = true;
									}	
								}
							}
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									if (cydchn->fm.ops[i].wave_entry)
									{
										for (int s = 0; s < CYD_SUB_OSCS; ++s)
										{
											if ((cydchn->fm.ops[i].wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->fm.ops[i].subosc[s].wave.end_offset - ((Sint32)cydchn->fm.ops[i].subosc[s].wave.end_offset - (inst & 0xf) < 0 ? 0 : (inst & 0xf)) > cydchn->fm.ops[i].subosc[s].wave.start_offset))
											{
												cydchn->fm.ops[i].subosc[s].wave.end_offset -= ((Sint32)cydchn->fm.ops[i].subosc[s].wave.end_offset - (inst & 0xf) < 0 ? 0 : (inst & 0xf));
												
												cydchn->fm.ops[i].subosc[s].wave.end_point_track_status = (Uint64)cydchn->fm.ops[i].subosc[s].wave.end_offset * WAVETABLE_RESOLUTION * cydchn->fm.ops[i].wave_entry->samples / 0x10000;
												
												cydchn->fm.ops[i].subosc[s].wave.use_end_track_status_offset = true;
											}
										}
									}
								}
							}
							
							break;
						}
						
						default:
						{
							if (cydchn->fm.ops[ops_index - 1].wave_entry)
							{
								for (int s = 0; s < CYD_SUB_OSCS; ++s)
								{
									if ((cydchn->fm.ops[ops_index - 1].wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_offset - ((Sint32)cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_offset - (inst & 0xf) < 0 ? 0 : (inst & 0xf)) > cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_offset))
									{
										cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_offset -= ((Sint32)cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_offset - (inst & 0xf) < 0 ? 0 : (inst & 0xf));
										
										cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_point_track_status = (Uint64)cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_offset * WAVETABLE_RESOLUTION * cydchn->fm.ops[ops_index - 1].wave_entry->samples / 0x10000;
										
										cydchn->fm.ops[ops_index - 1].subosc[s].wave.use_end_track_status_offset = true;
									}
								}
							}
							
							break;
						}
					}
				}
				break;
				
				case MUS_FX_WAVETABLE_END_POINT_DOWN_FINE:
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							if (cydchn->wave_entry)
							{
								if ((cydchn->wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->subosc[0].wave.end_offset + (cydchn->subosc[0].wave.end_offset + (inst & 0xf) > 0xffff ? 0xffff : (inst & 0xf)) > cydchn->subosc[0].wave.start_offset))
								{
									for (int s = 0; s < CYD_SUB_OSCS; ++s)
									{
										cydchn->subosc[s].wave.end_offset += (cydchn->subosc[s].wave.end_offset + (inst & 0xf) > 0xffff ? 0xffff : (inst & 0xf));
										
										cydchn->subosc[s].wave.end_point_track_status = (Uint64)cydchn->subosc[s].wave.end_offset * WAVETABLE_RESOLUTION * cydchn->wave_entry->samples / 0x10000;
										
										cydchn->subosc[s].wave.use_end_track_status_offset = true;
									}	
								}
							}
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									if (cydchn->fm.ops[i].wave_entry)
									{
										for (int s = 0; s < CYD_SUB_OSCS; ++s)
										{
											if ((cydchn->fm.ops[i].wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->fm.ops[i].subosc[s].wave.end_offset + (cydchn->fm.ops[i].subosc[s].wave.end_offset + (inst & 0xf) > 0xffff ? 0xffff : (inst & 0xf)) > cydchn->fm.ops[i].subosc[s].wave.start_offset))
											{
												cydchn->fm.ops[i].subosc[s].wave.end_offset += (cydchn->fm.ops[i].subosc[s].wave.end_offset + (inst & 0xf) > 0xffff ? 0xffff : (inst & 0xf));
												
												cydchn->fm.ops[i].subosc[s].wave.end_point_track_status = (Uint64)cydchn->fm.ops[i].subosc[s].wave.end_offset * WAVETABLE_RESOLUTION * cydchn->fm.ops[i].wave_entry->samples / 0x10000;
												
												cydchn->fm.ops[i].subosc[s].wave.use_end_track_status_offset = true;
											}
										}
									}
								}
							}
							
							break;
						}
						
						default:
						{
							if (cydchn->fm.ops[ops_index - 1].wave_entry)
							{
								for (int s = 0; s < CYD_SUB_OSCS; ++s)
								{
									if ((cydchn->fm.ops[ops_index - 1].wave_entry->flags & CYD_WAVE_LOOP) && (cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_offset + (cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_offset + (inst & 0xf) > 0xffff ? 0xffff : (inst & 0xf)) > cydchn->fm.ops[ops_index - 1].subosc[s].wave.start_offset))
									{
										cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_offset += (cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_offset + (inst & 0xf) > 0xffff ? 0xffff : (inst & 0xf));
										
										cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_point_track_status = (Uint64)cydchn->fm.ops[ops_index - 1].subosc[s].wave.end_offset * WAVETABLE_RESOLUTION * cydchn->fm.ops[ops_index - 1].wave_entry->samples / 0x10000;
										
										cydchn->fm.ops[ops_index - 1].subosc[s].wave.use_end_track_status_offset = true;
									}
								}
							}
							
							break;
						}
					}
				}
				break;
				
				case MUS_FX_FILTER_SLOPE: //wasn't there
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							//track_status->filter_slope = inst & 0xf;
							cydchn->flt_slope = inst & 0xf;
							track_status->filter_slope = inst & 0xf;
							
							cyd_set_filter_coeffs(mus->cyd, cydchn, track_status->filter_cutoff, track_status->filter_resonance);
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									cydchn->fm.ops[i].flt_slope = inst & 0xf;
									track_status->ops_status[i].filter_slope = inst & 0xf;
									
									for(int j = 0; j < (int)pow(2, cydchn->fm.ops[i].flt_slope); ++j)
									{
										for (int sub = 0; sub < CYD_SUB_OSCS; ++sub)
										{
											if(mus->cyd->flags & CYD_USE_OLD_FILTER)
											{
												cydflt_set_coeff_old(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, resonance_table[track_status->ops_status[i].filter_resonance & 3]);
											}
											
											else
											{
												cydflt_set_coeff(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, track_status->ops_status[i].filter_resonance, mus->cyd->sample_rate);
											}
										}
									}
								}
							}
							
							break;
						}
						
						default:
						{
							cydchn->fm.ops[ops_index - 1].flt_slope = inst & 0xf;
							track_status->ops_status[ops_index - 1].filter_slope = inst & 0xf;
							
							for(int j = 0; j < (int)pow(2, cydchn->fm.ops[ops_index - 1].flt_slope); ++j)
							{
								for (int sub = 0; sub < CYD_SUB_OSCS; ++sub)
								{
									if(mus->cyd->flags & CYD_USE_OLD_FILTER)
									{
										cydflt_set_coeff_old(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, resonance_table[track_status->ops_status[ops_index - 1].filter_resonance & 3]);
									}
									
									else
									{
										cydflt_set_coeff(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, track_status->ops_status[ops_index - 1].filter_resonance, mus->cyd->sample_rate);
									}
								}
							}
							
							break;
						}
					}
				}
				break;
				
				case MUS_FX_EXT_SET_NOISE_MODE: //wasn't there
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							bool fixed_pitch_bit = (inst & 0b100); //setting fixed pitch noise mode
							cydchn->flags &= ~(1 << 29);
							cydchn->flags |= ((fixed_pitch_bit ? 1 : 0) << 29);
							
							bool metal_bit = (inst & 0b10); //setting metal noise mode
							cydchn->flags &= ~(1 << 7);
							cydchn->flags |= ((metal_bit ? 1 : 0) << 7);
							
							bool one_bit = (inst & 0b1); //setting 1-bit noise mode
							cydchn->flags &= ~(1 << 30);
							cydchn->flags |= ((one_bit ? 1 : 0) << 30);
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									cydchn->fm.ops[i].flags &= ~(1 << 14); //ok
									cydchn->fm.ops[i].flags |= ((fixed_pitch_bit ? 1 : 0) << 14);
									
									cydchn->fm.ops[i].flags &= ~(1 << 7); //ok
									cydchn->fm.ops[i].flags |= ((metal_bit ? 1 : 0) << 7);
									
									cydchn->fm.ops[i].flags &= ~(1 << 30); //ok
									cydchn->fm.ops[i].flags |= ((one_bit ? 1 : 0) << 30);
								}
							}
							
							break;
						}
						
						default:
						{
							bool fixed_pitch_bit = (inst & 0b100); //setting fixed pitch noise mode
							bool metal_bit = (inst & 0b10); //setting metal noise mode
							bool one_bit = (inst & 0b1); //setting 1-bit noise mode
							
							cydchn->fm.ops[ops_index - 1].flags &= ~(1 << 14); //ok
							cydchn->fm.ops[ops_index - 1].flags |= ((fixed_pitch_bit ? 1 : 0) << 14);
							
							cydchn->fm.ops[ops_index - 1].flags &= ~(1 << 7); //ok
							cydchn->fm.ops[ops_index - 1].flags |= ((metal_bit ? 1 : 0) << 7);
							
							cydchn->fm.ops[ops_index - 1].flags &= ~(1 << 30); //ok
							cydchn->fm.ops[ops_index - 1].flags |= ((one_bit ? 1 : 0) << 30);
							
							break;
						}
					}
				}
				break;
				
				case MUS_FX_EXT_NOTE_CUT:
				{
					if ((inst & 0xf) <= tick)
					{
						switch(ops_index)
						{
							case 0:
							case 0xFF:
							{
								if (!(chn->flags & MUS_CHN_DISABLED))
								{
									cydchn->adsr.volume = 0;
									track_status->volume = 0;
								}
								
								if(ops_index == 0xFF)
								{
									for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
									{
										if (!(chn->ops[i].flags & MUS_FM_OP_DISABLED))
										{
											cydchn->fm.ops[i].adsr.volume = 0;
											track_status->ops_status[i].volume = 0;
										}
									}
								}
								
								break;
							}
							
							default:
							{
								if (!(chn->ops[ops_index - 1].flags & MUS_FM_OP_DISABLED))
								{
									cydchn->fm.ops[ops_index - 1].adsr.volume = 0;
									track_status->ops_status[ops_index - 1].volume = 0;
								}
								
								break;
							}
						}
					}
				}
				break;

				case MUS_FX_EXT_RETRIGGER:
				{
					if ((inst & 0xf) > 0 && (tick % (inst & 0xf)) == 0)
					{
						switch(ops_index)
						{
							case 0:
							case 0xFF:
							{
								Uint8 prev_vol_tr = track_status->volume;
								Uint8 prev_vol_cyd = cydchn->adsr.volume;
								
								mus_trigger_instrument_internal(mus, chan, chn->instrument, chn->last_note, -1, false);
								
								track_status->volume = prev_vol_tr;
								cydchn->adsr.volume = prev_vol_cyd;
								
								if(ops_index == 0xFF)
								{
									cydchn->fm.alg = chn->instrument->alg;
									
									track_status->fm_4op_vol = chn->instrument->fm_4op_vol;
									cydchn->fm.fm_4op_vol = chn->instrument->fm_4op_vol;
									
									for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
									{
										Uint8 prev_vol_tr1 = track_status->ops_status[i].volume;
										Uint8 prev_vol_cyd1 = cydchn->fm.ops[i].adsr.volume;
										
										mus_trigger_fm_op_internal(&cydchn->fm, chn->instrument, cydchn, chn, track_status, mus, i, chn->ops[i].last_note, chan, 0, false);
										
										track_status->ops_status[i].volume = prev_vol_tr1;
										cydchn->fm.ops[i].adsr.volume = prev_vol_cyd1;
										
										cydchn->fm.ops[i].adsr.envelope_state = ATTACK;
										
										if(!(cydchn->fm.ops[i].adsr.use_volume_envelope))
										{
											cydchn->fm.ops[i].adsr.envelope = 0x0;
										
											cydchn->fm.ops[i].adsr.env_speed = envspd(cyd, cydchn->fm.ops[i].adsr.a);
											
											if(cydchn->fm.ops[i].env_ksl_mult != 0.0 && cydchn->fm.ops[i].env_ksl_mult != 1.0)
											{
												cydchn->fm.ops[i].adsr.env_speed = (int)((double)envspd(cyd, cydchn->fm.ops[i].adsr.a) * cydchn->fm.ops[i].env_ksl_mult);
											}
										}
										
										//cyd_cycle_adsr(cyd, 0, 0, &cydchn->fm.ops[i].adsr, cydchn->fm.ops[i].env_ksl_mult);
										cyd_cycle_fm_op_adsr(cyd, 0, 0, &cydchn->fm.ops[i].adsr, cydchn->fm.ops[i].env_ksl_mult, cydchn->fm.ops[i].ssg_eg_type | (((cydchn->fm.ops[i].flags & CYD_FM_OP_ENABLE_SSG_EG) ? 1 : 0) << 3));
										
										for (int s = 0; s < CYD_SUB_OSCS; ++s)
										{
											cydchn->fm.ops[i].subosc[s].accumulator = 0;
											cydchn->fm.ops[i].subosc[s].noise_accumulator = 0;
											cydchn->fm.ops[i].subosc[s].wave.acc = 0;
										}
										
										cydchn->fm.ops[i].flags |= CYD_FM_OP_ENABLE_GATE;
									}
								}
								
								break;
							}
							
							default:
							{
								Uint8 prev_vol_tr1 = track_status->ops_status[ops_index - 1].volume;
								Uint8 prev_vol_cyd1 = cydchn->fm.ops[ops_index - 1].adsr.volume;
								
								mus_trigger_fm_op_internal(&cydchn->fm, chn->instrument, cydchn, chn, track_status, mus, ops_index - 1, chn->ops[ops_index - 1].last_note, chan, 0, false);
								
								track_status->ops_status[ops_index - 1].volume = prev_vol_tr1;
								cydchn->fm.ops[ops_index - 1].adsr.volume = prev_vol_cyd1;
								
								cydchn->fm.ops[ops_index - 1].adsr.envelope_state = ATTACK;
								
								if(!(cydchn->fm.ops[ops_index - 1].adsr.use_volume_envelope))
								{
									cydchn->fm.ops[ops_index - 1].adsr.envelope = 0x0;
									
									cydchn->fm.ops[ops_index - 1].adsr.env_speed = envspd(cyd, cydchn->fm.ops[ops_index - 1].adsr.a);
									
									if(cydchn->fm.ops[ops_index - 1].env_ksl_mult != 0.0 && cydchn->fm.ops[ops_index - 1].env_ksl_mult != 1.0)
									{
										cydchn->fm.ops[ops_index - 1].adsr.env_speed = (int)((double)envspd(cyd, cydchn->fm.ops[ops_index - 1].adsr.a) * cydchn->fm.ops[ops_index - 1].env_ksl_mult);
									}
								}
								
								//cyd_cycle_adsr(cyd, 0, 0, &cydchn->fm.ops[ops_index - 1].adsr, cydchn->fm.ops[ops_index - 1].env_ksl_mult);
								cyd_cycle_fm_op_adsr(cyd, 0, 0, &cydchn->fm.ops[ops_index - 1].adsr, cydchn->fm.ops[ops_index - 1].env_ksl_mult, cydchn->fm.ops[ops_index - 1].ssg_eg_type | (((cydchn->fm.ops[ops_index - 1].flags & CYD_FM_OP_ENABLE_SSG_EG) ? 1 : 0) << 3));
								
								for (int s = 0; s < CYD_SUB_OSCS; ++s)
								{
									cydchn->fm.ops[ops_index - 1].subosc[s].accumulator = 0;
									cydchn->fm.ops[ops_index - 1].subosc[s].noise_accumulator = 0;
									cydchn->fm.ops[ops_index - 1].subosc[s].wave.acc = 0;
								}
								
								cydchn->fm.ops[ops_index - 1].flags |= CYD_FM_OP_ENABLE_GATE;
								
								break;
							}
						}
					}
				}
				break;
				
				case MUS_FX_EXT_OSC_MIX: //wasn't there
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							cydchn->mixmode = (inst & 0xf);
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									cydchn->fm.ops[i].mixmode = (inst & 0xf);
								}
							}
							
							break;
						}
						
						default:
						{
							cydchn->fm.ops[ops_index - 1].mixmode = (inst & 0xf);
							
							break;
						}
					}
				}
				break;
			}
		}
		break;
	}

	if (tick == 0 || chn->prog_period[prog_number] < 2)
	{
		// --- commands that run only on tick 0
		
		switch (inst & 0xfff0)
		{
			case MUS_FX_EXT_PORTA_UP:
			{
				switch(ops_index)
				{
					case 0:
					case 0xFF:
					{
						Uint32 prev = chn->note;
						chn->note += ((inst & 0xf) << 2);
						if (prev > chn->note) chn->note = 0xffff;

						mus_set_slide(mus, chan, chn->note);
						
						if(ops_index == 0xFF)
						{
							for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
							{
								Uint32 prev = chn->ops[i].note;
								chn->ops[i].note += ((inst & 0xf) << 2);
								if (prev > chn->ops[i].note) chn->ops[i].note = 0xffff;
								
								chn->ops[i].target_note = chn->ops[i].note;
							}
						}
						
						break;
					}
					
					default:
					{
						Uint32 prev = chn->ops[ops_index - 1].note;
						chn->ops[ops_index - 1].note += ((inst & 0xf) << 2);
						if (prev > chn->ops[ops_index - 1].note) chn->ops[ops_index - 1].note = 0xffff;
						
						chn->ops[ops_index - 1].target_note = chn->ops[ops_index - 1].note;
						
						break;
					}
				}
			}
			break;

			case MUS_FX_EXT_PORTA_DN:
			{
				switch(ops_index)
				{
					case 0:
					case 0xFF:
					{
						Uint32 prev = chn->note;
						chn->note -= ((inst & 0xf) << 2);
						if (prev < chn->note) chn->note = 0x0;

						mus_set_slide(mus, chan, chn->note);
						
						if(ops_index == 0xFF)
						{
							for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
							{
								Uint32 prev = chn->ops[i].note;
								chn->ops[i].note -= ((inst & 0xf) << 2);
								if (prev < chn->ops[i].note) chn->ops[i].note = 0x0;
								
								chn->ops[i].target_note = chn->ops[i].note;
							}
						}
						
						break;
					}
					
					default:
					{
						Uint32 prev = chn->ops[ops_index - 1].note;
						chn->ops[ops_index - 1].note -= ((inst & 0xf) << 2);
						if (prev < chn->ops[ops_index - 1].note) chn->ops[ops_index - 1].note = 0x0;
						
						chn->ops[ops_index - 1].target_note = chn->ops[ops_index - 1].note;
						
						break;
					}
				}
			}
			break;
		}
		
		switch (inst & 0xff00)
		{
			case MUS_FX_EXT:
			{
				// Protracker style Exy commands

				switch (inst & 0xfff0)
				{
					case MUS_FX_EXT_SINE_ACC_SHIFT:
					{
						switch(ops_index)
						{
							case 0:
							case 0xFF:
							{
								cydchn->sine_acc_shift = inst & 0xF;
								
								if(ops_index == 0xFF)
								{
									for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
									{
										cydchn->fm.ops[i].sine_acc_shift = inst & 0xF;
									}
								}
								
								break;
							}
							
							default:
							{
								cydchn->fm.ops[ops_index - 1].sine_acc_shift = inst & 0xF;
								
								break;
							}
						}
					}
					break;
					
					case MUS_FX_EXT_FADE_VOLUME_DN:
					{
						switch(ops_index)
						{
							case 0:
							case 0xFF:
							{
								if (!(chn->flags & MUS_CHN_DISABLED))
								{
									track_status->volume -= inst & 0xf;
									if (track_status->volume > MAX_VOLUME) track_status->volume = 0;

									update_volumes(mus, track_status, chn, cydchn, track_status->volume);
								}
								
								if(ops_index == 0xFF)
								{
									if(ops_index == 0xFF)
									{
										if((int)track_status->fm_4op_vol + (int)(inst & 0xf) < 0)
										{
											track_status->fm_4op_vol = 0;
										}
										
										else
										{
											track_status->fm_4op_vol -= (inst & 0xf);
										}
									}
								}
								
								break;
							}
							
							default:
							{
								if (!(chn->ops[ops_index - 1].flags & MUS_FM_OP_DISABLED))
								{
									track_status->ops_status[ops_index - 1].volume -= inst & 0xf;
									if (track_status->ops_status[ops_index - 1].volume > MAX_VOLUME) track_status->ops_status[ops_index - 1].volume = 0;
									
									update_fm_op_volume(mus, track_status, chn, cydchn, track_status->ops_status[ops_index - 1].volume, ops_index - 1);
								}
								
								break;
							}
						}
					}
					break;

					case MUS_FX_EXT_FADE_VOLUME_UP:
					{
						switch(ops_index)
						{
							case 0:
							case 0xFF:
							{
								if (!(chn->flags & MUS_CHN_DISABLED))
								{
									track_status->volume += inst & 0xf;
									if (track_status->volume > MAX_VOLUME) track_status->volume = MAX_VOLUME;

									update_volumes(mus, track_status, chn, cydchn, track_status->volume);
								}
								
								if(ops_index == 0xFF)
								{
									if(track_status->fm_4op_vol + (inst & 0xf) > 0xff)
									{
										track_status->fm_4op_vol = 0xff;
									}
									
									else
									{
										track_status->fm_4op_vol += (inst & 0xf);
									}
								}
								
								break;
							}
							
							default:
							{
								if (!(chn->ops[ops_index - 1].flags & MUS_FM_OP_DISABLED))
								{
									track_status->ops_status[ops_index - 1].volume += inst & 0xf;
									if (track_status->ops_status[ops_index - 1].volume > MAX_VOLUME) track_status->ops_status[ops_index - 1].volume = MAX_VOLUME;
									
									update_fm_op_volume(mus, track_status, chn, cydchn, track_status->ops_status[ops_index - 1].volume, ops_index - 1);
								}
								
								break;
							}
						}
					}
					break;
					
					case MUS_FX_EXT_FINE_PORTA_UP:
					{
						switch(ops_index)
						{
							case 0:
							case 0xFF:
							{
								Uint32 prev = chn->note;
								chn->note += (inst & 0xf);
								if (prev > chn->note) chn->note = 0xffff;

								mus_set_slide(mus, chan, chn->note);
								
								if(ops_index == 0xFF)
								{
									for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
									{
										Uint32 prev = chn->ops[i].note;
										chn->ops[i].note += (inst & 0xf);
										if (prev > chn->ops[i].note) chn->ops[i].note = 0xffff;
										
										chn->ops[i].target_note = chn->ops[i].note;
									}
								}
								
								break;
							}
							
							default:
							{
								Uint32 prev = chn->ops[ops_index - 1].note;
								chn->ops[ops_index - 1].note += (inst & 0xf);
								if (prev > chn->ops[ops_index - 1].note) chn->ops[ops_index - 1].note = 0xffff;
								
								chn->ops[ops_index - 1].target_note = chn->ops[ops_index - 1].note;
								
								break;
							}
						}
					}
					break;

					case MUS_FX_EXT_FINE_PORTA_DN:
					{
						switch(ops_index)
						{
							case 0:
							case 0xFF:
							{
								Uint32 prev = chn->note;
								chn->note -= (inst & 0xf);
								if (prev < chn->note) chn->note = 0x0;

								mus_set_slide(mus, chan, chn->note);
								
								if(ops_index == 0xFF)
								{
									for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
									{
										Uint32 prev = chn->ops[i].note;
										chn->ops[i].note -= (inst & 0xf);
										if (prev < chn->ops[i].note) chn->ops[i].note = 0x0;
										
										chn->ops[i].target_note = chn->ops[i].note;
									}
								}
								
								break;
							}
							
							default:
							{
								Uint32 prev = chn->ops[ops_index - 1].note;
								chn->ops[ops_index - 1].note -= (inst & 0xf);
								if (prev < chn->ops[ops_index - 1].note) chn->ops[ops_index - 1].note = 0x0;
								
								chn->ops[ops_index - 1].target_note = chn->ops[ops_index - 1].note;
								
								break;
							}
						}
					}
					break;
				}
			}
			break;

			default:

			switch (inst & 0xf000)
			{
#ifndef CYD_DISABLE_FILTER
				case MUS_FX_CUTOFF_FINE_SET:
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							track_status->filter_cutoff = (inst & 0xfff);
							cyd_set_filter_coeffs(mus->cyd, cydchn, track_status->filter_cutoff, track_status->filter_resonance);
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									track_status->ops_status[i].filter_cutoff = (inst & 0xfff);
									
									for(int j = 0; j < (int)pow(2, cydchn->fm.ops[i].flt_slope); ++j)
									{
										for (int sub = 0; sub < CYD_SUB_OSCS; ++sub)
										{
											if(mus->cyd->flags & CYD_USE_OLD_FILTER)
											{
												cydflt_set_coeff_old(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, resonance_table[track_status->ops_status[i].filter_resonance & 3]);
											}
											
											else
											{
												cydflt_set_coeff(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, track_status->ops_status[i].filter_resonance, mus->cyd->sample_rate);
											}
										}
									}
								}
							}
							
							break;
						}
						
						default:
						{
							track_status->ops_status[ops_index - 1].filter_cutoff = (inst & 0xfff);
							
							for(int j = 0; j < (int)pow(2, cydchn->fm.ops[ops_index - 1].flt_slope); ++j)
							{
								for (int sub = 0; sub < CYD_SUB_OSCS; ++sub)
								{
									if(mus->cyd->flags & CYD_USE_OLD_FILTER)
									{
										cydflt_set_coeff_old(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, resonance_table[track_status->ops_status[ops_index - 1].filter_resonance & 3]);
									}
									
									else
									{
										cydflt_set_coeff(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, track_status->ops_status[ops_index - 1].filter_resonance, mus->cyd->sample_rate);
									}
								}
							}
							
							break;
						}
					}
				}
				break;
#endif

#ifndef CYD_DISABLE_WAVETABLE
				case MUS_FX_WAVETABLE_OFFSET:
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							if(mus->song)
							{
								if((mus->song->flags & MUS_USE_OLD_SAMPLE_LOOP_BEHAVIOUR) && cydchn->wave_entry)
								{
									for (int s = 0; s < CYD_SUB_OSCS; ++s)
									{
										cydchn->subosc[s].wave.acc = (Uint64)(inst & 0xfff) * WAVETABLE_RESOLUTION * cydchn->wave_entry->samples / 0x1000;
									}
								}
								
								else
								{
									cyd_set_wavetable_offset(cydchn, inst & 0xfff);
								}
							}
							
							else
							{
								cyd_set_wavetable_offset(cydchn, inst & 0xfff);
							}
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									cyd_set_fm_op_wavetable_offset(cydchn, inst & 0xfff, i);
								}
							}
							
							break;
						}
						
						default:
						{
							cyd_set_fm_op_wavetable_offset(cydchn, inst & 0xfff, ops_index - 1);
							
							break;
						}
					}
				}
				break;
				
				case MUS_FX_WAVETABLE_END_POINT: //wasn't there
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							cyd_set_wavetable_end_offset(cydchn, inst & 0xfff);
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									cyd_set_fm_op_wavetable_end_offset(cydchn, inst & 0xfff, i);
								}
							}
							
							break;
						}
						
						default:
						{
							cyd_set_fm_op_wavetable_end_offset(cydchn, inst & 0xfff, ops_index - 1);
							
							break;
						}
					}
				}
				break;
#endif
				
				case MUS_FX_PW_FINE_SET: //wasn't there
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							track_status->pw = inst & 0xfff;
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									track_status->ops_status[i].pw = inst & 0xfff;
								}
							}
							
							break;
						}
						
						default:
						{
							track_status->ops_status[ops_index - 1].pw = inst & 0xfff;
							
							break;
						}
					}
				}
				break;

			}

			switch (inst & 0xff00)
			{
				/*case MUS_FX_SET_FREQUENCY_LOWER_BYTE:
				{
					debug("ff");
					
					if(ops_index == 0xFF || ops_index == 0)
					{
						debug("freq before %d", cydchn->subosc[0].frequency);
						
						Uint32 frequency = (Uint64)cydchn->subosc[0].frequency / (Uint64)(ACC_LENGTH >> (cyd->oversample)) * (Uint64)1024 * (Uint64)cyd->sample_rate;
						frequency = (frequency & 0xffff00) | (inst & 0xff);
						
						for(int s = 0; s < CYD_SUB_OSCS; ++s)
						{
							//cyd_set_frequency(cyd, cydchn, s, frequency);
							cydchn->subosc[s].frequency = (Uint64)(ACC_LENGTH >> (cyd->oversample)) / (Uint64)1024 * (Uint64)(frequency) / (Uint64)cyd->sample_rate;
							
							//chn->subosc[s].frequency = (Uint64)(ACC_LENGTH >> (cyd->oversample)) / (Uint64)1024 * (Uint64)(frequency) / (Uint64)cyd->sample_rate;
							
							if(cydchn->flags & CYD_CHN_ENABLE_WAVE)
							{
								cyd_set_wavetable_frequency(cyd, cydchn, s, frequency);
							}
						}
						
						debug("freq after %d", cydchn->subosc[0].frequency);
					}
					
					else
					{
						Uint32 frequency = (Uint64)cydchn->fm.ops[ops_index - 1].subosc[0].frequency / (ACC_LENGTH >> (cyd->oversample)) * (Uint64)1024 * (Uint64)cyd->sample_rate;
						frequency = (frequency & 0xffff00) | (inst & 0xff);
						
						for(int s = 0; s < CYD_SUB_OSCS; ++s)
						{
							cydchn->fm.ops[ops_index - 1].subosc[s].frequency = (Uint64)(ACC_LENGTH >> (cyd->oversample)) / (Uint64)1024 * (Uint64)(frequency) / (Uint64)cyd->sample_rate;
							
							if((cydchn->fm.ops[ops_index - 1].flags & CYD_FM_OP_ENABLE_WAVE) && cydchn->fm.ops[ops_index - 1].wave_entry && chn->instrument)
							{
								if(cydchn->fm.fm_freq_LUT == 0)
								{
									cydchn->fm.ops[ops_index - 1].subosc[s].wave.frequency = (Uint64)WAVETABLE_RESOLUTION * (Uint64)cydchn->fm.ops[ops_index - 1].wave_entry->sample_rate / (Uint64)mus->cyd->sample_rate * (Uint64)frequency / (Uint64)get_freq(cydchn->fm.ops[ops_index - 1].wave_entry->base_note) * (Uint64)harmonic1[cydchn->fm.ops[ops_index - 1].harmonic & 15] / (Uint64)harmonic1[cydchn->fm.ops[ops_index - 1].harmonic >> 4] / (((chn->instrument->fm_flags & CYD_FM_ENABLE_3CH_EXP_MODE) ? (chn->instrument->ops[ops_index - 1].flags & MUS_FM_OP_QUARTER_FREQ) : (chn->instrument->flags & MUS_INST_QUARTER_FREQ)) ? 4 : 1);
								}
								
								else
								{
									cydchn->fm.ops[ops_index - 1].subosc[s].wave.frequency = (Uint64)WAVETABLE_RESOLUTION * (Uint64)cydchn->fm.ops[ops_index - 1].wave_entry->sample_rate / (Uint64)mus->cyd->sample_rate * (Uint64)frequency / (Uint64)get_freq(cydchn->fm.ops[ops_index - 1].wave_entry->base_note) * (Uint64)harmonicOPN1[cydchn->fm.ops[ops_index - 1].harmonic & 15] / (Uint64)harmonicOPN1[cydchn->fm.ops[ops_index - 1].harmonic >> 4] / (((chn->instrument->fm_flags & CYD_FM_ENABLE_3CH_EXP_MODE) ? (chn->instrument->ops[ops_index - 1].flags & MUS_FM_OP_QUARTER_FREQ) : (chn->instrument->flags & MUS_INST_QUARTER_FREQ)) ? 4 : 1);
								}
							}
						}
					}
				}
				break;*/
				
				case MUS_FX_SET_CSM_TIMER_NOTE:
				{
					if(ops_index == 0xFF || ops_index == 0)
					{
						if(ops_index == 0xFF)
						{
							for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
							{
								chn->ops[i].CSM_timer_note = (chn->ops[i].CSM_timer_note & 0x00ff) | ((inst & 0xff) << 8);
							}
						}
					}
					
					else
					{
						chn->ops[ops_index - 1].CSM_timer_note = (chn->ops[ops_index - 1].CSM_timer_note & 0x00ff) | ((inst & 0xff) << 8);
					}
				}
				break;
				
				case MUS_FX_SET_CSM_TIMER_FINETUNE:
				{
					if(ops_index == 0xFF || ops_index == 0)
					{
						if(ops_index == 0xFF)
						{
							for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
							{
								chn->ops[i].CSM_timer_note = (chn->ops[i].CSM_timer_note & 0xff00) | (inst & 0xff);
							}
						}
					}
					
					else
					{
						chn->ops[ops_index - 1].CSM_timer_note = (chn->ops[ops_index - 1].CSM_timer_note & 0x00ff) | (inst & 0xff);
					}
				}
				break;
				
				case MUS_FX_FM_SET_4OP_ALGORITHM:
				{
					if(ops_index == 0xFF || ops_index == 0)
					{
						cydchn->fm.alg = inst & 0xFF;
					}
				}
				break;
				
				case MUS_FX_FM_SET_4OP_MASTER_VOLUME:
				{
					if(ops_index == 0xFF || ops_index == 0)
					{
						track_status->fm_4op_vol = inst & 0xFF;
					}
				}
				break;
		
				case MUS_FX_SET_GLOBAL_VOLUME:
				{
					if(ops_index == 0 || ops_index == 0xFF)
					{
						mus->play_volume = my_min((inst & 0xff), MAX_VOLUME);

						update_all_volumes(mus);
					}
				}
				break;

				case MUS_FX_FADE_GLOBAL_VOLUME:
				{
					if(ops_index == 0 || ops_index == 0xFF)
					{
						mus->play_volume -= inst & 0xf;

						if (mus->play_volume > MAX_VOLUME) mus->play_volume = 0;

						mus->play_volume += (inst & 0xf0) >> 4;

						if (mus->play_volume > MAX_VOLUME) mus->play_volume = MAX_VOLUME;

						update_all_volumes(mus);
					}
				}
				break;

				case MUS_FX_SET_CHANNEL_VOLUME:
				{
					if(ops_index == 0 || ops_index == 0xFF)
					{
						chn->volume = my_min((inst & 0xff), MAX_VOLUME);
						update_volumes(mus, track_status, chn, cydchn, track_status->volume);
					}
				}
				break;

				case MUS_FX_PW_SET:
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							track_status->pw = (inst & 0xff) << 4;
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									track_status->ops_status[i].pw = (inst & 0xff) << 4;
								}
							}
							
							break;
						}
						
						default:
						{
							track_status->ops_status[ops_index - 1].pw = (inst & 0xff) << 4;
							
							break;
						}
					}
				}
				break;
#ifndef CYD_DISABLE_BUZZ
				case MUS_FX_BUZZ_SHAPE:
				{
					if(ops_index == 0 || ops_index == 0xFF)
					{
						cyd_set_env_shape(cydchn, inst & 3);
					}
				}
				break;

				case MUS_FX_BUZZ_SET_SEMI:
				{
					if(ops_index == 0 || ops_index == 0xFF)
					{
						chn->buzz_offset = (((inst & 0xff)) - 0x80) << 8;

						mus_set_buzz_frequency(mus, chan, chn->note);
					}
				}
				break;

				case MUS_FX_BUZZ_SET:
				{
					if(ops_index == 0 || ops_index == 0xFF)
					{
						chn->buzz_offset = (chn->buzz_offset & 0xff00) | (inst & 0xff);

						mus_set_buzz_frequency(mus, chan, chn->note);
					}
				}
				break;
#endif

#ifndef CYD_DISABLE_FM
				case MUS_FX_FM_SET_MODULATION:
				{
					if(ops_index == 0 || ops_index == 0xFF)
					{
						cydchn->fm.adsr.volume = inst & 0xff;
					}
				}
				break;

				case MUS_FX_FM_SET_HARMONIC:
				{
					cydchn->fm.harmonic = inst & 0xff;
					
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							cydchn->fm.harmonic = inst & 0xff;
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									cydchn->fm.ops[i].harmonic = inst & 0xff;
								}
							}
							
							break;
						}
						
						default:
						{
							cydchn->fm.ops[ops_index - 1].harmonic = inst & 0xff;
							
							break;
						}
					}
				}
				break;

				case MUS_FX_FM_SET_WAVEFORM:
				{
					if(ops_index == 0 || ops_index == 0xFF)
					{
						if ((inst & 255) < CYD_WAVE_MAX_ENTRIES)
						{
							cydchn->fm.wave_entry = &mus->cyd->wavetable_entries[inst & 255];
						}
					}
				}
				break;
#endif

#ifdef STEREOOUTPUT
				case MUS_FX_SET_PANNING:
				{
					if(ops_index == 0 || ops_index == 0xFF)
					{
						cyd_set_panning(mus->cyd, cydchn, inst & 0xff);
						
						cydchn->init_panning = inst & 0xff;
					}
				}
				break;
#endif

#ifndef CYD_DISABLE_FILTER
				case MUS_FX_FILTER_TYPE:
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							cydchn->flttype = (inst & 0xf);
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									cydchn->fm.ops[i].flttype = (inst & 0xf);
								}
							}
							
							break;
						}
						
						default:
						{
							cydchn->fm.ops[ops_index - 1].flttype = (inst & 0xf);
							
							break;
						}
					}
				}
				break;

				case MUS_FX_CUTOFF_SET:
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							track_status->filter_cutoff = (inst & 0xff) << 4; //track_status->filter_cutoff = (inst & 0xff) << 3;
							cyd_set_filter_coeffs(mus->cyd, cydchn, track_status->filter_cutoff, track_status->filter_resonance);
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									track_status->ops_status[i].filter_cutoff = (inst & 0xff) << 4;
									
									for(int j = 0; j < (int)pow(2, cydchn->fm.ops[i].flt_slope); ++j)
									{
										for (int sub = 0; sub < CYD_SUB_OSCS; ++sub)
										{
											if(mus->cyd->flags & CYD_USE_OLD_FILTER)
											{
												cydflt_set_coeff_old(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, resonance_table[track_status->ops_status[i].filter_resonance & 3]);
											}
											
											else
											{
												cydflt_set_coeff(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, track_status->ops_status[i].filter_resonance, mus->cyd->sample_rate);
											}
										}
									}
								}
							}
							
							break;
						}
						
						default:
						{
							track_status->ops_status[ops_index - 1].filter_cutoff = (inst & 0xff) << 4;
							
							for(int j = 0; j < (int)pow(2, cydchn->fm.ops[ops_index - 1].flt_slope); ++j)
							{
								for (int sub = 0; sub < CYD_SUB_OSCS; ++sub)
								{
									if(mus->cyd->flags & CYD_USE_OLD_FILTER)
									{
										cydflt_set_coeff_old(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, resonance_table[track_status->ops_status[ops_index - 1].filter_resonance & 3]);
									}
									
									else
									{
										cydflt_set_coeff(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, track_status->ops_status[ops_index - 1].filter_resonance, mus->cyd->sample_rate);
									}
								}
							}
							
							break;
						}
					}
				}
				break;

				case MUS_FX_CUTOFF_SET_COMBINED:
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							if ((inst & 0xff) < 0x80)
							{
								track_status->filter_cutoff = (inst & 0xff) << 5;
								cydchn->flttype = FLT_LP;
								cyd_set_filter_coeffs(mus->cyd, cydchn, track_status->filter_cutoff, track_status->filter_resonance);
							}
							
							else
							{
								track_status->filter_cutoff = ((inst & 0xff) - 0x80) << 5;
								cydchn->flttype = FLT_HP;
								cyd_set_filter_coeffs(mus->cyd, cydchn, track_status->filter_cutoff, track_status->filter_resonance);
							}
							
							if(ops_index == 0xFF)
							{
								if ((inst & 0xff) < 0x80)
								{
									for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
									{
										track_status->ops_status[i].filter_cutoff = (inst & 0xff) << 5;
										cydchn->fm.ops[i].flttype = FLT_LP;
										
										for(int j = 0; j < (int)pow(2, cydchn->fm.ops[i].flt_slope); ++j)
										{
											for (int sub = 0; sub < CYD_SUB_OSCS; ++sub)
											{
												if(mus->cyd->flags & CYD_USE_OLD_FILTER)
												{
													cydflt_set_coeff_old(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, resonance_table[track_status->ops_status[i].filter_resonance & 3]);
												}
												
												else
												{
													cydflt_set_coeff(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, track_status->ops_status[i].filter_resonance, mus->cyd->sample_rate);
												}
											}
										}
									}
								}
								
								else
								{
									for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
									{
										track_status->ops_status[i].filter_cutoff = ((inst & 0xff) - 0x80) << 5;
										cydchn->fm.ops[i].flttype = FLT_HP;
										
										for(int j = 0; j < (int)pow(2, cydchn->fm.ops[i].flt_slope); ++j)
										{
											for (int sub = 0; sub < CYD_SUB_OSCS; ++sub)
											{
												if(mus->cyd->flags & CYD_USE_OLD_FILTER)
												{
													cydflt_set_coeff_old(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, resonance_table[track_status->ops_status[i].filter_resonance & 3]);
												}
												
												else
												{
													cydflt_set_coeff(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, track_status->ops_status[i].filter_resonance, mus->cyd->sample_rate);
												}
											}
										}
									}
								}
							}
							
							break;
						}
						
						default:
						{
							if ((inst & 0xff) < 0x80)
							{
								track_status->ops_status[ops_index - 1].filter_cutoff = (inst & 0xff) << 5;
								cydchn->fm.ops[ops_index - 1].flttype = FLT_LP;
								
								for(int j = 0; j < (int)pow(2, cydchn->fm.ops[ops_index - 1].flt_slope); ++j)
								{
									for (int sub = 0; sub < CYD_SUB_OSCS; ++sub)
									{
										if(mus->cyd->flags & CYD_USE_OLD_FILTER)
										{
											cydflt_set_coeff_old(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, resonance_table[track_status->ops_status[ops_index - 1].filter_resonance & 3]);
										}
										
										else
										{
											cydflt_set_coeff(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, track_status->ops_status[ops_index - 1].filter_resonance, mus->cyd->sample_rate);
										}
									}
								}
							}
							
							else
							{
								track_status->ops_status[ops_index - 1].filter_cutoff = ((inst & 0xff) - 0x80) << 5;
								cydchn->fm.ops[ops_index - 1].flttype = FLT_HP;
								
								for(int j = 0; j < (int)pow(2, cydchn->fm.ops[ops_index - 1].flt_slope); ++j)
								{
									for (int sub = 0; sub < CYD_SUB_OSCS; ++sub)
									{
										if(mus->cyd->flags & CYD_USE_OLD_FILTER)
										{
											cydflt_set_coeff_old(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, resonance_table[track_status->ops_status[ops_index - 1].filter_resonance & 3]);
										}
										
										else
										{
											cydflt_set_coeff(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, track_status->ops_status[ops_index - 1].filter_resonance, mus->cyd->sample_rate);
										}
									}
								}
							}
							
							break;
						}
					}
				}
				break;

				case MUS_FX_RESONANCE_SET:
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							track_status->filter_resonance = inst & 15; //was `inst & 3;`
							cyd_set_filter_coeffs(mus->cyd, cydchn, track_status->filter_cutoff, inst & 15); //cyd_set_filter_coeffs(mus->cyd, cydchn, track_status->filter_cutoff, inst & 3);
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									track_status->ops_status[i].filter_resonance = (inst & 15);
									
									for(int j = 0; j < (int)pow(2, cydchn->fm.ops[i].flt_slope); ++j)
									{
										for (int sub = 0; sub < CYD_SUB_OSCS; ++sub)
										{
											if(mus->cyd->flags & CYD_USE_OLD_FILTER)
											{
												cydflt_set_coeff_old(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, resonance_table[track_status->ops_status[i].filter_resonance & 3]);
											}
											
											else
											{
												cydflt_set_coeff(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, track_status->ops_status[i].filter_resonance, mus->cyd->sample_rate);
											}
										}
									}
								}
							}
							
							break;
						}
						
						default:
						{
							track_status->ops_status[ops_index - 1].filter_resonance = (inst & 15);
							
							for(int j = 0; j < (int)pow(2, cydchn->fm.ops[ops_index - 1].flt_slope); ++j)
							{
								for (int sub = 0; sub < CYD_SUB_OSCS; ++sub)
								{
									if(mus->cyd->flags & CYD_USE_OLD_FILTER)
									{
										cydflt_set_coeff_old(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, resonance_table[track_status->ops_status[ops_index - 1].filter_resonance & 3]);
									}
									
									else
									{
										cydflt_set_coeff(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, track_status->ops_status[ops_index - 1].filter_resonance, mus->cyd->sample_rate);
									}
								}
							}
							
							break;
						}
					}
				}
				break;
#endif

				case MUS_FX_SET_SPEED:
				{
					if (from_program)
					{
						if(ops_index == 0xff || ops_index == 0)
						{
							chn->prog_period[prog_number] = inst & 0xff;
						}
						
						else
						{
							chn->ops[ops_index - 1].prog_period[prog_number] = inst & 0xff;
						}
					}
					
					else
					{
						mus->song->song_speed = inst & 0xf;
						if ((inst & 0xf0) == 0) mus->song->song_speed2 = mus->song->song_speed;
						else mus->song->song_speed2 = (inst >> 4) & 0xf;
					}
				}
				break;
				
				case MUS_FX_SET_SPEED1:
				{
					if (!from_program)
					{
						mus->song->song_speed = inst & 0xff;
					}
				}
				break;
				
				case MUS_FX_SET_SPEED2:
				{
					if (!from_program)
					{
						mus->song->song_speed2 = inst & 0xff;
					}
				}
				break;

				case MUS_FX_SET_RATE:
				{
					if (!from_program)
					{
						if((mus->song->song_rate & 0xFF00) + (inst & 0xff) <= 44100)
						{
							mus->song->song_rate &= 0xFF00;
							mus->song->song_rate += inst & 0xff;
							if (mus->song->song_rate < 1) mus->song->song_rate = 1;
							cyd_set_callback_rate(mus->cyd, mus->song->song_rate);
						}
					}
				}
				break;
				
				case MUS_FX_SET_RATE_HIGHER_BYTE: //wasn't there
				{
					if (!from_program)
					{
						if((mus->song->song_rate & 0x00FF) + ((inst & 0xff) << 8) <= 44100)
						{
							mus->song->song_rate &= 0x00FF;
							mus->song->song_rate += (inst & 0xff) << 8;
							if (mus->song->song_rate < 1) mus->song->song_rate = 1;
							cyd_set_callback_rate(mus->cyd, mus->song->song_rate);
						}
					}
				}
				break;

				case MUS_FX_PORTA_UP_SEMI:
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							Uint32 prev = chn->note;
							chn->note += (inst & 0xff) << 8;
							if (prev > chn->note || chn->note >= (FREQ_TAB_SIZE << 8)) chn->note = ((FREQ_TAB_SIZE - 1) << 8);
							mus_set_slide(mus, chan, chn->note);
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									Uint32 prev = chn->ops[i].note;
									chn->ops[i].note += (inst & 0xff) << 8;
									if (prev > chn->ops[i].note || chn->ops[i].note >= (FREQ_TAB_SIZE << 8)) chn->ops[i].note = ((FREQ_TAB_SIZE - 1) << 8);
									
									chn->ops[i].target_note = chn->ops[i].note;
								}
							}
							
							break;
						}
						
						default:
						{
							Uint32 prev = chn->ops[ops_index - 1].note;
							chn->ops[ops_index - 1].note += (inst & 0xff) << 8;
							if (prev > chn->ops[ops_index - 1].note || chn->ops[ops_index - 1].note >= (FREQ_TAB_SIZE << 8)) chn->ops[ops_index - 1].note = ((FREQ_TAB_SIZE - 1) << 8);
							
							chn->ops[ops_index - 1].target_note = chn->ops[ops_index - 1].note;
						}
					}
				}
				break;

				case MUS_FX_PORTA_DN_SEMI:
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							Uint32 prev = chn->note;
							chn->note -= (inst & 0xff) << 8;
							if (prev < chn->note) chn->note = 0x0;
							mus_set_slide(mus, chan, chn->note);
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									Uint32 prev = chn->ops[i].note;
									chn->ops[i].note -= (inst & 0xff) << 8;
									if (prev < chn->ops[i].note) chn->ops[i].note = 0x0;
									
									chn->ops[i].target_note = chn->ops[i].note;
								}
							}
							
							break;
						}
						
						default:
						{
							Uint32 prev = chn->ops[ops_index - 1].note;
							chn->ops[ops_index - 1].note -= (inst & 0xff) << 8;
							if (prev < chn->ops[ops_index - 1].note) chn->ops[ops_index - 1].note = 0x0;
							
							chn->ops[ops_index - 1].target_note = chn->ops[ops_index - 1].note;
						}
					}
				}
				break;

				case MUS_FX_ARPEGGIO_ABS:
				{
					if(ops_index == 0 || ops_index == 0xFF)
					{
						chn->arpeggio_note = 0;
						chn->fixed_note = (inst & 0xff) << 8;
						
						if(ops_index == 0xFF)
						{
							for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
							{
								chn->ops[i].arpeggio_note = 0;
								chn->ops[i].fixed_note = (inst & 0xff) << 8;
							}
						}
					}
					
					else
					{
						chn->ops[ops_index - 1].arpeggio_note = 0;
						chn->ops[ops_index - 1].fixed_note = (inst & 0xff) << 8;
					}
				}
				break;

				case MUS_FX_ARPEGGIO:
				{
					if(ops_index == 0 || ops_index == 0xFF)
					{
						if (chn->fixed_note != 0xffff)
						{
							chn->note = chn->last_note;
							chn->fixed_note = 0xffff;
						}

						if ((inst & 0xff) == 0xf0)
							chn->arpeggio_note = track_status->extarp1;
						else if ((inst & 0xff) == 0xf1)
							chn->arpeggio_note = track_status->extarp2;
						else
							chn->arpeggio_note = inst & 0xff;
						
						if(ops_index == 0xFF)
						{
							for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
							{
								if (chn->ops[i].fixed_note != 0xffff)
								{
									chn->ops[i].note = chn->ops[i].last_note;
									chn->ops[i].fixed_note = 0xffff;
								}
								
								chn->ops[i].arpeggio_note = inst & 0xff;
							}
						}
					}
					
					else
					{
						if (chn->ops[ops_index - 1].fixed_note != 0xffff)
						{
							chn->ops[ops_index - 1].note = chn->ops[ops_index - 1].last_note;
							chn->ops[ops_index - 1].fixed_note = 0xffff;
						}
						
						chn->ops[ops_index - 1].arpeggio_note = inst & 0xff;
					}
				}
				break;
				
				case MUS_FX_ARPEGGIO_DOWN:
				{
					if(ops_index == 0 || ops_index == 0xFF)
					{
						if (chn->fixed_note != 0xffff)
						{
							chn->note = chn->last_note;
							chn->fixed_note = 0xffff;
						}

						if ((inst & 0xff) == 0xf0)
							chn->arpeggio_note = track_status->extarp1;
						else if ((inst & 0xff) == 0xf1)
							chn->arpeggio_note = track_status->extarp2;
						else
							chn->arpeggio_note = -1 * (inst & 0xff);
						
						if(ops_index == 0xFF)
						{
							for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
							{
								if (chn->ops[i].fixed_note != 0xffff)
								{
									chn->ops[i].note = chn->ops[i].last_note;
									chn->ops[i].fixed_note = 0xffff;
								}
								
								chn->ops[i].arpeggio_note = -1 * (inst & 0xff);
							}
						}
					}
					
					else
					{
						if (chn->ops[ops_index - 1].fixed_note != 0xffff)
						{
							chn->ops[ops_index - 1].note = chn->ops[ops_index - 1].last_note;
							chn->ops[ops_index - 1].fixed_note = 0xffff;
						}
						
						chn->ops[ops_index - 1].arpeggio_note = -1 * (inst & 0xff);
					}
				}
				break;
				
				case MUS_FX_SET_NOISE_CONSTANT_PITCH:
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							Uint32 frequency = get_freq(((inst & 0xff) << 8) & mus->pitch_mask) / ((cydchn->musflags & MUS_INST_QUARTER_FREQ) ? 4 : 1);
					
							for(int i = 0; i < CYD_SUB_OSCS; ++i)
							{
								if (frequency != 0)
								{
									cydchn->subosc[i].noise_frequency = (Uint64)(ACC_LENGTH >> (mus->cyd->oversample)) / (Uint64)1024 * (Uint64)(frequency) / (Uint64)mus->cyd->sample_rate;
								}
							}
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									frequency = get_freq(((inst & 0xff) << 8) & mus->pitch_mask) / ((cydchn->musflags & MUS_INST_QUARTER_FREQ) ? 4 : 1);
									
									if (frequency != 0)
									{
										for (int s = 0; s < CYD_SUB_OSCS; ++s)
										{
											cydchn->fm.ops[i].subosc[s].noise_frequency = (Uint64)(ACC_LENGTH >> (mus->cyd->oversample)) / (Uint64)1024 * (Uint64)(frequency) / (Uint64)mus->cyd->sample_rate;
										}
									}
								}
							}
							
							break;
						}
						
						default:
						{
							Uint32 frequency = get_freq(((inst & 0xff) << 8) & mus->pitch_mask) / ((cydchn->musflags & MUS_INST_QUARTER_FREQ) ? 4 : 1);
							
							if (frequency != 0)
							{
								for (int s = 0; s < CYD_SUB_OSCS; ++s)
								{
									cydchn->fm.ops[ops_index - 1].subosc[s].noise_frequency = (Uint64)(ACC_LENGTH >> (mus->cyd->oversample)) / (Uint64)1024 * (Uint64)(frequency) / (Uint64)mus->cyd->sample_rate;
								}
							}
							
							break;
						}
					}
				}
				break;

				case MUS_FX_SET_VOLUME:
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							track_status->volume = my_min(MAX_VOLUME, inst & 0xff);
							update_volumes(mus, track_status, chn, cydchn, track_status->volume);
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									track_status->ops_status[i].volume = my_min(MAX_VOLUME, inst & 0xff);
									update_fm_op_volume(mus, track_status, chn, cydchn, track_status->ops_status[i].volume, i);
								}
							}
							
							break;
						}
						
						default:
						{
							track_status->ops_status[ops_index - 1].volume = my_min(MAX_VOLUME, inst & 0xff);
							update_fm_op_volume(mus, track_status, chn, cydchn, track_status->ops_status[ops_index - 1].volume, ops_index - 1);
							
							break;
						}
					}
				}
				break;

				case MUS_FX_SET_SYNCSRC:
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							if ((inst & 0xff) != 0xff)
							{
								cydchn->sync_source = (inst & 0xff);
								cydchn->flags |= CYD_CHN_ENABLE_SYNC;
							}
							
							else
							{
								cydchn->flags &= ~CYD_CHN_ENABLE_SYNC;
							}
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									if ((inst & 0xff) != 0xff)
									{
										cydchn->fm.ops[i].sync_source = (inst & 0xff);
										cydchn->fm.ops[i].flags |= CYD_FM_OP_ENABLE_SYNC;
									}
									
									else
									{
										cydchn->fm.ops[i].flags &= ~CYD_FM_OP_ENABLE_SYNC;
									}
								}
							}
							
							break;
						}
						
						default:
						{
							if ((inst & 0xff) != 0xff)
							{
								cydchn->fm.ops[ops_index - 1].sync_source = (inst & 0xff);
								cydchn->fm.ops[ops_index - 1].flags |= CYD_FM_OP_ENABLE_SYNC;
							}
							
							else
							{
								cydchn->fm.ops[ops_index - 1].flags &= ~CYD_FM_OP_ENABLE_SYNC;
							}
							
							break;
						}
					}
				}
				break;

				case MUS_FX_SET_RINGSRC:
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							if ((inst & 0xff) != 0xff)
							{
								cydchn->ring_mod = (inst & 0xff);
								cydchn->flags |= CYD_CHN_ENABLE_RING_MODULATION;
							}
							
							else
							{
								cydchn->flags &= ~CYD_CHN_ENABLE_RING_MODULATION;
							}
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									if ((inst & 0xff) != 0xff)
									{
										cydchn->fm.ops[i].ring_mod = (inst & 0xff);
										cydchn->fm.ops[i].flags |= CYD_FM_OP_ENABLE_RING_MODULATION;
									}
									
									else
									{
										cydchn->fm.ops[i].flags &= ~CYD_FM_OP_ENABLE_RING_MODULATION;
									}
								}
							}
							
							break;
						}
						
						default:
						{
							if ((inst & 0xff) != 0xff)
							{
								cydchn->fm.ops[ops_index - 1].ring_mod = (inst & 0xff);
								cydchn->fm.ops[ops_index - 1].flags |= CYD_FM_OP_ENABLE_RING_MODULATION;
							}
							
							else
							{
								cydchn->fm.ops[ops_index - 1].flags &= ~CYD_FM_OP_ENABLE_RING_MODULATION;
							}
							
							break;
						}
					}
				}
				break;

#ifndef CYD_DISABLE_FX
				case MUS_FX_SET_FXBUS:
				{
					if(ops_index == 0 || ops_index == 0xFF)
					{
						cydchn->fx_bus = (inst & 0xff) & (CYD_MAX_FX_CHANNELS - 1);
					}
				}
				break;

				case MUS_FX_SET_DOWNSAMPLE:
				{
					if(ops_index == 0 || ops_index == 0xFF)
					{
						cydcrush_set(&cyd->fx[cydchn->fx_bus].crush, inst & 0xff, -1, -1, -1, cyd->sample_rate);
					}
				}
				break;
#endif

#ifndef CYD_DISABLE_WAVETABLE
				case MUS_FX_SET_WAVETABLE_ITEM:
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							if ((inst & 255) < CYD_WAVE_MAX_ENTRIES)
							{
								cydchn->wave_entry = &mus->cyd->wavetable_entries[inst & 255];
							}
							
							if(ops_index == 0xFF)
							{
								if ((inst & 255) < CYD_WAVE_MAX_ENTRIES)
								{
									for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
									{
										cydchn->fm.ops[i].wave_entry = &mus->cyd->wavetable_entries[inst & 255];
									}
								}
							}
							
							break;
						}
						
						default:
						{
							if ((inst & 255) < CYD_WAVE_MAX_ENTRIES)
							{
								cydchn->fm.ops[ops_index - 1].wave_entry = &mus->cyd->wavetable_entries[inst & 255];
							}
							
							break;
						}
					}
				}
				break;
#endif
				case MUS_FX_SET_WAVEFORM:
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							Uint32 final = 0;

							if (inst & MUS_FX_WAVE_NOISE)
								final |= CYD_CHN_ENABLE_NOISE;

							if (inst & MUS_FX_WAVE_PULSE)
								final |= CYD_CHN_ENABLE_PULSE;

							if (inst & MUS_FX_WAVE_TRIANGLE)
								final |= CYD_CHN_ENABLE_TRIANGLE;

							if (inst & MUS_FX_WAVE_SAW)
								final |= CYD_CHN_ENABLE_SAW;

							if (inst & MUS_FX_WAVE_WAVE)
								final |= CYD_CHN_ENABLE_WAVE;
							
							if (inst & MUS_FX_WAVE_SINE)
								final |= CYD_CHN_ENABLE_SINE;

#ifndef CYD_DISABLE_LFSR
							if (inst & MUS_FX_WAVE_LFSR)
								final |= CYD_CHN_ENABLE_LFSR;
#endif

							cyd_set_waveform(cydchn, final);
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									final = 0;

									if (inst & MUS_FX_WAVE_NOISE)
										final |= CYD_FM_OP_ENABLE_NOISE;

									if (inst & MUS_FX_WAVE_PULSE)
										final |= CYD_FM_OP_ENABLE_PULSE;

									if (inst & MUS_FX_WAVE_TRIANGLE)
										final |= CYD_FM_OP_ENABLE_TRIANGLE;

									if (inst & MUS_FX_WAVE_SAW)
										final |= CYD_FM_OP_ENABLE_SAW;
									
									if (inst & MUS_FX_WAVE_SINE)
										final |= CYD_FM_OP_ENABLE_SINE;

									if ((inst & MUS_FX_WAVE_WAVE) && (chn->instrument->ops[i].cydflags & CYD_FM_OP_ENABLE_WAVE))
										final |= CYD_FM_OP_ENABLE_WAVE;
									
									cydchn->fm.ops[i].flags = ((Uint32)cydchn->fm.ops[i].flags & (Uint32)(~FM_OP_WAVEFORMS)) | ((Uint32)final & (Uint32)FM_OP_WAVEFORMS);
								}
							}
							
							break;
						}
						
						default:
						{
							Uint32 final = 0;
							
							if (inst & MUS_FX_WAVE_NOISE)
								final |= CYD_FM_OP_ENABLE_NOISE;

							if (inst & MUS_FX_WAVE_PULSE)
								final |= CYD_FM_OP_ENABLE_PULSE;

							if (inst & MUS_FX_WAVE_TRIANGLE)
								final |= CYD_FM_OP_ENABLE_TRIANGLE;

							if (inst & MUS_FX_WAVE_SAW)
								final |= CYD_FM_OP_ENABLE_SAW;
							
							if (inst & MUS_FX_WAVE_SINE)
								final |= CYD_FM_OP_ENABLE_SINE;

							if ((inst & MUS_FX_WAVE_WAVE) && (chn->instrument->ops[ops_index - 1].cydflags & CYD_FM_OP_ENABLE_WAVE))
								final |= CYD_FM_OP_ENABLE_WAVE;
							
							cydchn->fm.ops[ops_index - 1].flags = (cydchn->fm.ops[ops_index - 1].flags & (~FM_OP_WAVEFORMS)) | (final & FM_OP_WAVEFORMS);
							
							break;
						}
					}
				}
				break;

				case MUS_FX_RESTART_PROGRAM:
				{
					if (!from_program)
					{
						if(ops_index == 0 || ops_index == 0xFF)
						{
							for(int pr = 0; pr < chn->instrument->num_macros; ++pr)
							{
								chn->program_counter[pr] = 0;
								chn->program_tick[pr] = 0;
								
								for(int i = 0; i < MUS_MAX_NESTEDNESS; ++i)
								{
									chn->program_loop[pr][i] = 1;
									chn->program_loop_addresses[pr][i][0] = chn->program_loop_addresses[pr][i][1] = 0;
								}
								
								chn->nestedness[pr] = 0;
				
								int i = 1;
								int temp_address = 0;
								
								while(i < MUS_MAX_NESTEDNESS && temp_address < MUS_PROG_LEN)
								{
									if(chn->instrument->program[pr][temp_address] == MUS_FX_LABEL)
									{
										chn->program_loop_addresses[pr][i][0] = temp_address;
										i++;
									}
									
									if(chn->instrument->program[pr][temp_address] == MUS_FX_LOOP)
									{
										goto loops;
									}
									
									else
									{
										temp_address++;
									}
								}
								
								loops:;
							
							}
							
							for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
							{
								for(int pr = 0; pr < chn->instrument->ops[i].num_macros; ++pr)
								{
									chn->ops[i].program_counter[pr] = 0;
									chn->ops[i].program_tick[pr] = 0;
									//chn->ops[i].program_loop = 1;
									
									for(int j = 0; j < MUS_MAX_NESTEDNESS; ++j)
									{
										chn->ops[i].program_loop[pr][j] = 1;
										chn->ops[i].program_loop_addresses[pr][j][0] = chn->ops[i].program_loop_addresses[pr][j][1] = 0;
									}
									
									chn->ops[i].nestedness[pr] = 0;
			
									int j = 1;
									int temp_address = 0;
									
									while(j < MUS_MAX_NESTEDNESS && temp_address < MUS_PROG_LEN)
									{
										if(chn->instrument->ops[i].program[pr][temp_address] == MUS_FX_LABEL)
										{
											chn->ops[i].program_loop_addresses[pr][j][0] = temp_address;
											j++;
										}
										
										if(chn->instrument->ops[i].program[pr][temp_address] == MUS_FX_LOOP)
										{
											goto loops1;
										}
										
										else
										{
											temp_address++;
										}
									}
									
									loops1:;
								}
							}
						}
						
						else //if from phat fucker 5-col pattern which is/would be workaround for Deflemask and Furnace extended ch3 modes
						{
							for(int pr = 0; pr < chn->instrument->ops[ops_index - 1].num_macros; ++pr)
							{
								chn->ops[ops_index - 1].program_counter[pr] = 0;
								chn->ops[ops_index - 1].program_tick[pr] = 0;
								//chn->ops[i].program_loop = 1;
								
								for(int j = 0; j < MUS_MAX_NESTEDNESS; ++j)
								{
									chn->ops[ops_index - 1].program_loop[pr][j] = 1;
									chn->ops[ops_index - 1].program_loop_addresses[pr][j][0] = chn->ops[ops_index - 1].program_loop_addresses[pr][j][1] = 0;
								}
								
								chn->ops[ops_index - 1].nestedness[pr] = 0;
		
								int j = 1;
								int temp_address = 0;
								
								while(j < MUS_MAX_NESTEDNESS && temp_address < MUS_PROG_LEN)
								{
									if(chn->instrument->ops[ops_index - 1].program[pr][temp_address] == MUS_FX_LABEL)
									{
										chn->ops[ops_index - 1].program_loop_addresses[pr][j][0] = temp_address;
										j++;
									}
									
									if(chn->instrument->ops[ops_index - 1].program[pr][temp_address] == MUS_FX_LOOP)
									{
										goto loops2;
									}
									
									else
									{
										temp_address++;
									}
								}
								
								loops2:;
							}
						}
					}
				}
				break;
			}

			break;
		}
	}
}


static void mus_exec_track_command(MusEngine *mus, int chan, int first_tick)
{
	MusTrackStatus *track_status = &mus->song_track[chan];
	const Uint8 vol = track_status->pattern->step[track_status->pattern_step].volume;
	
	switch (vol & 0xf0)
	{
		case MUS_NOTE_VOLUME_PAN_LEFT:
				do_command(mus, chan, mus->song_counter, MUS_FX_PAN_LEFT | ((Uint16)(vol & 0xf) * 2), 0, mus->channel[chan].instrument != NULL ? ((mus->channel[chan].instrument->fm_flags & CYD_FM_FOUROP_USE_MAIN_INST_PROG) ? 0xFF : 0) : 0, 0);
			break;

		case MUS_NOTE_VOLUME_PAN_RIGHT:
				do_command(mus, chan, mus->song_counter, MUS_FX_PAN_RIGHT | ((Uint16)(vol & 0xf) * 2), 0, mus->channel[chan].instrument != NULL ? ((mus->channel[chan].instrument->fm_flags & CYD_FM_FOUROP_USE_MAIN_INST_PROG) ? 0xFF : 0) : 0, 0);
			break;

		case MUS_NOTE_VOLUME_SET_PAN:
		{
			Uint16 val = vol & 0xf;
			Uint16 panning = (val <= 8 ? val * CYD_PAN_CENTER / 8 : (val - 8) * (CYD_PAN_RIGHT - CYD_PAN_CENTER) / 8 + CYD_PAN_CENTER);
			
			do_command(mus, chan, mus->song_counter, MUS_FX_SET_PANNING | panning, 0, mus->channel[chan].instrument != NULL ? ((mus->channel[chan].instrument->fm_flags & CYD_FM_FOUROP_USE_MAIN_INST_PROG) ? 0xFF : 0) : 0, 0);
			
			//debug("Panned to %x", panning);
		}
		break;

		case MUS_NOTE_VOLUME_FADE_UP:
			do_command(mus, chan, mus->song_counter, MUS_FX_FADE_VOLUME | ((Uint16)(vol & 0xf) << 4), 0, mus->channel[chan].instrument != NULL ? ((mus->channel[chan].instrument->fm_flags & CYD_FM_FOUROP_USE_MAIN_INST_PROG) ? 0xFF : 0) : 0, 0);
		break;

		case MUS_NOTE_VOLUME_FADE_DN:
			do_command(mus, chan, mus->song_counter, MUS_FX_FADE_VOLUME | ((Uint16)(vol & 0xf)), 0, mus->channel[chan].instrument != NULL ? ((mus->channel[chan].instrument->fm_flags & CYD_FM_FOUROP_USE_MAIN_INST_PROG) ? 0xFF : 0) : 0, 0);
			break;

		default:
			if (vol <= MAX_VOLUME)
			{
				do_command(mus, chan, first_tick ? 0 : mus->song_counter, MUS_FX_SET_VOLUME | (Uint16)(vol), 0, mus->channel[chan].instrument != NULL ? ((mus->channel[chan].instrument->fm_flags & CYD_FM_FOUROP_USE_MAIN_INST_PROG) ? 0xFF : 0) : 0, 0);
			}
			break;
	}
	
	for(int i = 0; i < MUS_MAX_COMMANDS; ++i)
	{
		Uint16 inst = track_status->pattern->step[track_status->pattern_step].command[i];

		switch (inst & 0xff00)
		{
			case MUS_FX_ARPEGGIO:
				if (!(inst & 0xff)) break; // no params = use the same settings
			case MUS_FX_SET_EXT_ARP:
			{
				track_status->extarp1 = (inst & 0xf0) >> 4;
				track_status->extarp2 = (inst & 0xf);
			}
			break;
			
			case MUS_FX_SET_2ND_ARP_NOTE:
			{
				track_status->extarp1 = (inst & 0xff);
			}
			break;
			
			case MUS_FX_SET_3RD_ARP_NOTE:
			{
				track_status->extarp2 = (inst & 0xff);
			}
			break;

			default:
			{
				if(mus->channel[chan].instrument != NULL || ((inst & 0xfff0) != MUS_FX_EXT_RETRIGGER && (inst & 0xff00) != MUS_FX_RESTART_PROGRAM))
				{
					do_command(mus, chan, mus->song_counter, inst, 0, mus->channel[chan].instrument != NULL ? ((mus->channel[chan].instrument->fm_flags & CYD_FM_FOUROP_USE_MAIN_INST_PROG) ? 0xFF : 0) : 0, 0);
				} //mus->channel[chan].instrument != NULL ? ((mus->channel[chan].instrument->fm_flags & CYD_FM_FOUROP_USE_MAIN_INST_PROG) ? 0xFF : 0) : 0
			}
			break;
		}
	}
}


static void mus_exec_prog_tick(MusEngine *mus, int chan, int advance, int pr /* program */)
{
	MusChannel *chn = &mus->channel[chan];
	
	int tick = chn->program_tick[pr];
	
	bool increase_nestedness = true;

	do_it_again:;
	
	const Uint16 inst = chn->instrument->program[pr][tick];
	bool unite_bit = (chn->instrument->program_unite_bits[pr][tick / 8] & (1 << (tick & 7))) > 0 ? true : false;
	
	//debug("chn nestedness %d", chn->nestedness);

	switch (inst)
	{
		case MUS_FX_END:
		{
			//chn->flags &= ~MUS_CHN_PROGRAM_RUNNING;
			chn->program_flags &= ~(1 << (pr));
			return;
		}
		break;
	}

	if(inst != MUS_FX_NOP)
	{
		switch (inst & 0xff00)
		{
			case MUS_FX_JUMP:
			{
				//debug("tick tick %d chn->program_tick %d", tick, chn->program_tick);
				//debug("unite bit %d", chn->instrument->program_unite_bits[tick / 8] & (1 << (tick & 7)));
				
				tick = inst & (MUS_PROG_LEN);
				
				if(tick != chn->program_tick[pr])
				{
					//debug("tick %d chn->program_tick %d", tick, chn->program_tick);
					goto do_it_again;
				}
			}
			break;

			case MUS_FX_LABEL:
			{
				if(chn->nestedness[pr] < MUS_MAX_NESTEDNESS && advance && increase_nestedness) chn->nestedness[pr]++;
				
				increase_nestedness = true;
				
				int i = chn->nestedness[pr];
				int temp_address = tick;
				
				if (advance)
				{
					while(i < MUS_MAX_NESTEDNESS && temp_address < MUS_PROG_LEN)
					{
						if(chn->instrument->program[pr][temp_address] == MUS_FX_LABEL)
						{
							chn->program_loop_addresses[pr][i][0] = temp_address;
							//debug("chn->program_loop_addresses[i][0] %d", temp_address);
							i++;
						}
						
						if(chn->instrument->program[pr][temp_address] == MUS_FX_LOOP)
						{
							goto loops3;
						}
						
						else
						{
							temp_address++;
						}
					}
				}
				
				loops3:;
			}
			break;

			case MUS_FX_LOOP:
			{
				if (chn->program_loop[pr][chn->nestedness[pr]] == (inst & 0xff))
				{
					if (advance)
					{
						chn->program_loop[pr][chn->nestedness[pr]] = 1;
						
						if(chn->nestedness[pr] > 0) chn->nestedness[pr]--;
						
						//chn->program_tick++;
						tick++;
						
						int i = chn->nestedness[pr] + 1;
						int temp_address = tick;
						
						while(i < MUS_MAX_NESTEDNESS && temp_address < MUS_PROG_LEN)
						{
							if(chn->instrument->program[pr][temp_address] == MUS_FX_LABEL)
							{
								chn->program_loop_addresses[pr][i][0] = temp_address;
								//debug("chn->program_loop_addresses[i][0] %d", temp_address);
								i++;
							}
							
							if(chn->instrument->program[pr][temp_address] == MUS_FX_LOOP)
							{
								goto loops2;
							}
							
							else
							{
								temp_address++;
							}
						}
						
						loops2:;
						
						goto do_it_again;
					}
				}
				
				else
				{
					if (advance)
					{
						if(chn->program_loop[pr][chn->nestedness[pr]] < 255) ++chn->program_loop[pr][chn->nestedness[pr]];
						
						chn->program_tick[pr] = tick = chn->program_loop_addresses[pr][chn->nestedness[pr]][0];
						increase_nestedness = false;
						goto do_it_again;
					}
				}
			}
			break;

			default:
			
			if(mus->channel[chan].instrument != NULL || ((inst & 0xfff0) != MUS_FX_EXT_RETRIGGER && (inst & 0xff00) != MUS_FX_RESTART_PROGRAM))
			{
				do_command(mus, chan, chn->program_counter[pr], inst, 1, mus->channel[chan].instrument != NULL ? ((mus->channel[chan].instrument->fm_flags & CYD_FM_FOUROP_USE_MAIN_INST_PROG) ? 0xFF : 0) : 0, pr);
			}

			break;
		}
	}

	if (inst == MUS_FX_NOP || (inst & 0xff00) != MUS_FX_JUMP)
	{
		++tick;
		
		if (tick >= MUS_PROG_LEN)
		{
			tick = 0;
		}
	}
	
	if (unite_bit && inst != MUS_FX_NOP) //old command if ((inst & 0x8000) && inst != MUS_FX_NOP && !dont_reloop)
	{
		goto do_it_again;
	}
	
	if (advance)
	{
		chn->program_tick[pr] = tick;
	}
}

static void mus_exec_4op_prog_tick(MusEngine *mus, int chan, int advance, int i /*operator number*/, int pr /* program number */)
{
	MusChannel *chn = &mus->channel[chan];
	
	int tick = chn->ops[i].program_tick[pr];
	
	bool increase_nestedness = true;

	do_it_again4op:;
	
	const Uint16 inst = chn->instrument->ops[i].program[pr][tick];
	bool unite_bit = (chn->instrument->ops[i].program_unite_bits[pr][tick / 8] & (1 << (tick & 7))) > 0 ? true : false;

	switch (inst)
	{
		case MUS_FX_END:
		{
			//chn->ops[i].flags &= ~MUS_FM_OP_PROGRAM_RUNNING;
			chn->ops[i].program_flags &= ~(1 << (pr));
			return;
		}
		break;
	}

	if(inst != MUS_FX_NOP)
	{
		switch (inst & 0xff00)
		{
			case MUS_FX_JUMP:
			{
				tick = inst & (MUS_PROG_LEN);
				
				if(tick != chn->ops[i].program_tick[pr])
				{
					goto do_it_again4op;
				}
			}
			break;

			case MUS_FX_LABEL:
			{
				//chn->instrument->program_unite_bits[tick / 8] |= (1 << (tick & 7)); //wasn't there
				
				if(chn->ops[i].nestedness[pr] < MUS_MAX_NESTEDNESS && advance && increase_nestedness) chn->ops[i].nestedness[pr]++;
				
				increase_nestedness = true;
				
				int j = chn->ops[i].nestedness[pr];
				int temp_address = tick;
				
				if(advance)
				{
					while(j < MUS_MAX_NESTEDNESS && temp_address < MUS_PROG_LEN)
					{
						if(chn->instrument->ops[i].program[pr][temp_address] == MUS_FX_LABEL)
						{
							chn->ops[i].program_loop_addresses[pr][j][0] = temp_address;
							j++;
						}
						
						if(chn->instrument->ops[i].program[pr][temp_address] == MUS_FX_LOOP)
						{
							goto loops3;
						}
						
						else
						{
							temp_address++;
						}
					}
				}
				
				loops3:;
			}
			break;

			case MUS_FX_LOOP:
			{
				if (chn->ops[i].program_loop[pr][chn->ops[i].nestedness[pr]] == (inst & 0xff))
				{
					if (advance)
					{
						chn->ops[i].program_loop[pr][chn->ops[i].nestedness[pr]] = 1;
						
						if(chn->ops[i].nestedness[pr] > 0) chn->ops[i].nestedness[pr]--;
						
						//chn->program_tick++;
						tick++;
						
						int j = chn->ops[i].nestedness[pr] + 1;
						int temp_address = tick;
						
						while(j < MUS_MAX_NESTEDNESS && temp_address < MUS_PROG_LEN)
						{
							if(chn->instrument->ops[i].program[pr][temp_address] == MUS_FX_LABEL)
							{
								chn->ops[i].program_loop_addresses[pr][j][0] = temp_address;
								j++;
							}
							
							if(chn->instrument->ops[i].program[pr][temp_address] == MUS_FX_LOOP)
							{
								goto loops2;
							}
							
							else
							{
								temp_address++;
							}
						}
						
						loops2:;
						
						goto do_it_again4op;
					}
				}
				
				else
				{
					if (advance)
					{
						if(chn->ops[i].program_loop[pr][chn->ops[i].nestedness[pr]] < 255) ++chn->ops[i].program_loop[pr][chn->ops[i].nestedness[pr]];
						
						tick = chn->ops[i].program_loop_addresses[pr][chn->ops[i].nestedness[pr]][0];
						chn->ops[i].program_tick[pr] = tick;
						increase_nestedness = false;
						goto do_it_again4op;
					}
				}
			}
			break;

			default:
			if(mus->channel[chan].instrument != NULL || ((inst & 0xfff0) != MUS_FX_EXT_RETRIGGER && (inst & 0xff00) != MUS_FX_RESTART_PROGRAM))
			{
				do_command(mus, chan, chn->ops[i].program_counter[pr], inst, 1, i + 1, pr);
			}
			break;
		}
	}

	if (inst == MUS_FX_NOP || (inst & 0xff00) != MUS_FX_JUMP)
	{
		++tick;
		
		if (tick >= MUS_PROG_LEN)
		{
			tick = 0;
		}
	}
	
	if (unite_bit && inst != MUS_FX_NOP) //old command if ((inst & 0x8000) && inst != MUS_FX_NOP && !dont_reloop)
	{
		goto do_it_again4op;
	}
	
	if (advance)
	{
		chn->ops[i].program_tick[pr] = tick;
	}
}

static Sint8 mus_shape(Uint16 position, Uint8 shape)
{
	switch (shape)
	{
		case MUS_SHAPE_SINE:
			return sine_table[position & (VIB_TAB_SIZE - 1)];
			break;

		case MUS_SHAPE_SQUARE:
			return ((position & (VIB_TAB_SIZE - 1)) & (VIB_TAB_SIZE / 2)) ? -128 : 127;
			break;

		case MUS_SHAPE_RAMP_UP:
			return (position & (VIB_TAB_SIZE - 1)) * 2 - 128;
			break;

		case MUS_SHAPE_RAMP_DN:
			return 127 - (position & (VIB_TAB_SIZE - 1)) * 2;
			break;
			
		case MUS_SHAPE_TRI_UP:
			return ((position & (VIB_TAB_SIZE - 1)) & (VIB_TAB_SIZE / 2)) ? ((position & (VIB_TAB_SIZE - 1)) * 4) - 128 : 127 - (position & (VIB_TAB_SIZE - 1)) * 4;
			break;
			
		case MUS_SHAPE_TRI_DOWN:
			return ((position & (VIB_TAB_SIZE - 1)) & (VIB_TAB_SIZE / 2)) ? 127 - (position & (VIB_TAB_SIZE - 1)) * 4 : ((position & (VIB_TAB_SIZE - 1)) * 4) - 128;
			break;

		default:
		case MUS_SHAPE_RANDOM:
			return rnd_table[(position / 8) & (VIB_TAB_SIZE - 1)];
			break;
	}
}


#ifndef CYD_DISABLE_PWM

static void do_pwm(MusEngine* mus, int chan)
{
	MusChannel *chn = &mus->channel[chan];
	MusInstrument *ins = chn->instrument;
	MusTrackStatus *track_status = &mus->song_track[chan];

	track_status->pwm_position += track_status->pwm_speed;
	mus->cyd->channel[chan].pw = track_status->pw + mus_shape(track_status->pwm_position >> 1, ins->pwm_shape) * track_status->pwm_depth / 16; //mus->cyd->channel[chan].pw = track_status->pw + mus_shape(track_status->pwm_position >> 1, ins->pwm_shape) * track_status->pwm_depth / 32;
}

static void do_4op_pwm(MusEngine* mus, int chan, int i /*op number*/)
{
	MusChannel *chn = &mus->channel[chan];
	MusInstrument *ins = chn->instrument;
	MusTrackStatus *track_status = &mus->song_track[chan];

	track_status->ops_status[i].pwm_position += track_status->ops_status[i].pwm_speed;
	mus->cyd->channel[chan].fm.ops[i].pw = track_status->ops_status[i].pw + mus_shape(track_status->ops_status[i].pwm_position >> 1, ins->ops[i].pwm_shape) * track_status->ops_status[i].pwm_depth / 16; //mus->cyd->channel[chan].fm.ops[i].pw = track_status->ops_status[i].pw + mus_shape(track_status->ops_status[i].pwm_position >> 1, ins->ops[i].pwm_shape) * track_status->ops_status[i].pwm_depth / 32;
}

#endif

void mus_trigger_fm_op_internal(CydFm* fm, MusInstrument* ins, CydChannel* cydchn, MusChannel* chn, MusTrackStatus* track, MusEngine* mus, Uint8 i/*op number*/, Uint16 note, int chan, bool retrig, bool update_adsr)
{
	fm->ops[i].flags = ins->ops[i].cydflags;
	
	fm->ops[i].base_note = ins->ops[i].base_note;
	fm->ops[i].finetune = ins->ops[i].finetune;
	
	fm->ops[i].harmonic = ins->ops[i].harmonic;
	
	fm->ops[i].ssg_eg_type = ins->ops[i].ssg_eg_type;
	
	if (ins->ops[i].flags & MUS_FM_OP_SET_PW)
	{
		track->ops_status[i].pw = ins->pw;
		fm->ops[i].pw = ins->ops[i].pw;
	}
	
	if (fm->ops[i].flags & CYD_FM_OP_ENABLE_WAVE)
	{
		//cyd_set_wave_entry(cydchn, &mus->cyd->wavetable_entries[ins->wavetable_entry]);
		
		fm->ops[i].wave_entry = &mus->cyd->wavetable_entries[ins->ops[i].wavetable_entry];
		
		for (int s = 0; s < CYD_SUB_OSCS; ++s)
		{
			fm->ops[i].subosc[s].wave.playing = true;
			fm->ops[i].subosc[s].wave.acc = 0;
			fm->ops[i].subosc[s].wave.frequency = 0;
			fm->ops[i].subosc[s].wave.direction = 0;
		}
	}
	
	else
	{
		fm->ops[i].wave_entry = NULL;
	}
	
	fm->ops[i].detune = ins->ops[i].detune;
	fm->ops[i].coarse_detune = ins->ops[i].coarse_detune;
	
	fm->ops[i].feedback = ins->ops[i].feedback;
	
	fm->ops[i].vol_ksl_level = ins->ops[i].vol_ksl_level;
	fm->ops[i].env_ksl_level = ins->ops[i].env_ksl_level;
	
	fm->ops[i].mixmode = ins->ops[i].mixmode; //wasn't there
	fm->ops[i].sine_acc_shift = ins->ops[i].sine_acc_shift; //wasn't there
	fm->ops[i].flt_slope = ins->ops[i].slope;
	
	fm->ops[i].curr_tremolo = 0;
	fm->ops[i].tremolo = 0;
	fm->ops[i].prev_tremolo = 0;
	
	if(update_adsr)
	{
		fm->ops[i].adsr.a = ins->ops[i].adsr.a;
		fm->ops[i].adsr.d = ins->ops[i].adsr.d;
		fm->ops[i].adsr.s = ins->ops[i].adsr.s;
		
		fm->ops[i].adsr.sr = ins->ops[i].adsr.sr;
		
		fm->ops[i].adsr.r = ins->ops[i].adsr.r;
	}
	
	fm->ops[i].adsr.passes = 0;
	
	if (ins->ops[i].flags & MUS_FM_OP_DRUM)
	{
		fm->ops[i].flags |= CYD_FM_OP_ENABLE_NOISE;
	}
	
	if(fm->flags & CYD_FM_ENABLE_3CH_EXP_MODE)
	{
		if (ins->ops[i].flags & MUS_FM_OP_LOCK_NOTE)
		{
			note = ((Uint16)ins->ops[i].base_note) << 8;
		}
		
		else
		{
			if(retrig)
			{
				note = chn->ops[i].triggered_note + (Uint16)(((int)ins->ops[i].base_note - MIDDLE_C) << 8);
			}
			
			else
			{
				note += (Uint16)((int)ins->ops[i].base_note - MIDDLE_C) << 8;
			}
		}
	}
	
	else
	{
		if (ins->ops[i].flags & MUS_FM_OP_LOCK_NOTE)
		{
			note = ((Uint16)ins->base_note) << 8;
		}
		
		else
		{
			if(retrig)
			{
				note = chn->ops[i].triggered_note + (Uint16)(((int)ins->base_note - MIDDLE_C) << 8);
			}
			
			else
			{
				note += (Uint16)((int)ins->base_note - MIDDLE_C) << 8;
			}
		}
	}
	
	if(fm->ops[i].flags & CYD_FM_OP_ENABLE_FIXED_NOISE_PITCH)
	{
		chn->ops[i].noise_note = ins->ops[i].noise_note;
		
		Uint32 frequency = get_freq((chn->ops[i].noise_note << 8) / (((ins->fm_flags & CYD_FM_ENABLE_3CH_EXP_MODE) ? (ins->ops[i].flags & MUS_FM_OP_QUARTER_FREQ) : (ins->flags & MUS_INST_QUARTER_FREQ)) ? 4 : 1));
		
		if (frequency != 0)
		{
			for (int s = 0; s < CYD_SUB_OSCS; ++s)
			{
				fm->ops[i].subosc[s].noise_frequency = (Uint64)(ACC_LENGTH >> (mus->cyd->oversample)) / (Uint64)1024 * (Uint64)(frequency) / (Uint64)mus->cyd->sample_rate;
			}
		}
	}
	
	else
	{
		if(ins->fm_flags & CYD_FM_ENABLE_3CH_EXP_MODE)
		{
			chn->ops[i].noise_note = ins->ops[i].base_note;
			
			Uint32 frequency = get_freq((chn->ops[i].noise_note << 8) / (((ins->fm_flags & CYD_FM_ENABLE_3CH_EXP_MODE) ? (ins->ops[i].flags & MUS_FM_OP_QUARTER_FREQ) : (ins->flags & MUS_INST_QUARTER_FREQ)) ? 4 : 1));
			
			if (frequency != 0)
			{
				for (int s = 0; s < CYD_SUB_OSCS; ++s)
				{
					fm->ops[i].subosc[s].noise_frequency = (Uint64)(ACC_LENGTH >> (mus->cyd->oversample)) / (Uint64)1024 * (Uint64)(frequency) / (Uint64)mus->cyd->sample_rate;
				}
			}
		}
		
		else
		{
			chn->ops[i].noise_note = ins->base_note;
			
			Uint32 frequency;
			
			if(ins->fm_freq_LUT == 0)
			{
				frequency = get_freq((chn->ops[i].noise_note << 8) / (((ins->fm_flags & CYD_FM_ENABLE_3CH_EXP_MODE) ? (ins->ops[i].flags & MUS_FM_OP_QUARTER_FREQ) : (ins->flags & MUS_INST_QUARTER_FREQ)) ? 4 : 1)) * (Uint64)harmonic1[fm->ops[i].harmonic & 15] / (Uint64)harmonic1[fm->ops[i].harmonic >> 4];
			}
			
			else
			{
				frequency = get_freq((chn->ops[i].noise_note << 8) / (((ins->fm_flags & CYD_FM_ENABLE_3CH_EXP_MODE) ? (ins->ops[i].flags & MUS_FM_OP_QUARTER_FREQ) : (ins->flags & MUS_INST_QUARTER_FREQ)) ? 4 : 1)) * (Uint64)harmonicOPN1[fm->ops[i].harmonic & 15] / (Uint64)harmonicOPN1[fm->ops[i].harmonic >> 4];
			}
			
			if (frequency != 0)
			{
				for (int s = 0; s < CYD_SUB_OSCS; ++s)
				{
					fm->ops[i].subosc[s].noise_frequency = (Uint64)(ACC_LENGTH >> (mus->cyd->oversample)) / (Uint64)1024 * (Uint64)(frequency) / (Uint64)mus->cyd->sample_rate;
				}
			}
		}
	}
	
	if(fm->flags & CYD_FM_ENABLE_3CH_EXP_MODE)
	{
		chn->ops[i].last_note = chn->ops[i].target_note = ((Uint32)note + fm->ops[i].finetune);
	}
	
	else
	{
		//debug("last note before, %d op %d", chn->ops[i].last_note, i);
		
		chn->ops[i].last_note = chn->ops[i].target_note = ((Uint32)note + (Sint32)fm->ops[i].detune * DETUNE + coarse_detune_table[fm->ops[i].coarse_detune]);// * harmonic1[fm->ops[i].harmonic & 15] / harmonic1[fm->ops[i].harmonic >> 4];
		
		//debug("last note after, %d op %d", chn->ops[i].last_note, i);
	}
	
	fm->ops[i].freq_for_ksl = get_freq(chn->ops[i].last_note);
	//fm->ops[i].freq_for_ksl = get_freq(chn->ops[i].last_note);
	
	chn->ops[i].arpeggio_note = 0;
	chn->ops[i].fixed_note = 0xffff;
	
	chn->ops[i].current_tick = 0;
	
	for(int pr = 0; pr < ins->ops[i].num_macros; ++pr)
	{
		chn->ops[i].prog_period[pr] = ins->ops[i].prog_period[pr];
	}
	
	if (!(ins->ops[i].flags & MUS_FM_OP_NO_PROG_RESTART))
	{
		for(int pr = 0; pr < ins->ops[i].num_macros; ++pr)
		{
			chn->ops[i].program_counter[pr] = 0;
			chn->ops[i].program_tick[pr] = 0;
			//chn->ops[i].program_loop = 1;
			
			for(int j = 0; j < MUS_MAX_NESTEDNESS; ++j)
			{
				chn->ops[i].program_loop[pr][j] = 1;
				chn->ops[i].program_loop_addresses[pr][j][0] = chn->ops[i].program_loop_addresses[pr][j][1] = 0;
			}
			
			chn->ops[i].nestedness[pr] = 0;
			
			int j = 1;
			int temp_address = 0;
			
			while(j < MUS_MAX_NESTEDNESS && temp_address < MUS_PROG_LEN)
			{
				if(chn->instrument->ops[i].program[pr][temp_address] == MUS_FX_LABEL)
				{
					chn->ops[i].program_loop_addresses[pr][j][0] = temp_address;
					j++;
				}
				
				if(chn->instrument->ops[i].program[pr][temp_address] == MUS_FX_LOOP)
				{
					goto loops;
				}
				
				else
				{
					temp_address++;
				}
			}
			
			loops:;
		}
	}
	
	for(int pr = 0; pr < ins->ops[i].num_macros; ++pr)
	{
		if (ins->ops[i].prog_period[pr] > 0)
		{
			//chn->ops[i].flags |= MUS_FM_OP_PROGRAM_RUNNING;
			chn->ops[i].program_flags |= (1 << pr);
		}
	}

	track->ops_status[i].vibrato_position = 0;
	track->ops_status[i].tremolo_position = 0;
	
	if (fm->ops[i].flags & CYD_FM_OP_ENABLE_KEY_SYNC)
	{
		track->ops_status[i].pwm_position = 0;
	}
	
	track->ops_status[i].pwm_speed = ins->ops[i].pwm_speed; //wasn't there
	track->ops_status[i].pwm_depth = ins->ops[i].pwm_depth; //wasn't there
	track->ops_status[i].pwm_delay = ins->ops[i].pwm_delay; //wasn't there
	
	track->ops_status[i].vibrato_depth = ins->ops[i].vibrato_depth;
	track->ops_status[i].vibrato_speed = ins->ops[i].vibrato_speed;
	track->ops_status[i].vibrato_delay = ins->ops[i].vibrato_delay;
	
	track->ops_status[i].tremolo_depth = ins->ops[i].tremolo_depth;
	track->ops_status[i].tremolo_speed = ins->ops[i].tremolo_speed;
	track->ops_status[i].tremolo_delay = ins->ops[i].tremolo_delay;

	track->ops_status[i].slide_speed = 0;
	
	update_fm_op_volume(mus, track, chn, cydchn, ((ins->ops[i].flags & MUS_FM_OP_RELATIVE_VOLUME) ? MAX_VOLUME : ins->ops[i].volume), i);
	
	if(fm->ops[i].flags & CYD_FM_OP_ENABLE_KEY_SYNC)
	{
		for (int s = 0; s < CYD_SUB_OSCS; ++s)
		{
			fm->ops[i].subosc[s].accumulator = 0;
			fm->ops[i].subosc[s].noise_accumulator = 0;
			fm->ops[i].subosc[s].wave.acc = 0;
		}
	}
	
	for (int s = 0; s < CYD_SUB_OSCS; ++s)
	{
		fm->ops[i].subosc[s].random = RANDOM_SEED;
	}

	fm->ops[i].sync_source = ins->ops[i].sync_source == 0xff ? chan : ins->ops[i].sync_source;
	fm->ops[i].ring_mod = ins->ops[i].ring_mod == 0xff ? chan : ins->ops[i].ring_mod;

	if (fm->ops[i].ring_mod >= mus->cyd->n_channels)
	{
		fm->ops[i].ring_mod = mus->cyd->n_channels - 1;
	}

	if (fm->ops[i].sync_source >= mus->cyd->n_channels)
	{
		fm->ops[i].sync_source = mus->cyd->n_channels - 1;
	}

	fm->ops[i].flttype = ins->ops[i].flttype;

#ifndef CYD_DISABLE_FILTER
	if (ins->ops[i].flags & MUS_FM_OP_SET_CUTOFF)
	{
		track->ops_status[i].filter_cutoff = ins->ops[i].cutoff;
		track->ops_status[i].filter_resonance = ins->ops[i].resonance;
		track->ops_status[i].filter_slope = ins->ops[i].slope;
		
		for(int j = 0; j < (int)pow(2, fm->ops[i].flt_slope); j++)
		{
			for (int sub = 0; sub < CYD_SUB_OSCS; ++sub)
			{
				if(mus->cyd->flags & CYD_USE_OLD_FILTER)
				{
					cydflt_set_coeff_old(&fm->ops[i].flts[j][sub], ins->ops[i].cutoff, resonance_table[ins->ops[i].resonance & 3]);
				}
				
				else
				{
					cydflt_set_coeff(&fm->ops[i].flts[j][sub], ins->ops[i].cutoff, ins->ops[i].resonance, mus->cyd->sample_rate);
				}
			}
		}
	}
#endif

	if (ins->ops[i].flags & MUS_FM_OP_SET_PW)
	{
		track->ops_status[i].pw = ins->ops[i].pw;
#ifndef CYD_DISABLE_PWM
		if(track->ops_status[i].pwm_delay == 0)
		{
			//do_pwm(mus,chan);
			do_4op_pwm(mus, chan, i);
		}
#endif
	}
	
	mus_set_fm_op_note(mus, chan, fm, chn->ops[i].last_note, i /*op number*/, 1, 1, ins);
	
	if(ins->ops[i].cydflags & CYD_FM_OP_ENABLE_CSM_TIMER)
	{
		chn->ops[i].CSM_timer_note = ((Uint16)ins->ops[i].CSM_timer_note << 8) + ins->ops[i].CSM_timer_finetune;
		Uint32 frequency = get_freq(chn->ops[i].CSM_timer_note);
		
		fm->ops[i].csm.frequency = (Uint64)(ACC_LENGTH) / (Uint64)1024 * (Uint64)(frequency) / (Uint64)mus->cyd->sample_rate;
	}
	
	if((ins->ops[i].flags & MUS_FM_OP_LINK_CSM_TIMER_NOTE) && (ins->ops[i].cydflags & CYD_FM_OP_ENABLE_CSM_TIMER))
	{
		chn->ops[i].CSM_timer_note = ((ins->ops[i].CSM_timer_note << 8) + ins->ops[i].CSM_timer_finetune) + chn->ops[i].note - ((cydchn->fm.flags & CYD_FM_ENABLE_3CH_EXP_MODE) ? ((ins->ops[i].base_note << 8) + ins->ops[i].finetune) : ((ins->base_note << 8) + ins->finetune));
		
		Uint32 frequency = get_freq(chn->ops[i].CSM_timer_note);
		
		cydchn->fm.ops[i].csm.frequency = (Uint64)(ACC_LENGTH) / (Uint64)1024 * (Uint64)(frequency) / (Uint64)mus->cyd->sample_rate;
	}
	
	if(!(ins->ops[i].cydflags & CYD_FM_OP_ENABLE_CSM_TIMER))
	{
		fm->ops[i].csm.frequency = 0;
	}
	
	for (int s = 0; s < CYD_SUB_OSCS; ++s)
	{
		fm->ops[i].subosc[s].wave.end_offset = 0xffff;
		fm->ops[i].subosc[s].wave.start_offset = 0;
		
		fm->ops[i].subosc[s].wave.start_point_track_status = 0;
		fm->ops[i].subosc[s].wave.end_point_track_status = 0;
		
		fm->ops[i].subosc[s].wave.use_start_track_status_offset = false;
		fm->ops[i].subosc[s].wave.use_end_track_status_offset = false;
	}
	
	fm->ops[i].adsr.use_volume_envelope = false;
	
	if(ins->ops[i].flags & MUS_FM_OP_USE_VOLUME_ENVELOPE)
	{
		fm->ops[i].adsr.num_vol_points = ins->ops[i].num_vol_points;
		
		fm->ops[i].adsr.vol_env_flags = ins->ops[i].vol_env_flags;
		fm->ops[i].adsr.vol_env_sustain = ins->ops[i].vol_env_sustain;
		fm->ops[i].adsr.vol_env_loop_start = ins->ops[i].vol_env_loop_start;
		fm->ops[i].adsr.vol_env_loop_end = ins->ops[i].vol_env_loop_end;
																								//this is coefficient to adjust (more or less) precisely to FT2 timing
		fm->ops[i].adsr.vol_env_fadeout = (Uint32)((double)(ins->ops[i].vol_env_fadeout << 7) * 48000.0 / (double)mus->cyd->sample_rate * 1390.0 / 2485.0);
		
		fm->ops[i].adsr.current_vol_env_point = 0;
		fm->ops[i].adsr.next_vol_env_point = 1;
		
		fm->ops[i].adsr.envelope = 0;
		fm->ops[i].adsr.volume_envelope_output = 0;
		fm->ops[i].adsr.use_volume_envelope = true;
		fm->ops[i].adsr.advance_volume_envelope = true;
		
		fm->ops[i].adsr.curr_vol_fadeout_value = 0x7FFFFFFF;
		
		//ACC_LENGTH is (1 << 16) * 100
		//since we want 100 deltax passed at 1 second
		//so frequency is 1 hz
		//(Uint64)(ACC_LENGTH) * (Uint64)(frequency) / (Uint64)cyd->sample_rate;
		fm->ops[i].adsr.env_speed = (Uint64)((1 << 16) * 100) * (Uint64)(1) / (Uint64)mus->cyd->sample_rate;
		
		for(int j = 0; j < ins->ops[i].num_vol_points; ++j)
		{
			fm->ops[i].adsr.volume_envelope[j].x = (Uint32)ins->ops[i].volume_envelope[j].x << 16;
			fm->ops[i].adsr.volume_envelope[j].y = (Uint16)ins->ops[i].volume_envelope[j].y << 8;
		}
	}
}


//***** USE THIS INSIDE MUS_ADVANCE_TICK TO AVOID MUTEX DEADLOCK
int mus_trigger_instrument_internal(MusEngine* mus, int chan, MusInstrument *ins, Uint16 note, int panning, bool update_adsr)
{
	mus->cyd->mus_volume = mus->volume;
	
	Uint16 temp_note = note;
	
	if (chan == -1)
	{
		for (int i = 0; i < mus->cyd->n_channels; ++i)
		{
			if (!(mus->cyd->channel[i].flags & CYD_CHN_ENABLE_GATE) && !(mus->channel[i].flags & MUS_CHN_DISABLED))
			{
				chan = i;
			}
		}

		if (chan == -1)
		{
			chan = (rand() % mus->cyd->n_channels);
		}
	}

	CydChannel *cydchn = &mus->cyd->channel[chan];
	MusChannel *chn = &mus->channel[chan];
	MusTrackStatus *track = &mus->song_track[chan];

	chn->flags = MUS_CHN_PLAYING | (chn->flags & MUS_CHN_DISABLED);
	
	for(int pr = 0; pr < ins->num_macros; ++pr)
	{
		if (ins->prog_period[pr] > 0)
		{
			//chn->flags |= MUS_CHN_PROGRAM_RUNNING;
			chn->program_flags |= (1 << pr);
		}
	}
	
	for(int pr = 0; pr < ins->num_macros; ++pr)
	{
		chn->prog_period[pr] = ins->prog_period[pr];
	}
	
	chn->instrument = ins;
	
	cydchn->base_note = ins->base_note;
	cydchn->finetune = ins->finetune;
	
	if (!(ins->flags & MUS_INST_NO_PROG_RESTART))
	{
		for(int pr = 0; pr < ins->num_macros; ++pr)
		{
			chn->program_counter[pr] = 0;
			chn->program_tick[pr] = 0;
			
			for(int i = 0; i < MUS_MAX_NESTEDNESS; ++i)
			{
				chn->program_loop[pr][i] = 1;
				chn->program_loop_addresses[pr][i][0] = chn->program_loop_addresses[pr][i][1] = 0;
			}
			
			chn->nestedness[pr] = 0;
			
			int i = 1;
			int temp_address = 0;
			
			while(i < MUS_MAX_NESTEDNESS && temp_address < MUS_PROG_LEN)
			{
				if(chn->instrument->program[pr][temp_address] == MUS_FX_LABEL)
				{
					chn->program_loop_addresses[pr][i][0] = temp_address;
					i++;
				}
				
				if(chn->instrument->program[pr][temp_address] == MUS_FX_LOOP)
				{
					goto loops;
				}
				
				else
				{
					temp_address++;
				}
			}
			
			loops:;
		}
	}
	
	cydchn->flags = ins->cydflags;
	
	cydchn->musflags = ins->flags;
	
	cydchn->vol_ksl_level = ins->vol_ksl_level;
	cydchn->env_ksl_level = ins->env_ksl_level;
	
	cydchn->mixmode = ins->mixmode; //wasn't there
	cydchn->sine_acc_shift = ins->sine_acc_shift; //wasn't there
	cydchn->flt_slope = ins->slope;
	
	cydchn->curr_tremolo = 0;
	cydchn->tremolo = 0;
	cydchn->prev_tremolo = 0;
	
	chn->arpeggio_note = 0;
	chn->fixed_note = 0xffff;
	cydchn->fx_bus = ins->fx_bus;

	if (ins->flags & MUS_INST_DRUM)
	{
		cyd_set_waveform(cydchn, CYD_CHN_ENABLE_NOISE);
	}

	if (ins->flags & MUS_INST_LOCK_NOTE)
	{
		note = ((Uint16)ins->base_note) << 8;
	}
	
	else
	{
		note += (Uint16)((int)ins->base_note - MIDDLE_C) << 8;
	}
	
	if(cydchn->flags & CYD_CHN_ENABLE_FIXED_NOISE_PITCH)
	{
		mus_set_noise_fixed_pitch_note(mus, chan, (Uint16)(ins->noise_note << 8), 1, ins->flags & MUS_INST_QUARTER_FREQ ? 4 : 1);
		chn->noise_note = ins->noise_note;
	}
	
	else
	{
		mus_set_noise_fixed_pitch_note(mus, chan, (Uint16)(ins->base_note << 8), 1, ins->flags & MUS_INST_QUARTER_FREQ ? 4 : 1);
		chn->noise_note = ins->base_note;
	}

	mus_set_note(mus, chan, ((Uint16)note) + ins->finetune, 1, ins->flags & MUS_INST_QUARTER_FREQ ? 4 : 1);
	
	cydchn->vol_ksl_mult = cydchn->env_ksl_mult = 1.0;
	
	if((cydchn->flags & CYD_CHN_ENABLE_VOLUME_KEY_SCALING) || (cydchn->flags & CYD_CHN_ENABLE_ENVELOPE_KEY_SCALING))
	{
		Sint16 vol_ksl_level_final = (cydchn->flags & CYD_CHN_ENABLE_VOLUME_KEY_SCALING) ? cydchn->vol_ksl_level : -1;
		
		cydchn->vol_ksl_mult = (vol_ksl_level_final == -1) ? 1.0 : (pow((get_freq((cydchn->base_note << 8) + cydchn->finetune) + 1.0) / (get_freq(cydchn->freq_for_ksl) + 1.0), (vol_ksl_level_final == 0 ? 0 : (vol_ksl_level_final / 127.0))));
		
		Sint16 env_ksl_level_final = (cydchn->flags & CYD_CHN_ENABLE_ENVELOPE_KEY_SCALING) ? cydchn->env_ksl_level : -1;
		
		cydchn->env_ksl_mult = (env_ksl_level_final == -1) ? 1.0 : (pow((get_freq((cydchn->base_note << 8) + cydchn->finetune) + 1.0) / (get_freq(cydchn->freq_for_ksl) + 1.0), (env_ksl_level_final == 0 ? 0 : (env_ksl_level_final / 127.0))));
		cydchn->env_ksl_mult = 1.0 / cydchn->env_ksl_mult;
	}
	
	chn->last_note = chn->target_note = (((Uint32)note) + ins->finetune);
	chn->current_tick = 0;

	track->vibrato_position = 0;
	track->tremolo_position = 0;

	track->vibrato_delay = ins->vibrato_delay;
	track->fm_vibrato_delay = ins->fm_vibrato_delay;
	
	track->pwm_delay = ins->pwm_delay; //wasn't there
	track->tremolo_delay = ins->tremolo_delay;
	track->fm_tremolo_delay = ins->fm_tremolo_delay;
	
	track->pwm_speed = ins->pwm_speed; //wasn't there
	track->pwm_depth = ins->pwm_depth; //wasn't there
	
	track->vibrato_depth = ins->vibrato_depth;
	track->vibrato_speed = ins->vibrato_speed;
	
	track->tremolo_depth = ins->tremolo_depth;
	track->tremolo_speed = ins->tremolo_speed;

	track->slide_speed = 0;

	update_volumes(mus, track, chn, cydchn, (ins->flags & MUS_INST_RELATIVE_VOLUME) ? MAX_VOLUME : ins->volume);

	cydchn->sync_source = ins->sync_source == 0xff ? chan : ins->sync_source;
	cydchn->ring_mod = ins->ring_mod == 0xff ? chan : ins->ring_mod;

	if (cydchn->ring_mod >= mus->cyd->n_channels)
	{
		cydchn->ring_mod = mus->cyd->n_channels - 1;
	}

	if (cydchn->sync_source >= mus->cyd->n_channels)
	{
		cydchn->sync_source = mus->cyd->n_channels - 1;
	}

	cydchn->flttype = ins->flttype;
	cydchn->lfsr_type = ins->lfsr_type;

	if (ins->cydflags & CYD_CHN_ENABLE_KEY_SYNC)
	{
		track->pwm_position = 0;
	}

#ifndef CYD_DISABLE_FILTER
	if (ins->flags & MUS_INST_SET_CUTOFF)
	{
		track->filter_cutoff = ins->cutoff;
		track->filter_resonance = ins->resonance;
		track->filter_slope = ins->slope;
		
		cyd_set_filter_coeffs(mus->cyd, cydchn, ins->cutoff, ins->resonance);
	}
#endif

	if (ins->flags & MUS_INST_SET_PW)
	{
		track->pw = ins->pw;
#ifndef CYD_DISABLE_PWM
		if(track->pwm_delay == 0)
		{
			do_pwm(mus, chan);
		}
#endif
	}

	if (ins->flags & MUS_INST_YM_BUZZ)
	{
#ifndef CYD_DISABLE_BUZZ
		cydchn->flags |= CYD_CHN_ENABLE_YM_ENV;
		cyd_set_env_shape(cydchn, ins->ym_env_shape);
		mus->channel[chan].buzz_offset = ins->buzz_offset;
#endif
	}
	
	else
	{
		cydchn->flags &= ~CYD_CHN_ENABLE_YM_ENV;
		
		if(update_adsr)
		{
			cydchn->adsr.a = ins->adsr.a;
			cydchn->adsr.d = ins->adsr.d;
			cydchn->adsr.s = ins->adsr.s;
			cydchn->adsr.r = ins->adsr.r;
		}
	}

#ifndef CYD_DISABLE_WAVETABLE
	if (ins->cydflags & CYD_CHN_ENABLE_WAVE)
	{
		cyd_set_wave_entry(cydchn, &mus->cyd->wavetable_entries[ins->wavetable_entry]);
	}
	
	else
	{
		cyd_set_wave_entry(cydchn, NULL);
	}
	
	for(int i = 0; i < CYD_SUB_OSCS; ++i)
	{
		cydchn->subosc[i].wave.end_offset = 0xffff;
		cydchn->subosc[i].wave.start_offset = 0;
		
		cydchn->subosc[i].wave.start_point_track_status = 0;
		cydchn->subosc[i].wave.end_point_track_status = 0;
		
		cydchn->subosc[i].wave.use_start_track_status_offset = false;
		cydchn->subosc[i].wave.use_end_track_status_offset = false;
		
		cydchn->subosc[i].accumulator = 0;
		cydchn->subosc[i].wave.acc = 0;
	}

#ifndef CYD_DISABLE_FM
	if(ins->cydflags & CYD_CHN_ENABLE_FM)
	{
		if (ins->fm_flags & CYD_FM_ENABLE_WAVE)
		{
			cydfm_set_wave_entry(&cydchn->fm, &mus->cyd->wavetable_entries[ins->fm_wave]);
		}
		
		else
		{
			cydfm_set_wave_entry(&cydchn->fm, NULL);
		}
		
		cydchn->fm.wave.end_offset = 0xffff;
		cydchn->fm.wave.start_offset = 0;
		
		cydchn->fm.wave.start_point_track_status = 0;
		cydchn->fm.wave.end_point_track_status = 0;
		
		cydchn->fm.wave.use_start_track_status_offset = false;
		cydchn->fm.wave.use_end_track_status_offset = false;
	}
#endif
#endif

#ifdef STEREOOUTPUT
	if (panning != -1)
	{
		cyd_set_panning(mus->cyd, cydchn, panning);
		
		//debug("set panning %d", panning);
		
		cydchn->init_panning = panning;
	}

#endif

#ifndef CYD_DISABLE_FM
	cydchn->fm.flags = 0;
	
	if(ins->cydflags & CYD_CHN_ENABLE_FM)
	{
		CydFm *fm = &cydchn->fm;

		fm->flags = ins->fm_flags;
		fm->harmonic = ins->fm_harmonic;
		
		fm->fm_freq_LUT = ins->fm_freq_LUT;
		
		if(update_adsr)
		{
			fm->adsr.a = ins->fm_adsr.a;
			fm->adsr.d = ins->fm_adsr.d;
			fm->adsr.s = ins->fm_adsr.s;
			fm->adsr.r = ins->fm_adsr.r;
		}
		
		fm->adsr.volume = ins->fm_modulation;
		
		track->fm_volume = ins->fm_modulation;
		
		fm->fm_vol_ksl_level = ins->fm_vol_ksl_level;
		fm->fm_env_ksl_level = ins->fm_env_ksl_level;
		
		fm->feedback = ins->fm_feedback;
		fm->attack_start = ins->fm_attack_start;
		
		fm->fm_base_note = ins->fm_base_note; //weren't there
		fm->fm_finetune = ins->fm_finetune;
		fm->fm_carrier_base_note = ins->base_note;
		fm->fm_carrier_finetune = ins->finetune;
		
		track->fm_vibrato_position = 0;
		track->fm_tremolo_position = 0;
		
		track->fm_vibrato_depth = ins->fm_vibrato_depth;
		track->fm_vibrato_speed = ins->fm_vibrato_speed;
		
		track->fm_tremolo_depth = ins->fm_tremolo_depth;
		track->fm_tremolo_speed = ins->fm_tremolo_speed;
		
		fm->fm_curr_tremolo = 0;
		fm->fm_tremolo = 0;
		fm->fm_prev_tremolo = 0;
		
		if(ins->fm_flags & CYD_FM_ENABLE_4OP)
		{
			fm->alg = ins->alg;
			
			track->fm_4op_vol = ins->fm_4op_vol;
			fm->fm_4op_vol = ins->fm_4op_vol;
			
			for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
			{
				chn->ops[i].trigger_delay = ins->ops[i].trigger_delay;
				cydchn->fm.ops[i].trigger_delay = ins->ops[i].trigger_delay;
				
				//debug("trig inst trig del %d op %d", cydchn->fm.ops[i].trigger_delay, i);
				
				chn->ops[i].triggered_note = temp_note;
				
				if(chn->ops[i].trigger_delay == 0)
				{
					mus_trigger_fm_op_internal(fm, ins, cydchn, chn, track, mus, i, temp_note, chan, 0, update_adsr);
				}
				
				chn->ops[i].trigger_delay--;
			}
		}
	}
#endif

	//cyd_set_frequency(mus->cyd, cydchn, chn->frequency);
	
	cyd_enable_gate(mus->cyd, cydchn, 1);
	
	if(mus->song)
	{
		if(cydchn->adsr.use_panning_envelope && (cydchn->init_panning != mus->song->default_panning[chan]))
		{
			cyd_set_panning(mus->cyd, cydchn, mus->song->default_panning[chan] + CYD_PAN_CENTER);
		}
	}
	
	cydchn->adsr.use_volume_envelope = false;
	cydchn->adsr.use_panning_envelope = false;
	
	if(ins->flags & MUS_INST_USE_VOLUME_ENVELOPE)
	{
		cydchn->adsr.num_vol_points = ins->num_vol_points;
		
		cydchn->adsr.vol_env_flags = ins->vol_env_flags;
		cydchn->adsr.vol_env_sustain = ins->vol_env_sustain;
		cydchn->adsr.vol_env_loop_start = ins->vol_env_loop_start;
		cydchn->adsr.vol_env_loop_end = ins->vol_env_loop_end;
		
																												//this is coefficient to adjust (more or less) precisely to FT2 timing
		cydchn->adsr.vol_env_fadeout = (Uint32)((double)(ins->vol_env_fadeout << 7) * 48000.0 / (double)mus->cyd->sample_rate * 1390.0 / 2485.0);
		
		cydchn->adsr.current_vol_env_point = 0;
		cydchn->adsr.next_vol_env_point = 1;
		
		cydchn->adsr.envelope = 0;
		cydchn->adsr.volume_envelope_output = 0;
		cydchn->adsr.use_volume_envelope = true;
		cydchn->adsr.advance_volume_envelope = true;
		
		cydchn->adsr.curr_vol_fadeout_value = 0x7FFFFFFF;
		
		//ACC_LENGTH is (1 << 16) * 100
		//since we want 100 deltax passed at 1 second
		//so frequency is 1 hz
		//(Uint64)(ACC_LENGTH) * (Uint64)(frequency) / (Uint64)cyd->sample_rate;
		cydchn->adsr.env_speed = (Uint64)((1 << 16) * 100) * (Uint64)(1) / (Uint64)mus->cyd->sample_rate;
		
		for(int i = 0; i < ins->num_vol_points; ++i)
		{
			cydchn->adsr.volume_envelope[i].x = (Uint32)ins->volume_envelope[i].x << 16;
			cydchn->adsr.volume_envelope[i].y = (Uint16)ins->volume_envelope[i].y << 8;
		}
	}
	
	if(ins->flags & MUS_INST_USE_PANNING_ENVELOPE)
	{
		cydchn->adsr.num_pan_points = ins->num_pan_points;
		
		cydchn->adsr.pan_env_flags = ins->pan_env_flags;
		cydchn->adsr.pan_env_sustain = ins->pan_env_sustain;
		cydchn->adsr.pan_env_loop_start = ins->pan_env_loop_start;
		cydchn->adsr.pan_env_loop_end = ins->pan_env_loop_end;
		
		cydchn->adsr.pan_env_fadeout = (Uint32)((double)(ins->pan_env_fadeout << 7) * 48000.0 / (double)mus->cyd->sample_rate * 1390.0 / 2485.0);
		
		cydchn->adsr.current_pan_env_point = 0;
		cydchn->adsr.next_pan_env_point = 1;
		
		cydchn->adsr.pan_envelope = 0;
		cydchn->adsr.panning_envelope_output = 0;
		cydchn->adsr.use_panning_envelope = true;
		cydchn->adsr.advance_panning_envelope = true;
		
		cydchn->adsr.curr_pan_fadeout_value = 0x7FFFFFFF;
		
		cydchn->adsr.pan_env_speed = (Uint64)((1 << 16) * 100) * (Uint64)(1) / (Uint64)mus->cyd->sample_rate;
		
		for(int i = 0; i < ins->num_pan_points; ++i)
		{
			cydchn->adsr.panning_envelope[i].x = (Uint32)ins->panning_envelope[i].x << 16;
			cydchn->adsr.panning_envelope[i].y = (Uint16)ins->panning_envelope[i].y << 7;
		}
	}

	return chan;
}


int mus_trigger_instrument(MusEngine* mus, int chan, MusInstrument *ins, Uint16 note, int panning)
{
	cyd_lock(mus->cyd, 1);

	chan = mus_trigger_instrument_internal(mus, chan, ins, note, panning, true);

	cyd_lock(mus->cyd, 0);

	return chan;
}


static void mus_advance_channel(MusEngine* mus, int chan)
{
	MusChannel *chn = &mus->channel[chan];
	MusTrackStatus *track_status = &mus->song_track[chan];

	if ((!(mus->cyd->channel[chan].flags & CYD_CHN_ENABLE_GATE) && !(mus->cyd->channel[chan].flags & CYD_CHN_ENABLE_FM)) || ((mus->cyd->channel[chan].flags & CYD_CHN_ENABLE_FM) && (mus->cyd->channel[chan].fm.adsr.envelope == 0) && !(mus->cyd->channel[chan].flags & CYD_CHN_ENABLE_GATE)))
	{
		if(mus->cyd->channel[chan].fm.flags & CYD_FM_ENABLE_4OP)
		{
			int ops_playing = 0;
			
			for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
			{
				if(mus->cyd->channel[chan].fm.ops[i].adsr.envelope > 0 || (mus->cyd->channel[chan].fm.ops[i].flags & CYD_FM_OP_ENABLE_GATE)) //so if CSM timer plays it still executes effects damn
				{
					ops_playing++;
				}
			}
			
			if(ops_playing == 0)
			{
				chn->flags &= ~MUS_CHN_PLAYING;
				return;
			}
		}
		
		else
		{
			chn->flags &= ~MUS_CHN_PLAYING;
			return;
		}
	}

	MusInstrument *ins = chn->instrument;

	if (ins->flags & MUS_INST_DRUM && chn->current_tick == 1)
	{
		cyd_set_waveform(&mus->cyd->channel[chan], ins->cydflags);
	}

	if (track_status->slide_speed != 0)
	{
		if (chn->target_note > chn->note)
		{
			chn->note += my_min((Uint32)track_status->slide_speed, chn->target_note - chn->note);
		}
		
		else if (chn->target_note < chn->note)
		{
			chn->note -= my_min((Uint32)track_status->slide_speed, chn->note - chn->target_note);
		}
	}

	++chn->current_tick;

	if (mus->channel[chan].program_flags)
	{
		for(int pr = 0; pr < mus->channel[chan].instrument->num_macros; ++pr)
		{
			if(mus->channel[chan].program_flags & (1 << pr))
			{
				int u = (chn->program_counter[pr] + 1) >= chn->prog_period[pr];
				mus_exec_prog_tick(mus, chan, u, pr);
				++chn->program_counter[pr];
				if (u) chn->program_counter[pr] = 0;
			}
		}
	}
	
	if(mus->cyd->channel[chan].fm.flags & CYD_FM_ENABLE_4OP)
	{
		mus->cyd->channel[chan].fm.fm_4op_vol = track_status->fm_4op_vol;
		
		for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
		{
			if(mus->channel[chan].ops[i].program_flags)
			{
				for(int pr = 0; pr < mus->channel[chan].instrument->ops[i].num_macros; ++pr)
				{
					if(mus->channel[chan].ops[i].program_flags & (1 << pr))
					{
						int u = (chn->ops[i].program_counter[pr] + 1) >= chn->ops[i].prog_period[pr];
						mus_exec_4op_prog_tick(mus, chan, u, i, pr);
						++chn->ops[i].program_counter[pr];
						if (u) chn->ops[i].program_counter[pr] = 0;
					}
				}
			}
			
			if ((ins->ops[i].flags & MUS_FM_OP_DRUM) && chn->ops[i].current_tick == 1)
			{
				//cyd_set_waveform(&mus->cyd->channel[chan], ins->cydflags);
				mus->cyd->channel[chan].fm.ops[i].flags = (mus->cyd->channel[chan].fm.ops[i].flags & (~WAVEFORMS)) | (ins->ops[i].cydflags & WAVEFORMS);
			}
			
			++chn->ops[i].current_tick;
			
			if(chn->ops[i].trigger_delay == 0)
			{
				mus_trigger_fm_op_internal(&mus->cyd->channel[chan].fm, chn->instrument, &mus->cyd->channel[chan], chn, track_status, mus, i, 0, chan, 1, mus->cyd->channel[chan].fm.update_ops_adsr);
				
				mus->cyd->channel[chan].fm.ops[i].adsr.envelope_state = ATTACK;
				
				if(!(ins->ops[i].flags & MUS_FM_OP_USE_VOLUME_ENVELOPE))
				{
					mus->cyd->channel[chan].fm.ops[i].adsr.envelope = 0x0;
					
					mus->cyd->channel[chan].fm.ops[i].adsr.env_speed = envspd(mus->cyd, mus->cyd->channel[chan].fm.ops[i].adsr.a);
					
					if(mus->cyd->channel[chan].fm.ops[i].env_ksl_mult != 0.0 && mus->cyd->channel[chan].fm.ops[i].env_ksl_mult != 1.0)
					{
						mus->cyd->channel[chan].fm.ops[i].adsr.env_speed = (int)((double)envspd(mus->cyd, mus->cyd->channel[chan].fm.ops[i].adsr.a) * mus->cyd->channel[chan].fm.ops[i].env_ksl_mult);
					}
				}
				
				//cyd_cycle_adsr(mus->cyd, 0, 0, &mus->cyd->channel[chan].fm.ops[i].adsr, mus->cyd->channel[chan].fm.ops[i].env_ksl_mult);
				cyd_cycle_fm_op_adsr(mus->cyd, 0, 0, &mus->cyd->channel[chan].fm.ops[i].adsr, mus->cyd->channel[chan].fm.ops[i].env_ksl_mult,  mus->cyd->channel[chan].fm.ops[i].ssg_eg_type | ((( mus->cyd->channel[chan].fm.ops[i].flags & CYD_FM_OP_ENABLE_SSG_EG) ? 1 : 0) << 3));
				
				for (int s = 0; s < CYD_SUB_OSCS; ++s)
				{
					mus->cyd->channel[chan].fm.ops[i].subosc[s].accumulator = 0;
					mus->cyd->channel[chan].fm.ops[i].subosc[s].noise_accumulator = 0;
					mus->cyd->channel[chan].fm.ops[i].subosc[s].wave.acc = 0;
				}
				
				mus->cyd->channel[chan].fm.ops[i].flags |= CYD_FM_OP_ENABLE_GATE;
				
				chn->ops[i].trigger_delay--;
			}
			
			if(chn->ops[i].trigger_delay >= 0)
			{
				chn->ops[i].trigger_delay--;
			}

			if (track_status->ops_status[i].slide_speed != 0)
			{
				if (chn->ops[i].target_note > chn->ops[i].note)
				{
					chn->ops[i].note += my_min((Uint32)track_status->ops_status[i].slide_speed, chn->ops[i].target_note - chn->ops[i].note);
				}
				
				else if (chn->ops[i].target_note < chn->ops[i].note)
				{
					chn->ops[i].note -= my_min((Uint32)track_status->ops_status[i].slide_speed, chn->ops[i].note - chn->ops[i].target_note);
				}
			}
		}
	}
	
	CydChannel *cydchn = &mus->cyd->channel[chan];

#ifndef CYD_DISABLE_VIBRATO

	Uint8 ctrl = 0;
	
	int vibdep = my_max(0, (int)track_status->vibrato_depth - (int)track_status->vibrato_delay);
	int vibspd = track_status->vibrato_speed;
	
	int tremdep = track_status->tremolo_depth;
	int tremspd = track_status->tremolo_speed;
	
	int vibdep_ops[CYD_FM_NUM_OPS] = { 0 };
	int vibspd_ops[CYD_FM_NUM_OPS] = { 0 };
	
	int tremspd_ops[CYD_FM_NUM_OPS] = { 0 };
	int tremdep_ops[CYD_FM_NUM_OPS] = { 0 };
	
	if(cydchn->fm.flags & CYD_FM_ENABLE_4OP)
	{
		for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
		{
			vibdep_ops[i] = my_max(0, (int)track_status->ops_status[i].vibrato_depth - (int)track_status->ops_status[i].vibrato_delay);
			vibspd_ops[i] = track_status->ops_status[i].vibrato_speed;
			
			tremdep_ops[i] = track_status->ops_status[i].tremolo_depth;
			tremspd_ops[i] = track_status->ops_status[i].tremolo_speed;
		}
	}
	
	int fm_vibdep = my_max(0, (int)track_status->fm_vibrato_depth - (int)track_status->fm_vibrato_delay);
	int fm_vibspd = track_status->fm_vibrato_speed;
	
	int fm_tremdep = track_status->fm_tremolo_depth;
	int fm_tremspd = track_status->fm_tremolo_speed;

	if (track_status->pattern)
	{
		ctrl = track_status->pattern->step[track_status->pattern_step].ctrl;
		
		for(int i = 0; i < MUS_MAX_COMMANDS; ++i)
		{
			if ((track_status->pattern->step[track_status->pattern_step].command[i] & 0xff00) == MUS_FX_VIBRATO)
			{
				ctrl |= MUS_CTRL_VIB;
				
				vibdep = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x000f) << 2;
				
				track_status->vibrato_depth = vibdep;
				
				vibspd = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x00f0) >> 2;
				
				track_status->vibrato_speed = vibspd;

				if (vibspd == 0)
				{
					vibspd = ins->vibrato_speed;
					track_status->vibrato_speed = ins->vibrato_speed;
				}
				
				if (vibdep == 0)
				{
					vibdep = ins->vibrato_depth;
					track_status->vibrato_depth = ins->vibrato_depth;
				}
				
				if(cydchn->fm.flags & CYD_FM_ENABLE_4OP)
				{
					for(int j = 0; j < CYD_FM_NUM_OPS; ++j)
					{
						vibdep_ops[j] = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x000f) << 2;
						track_status->ops_status[j].vibrato_depth = vibdep_ops[j];
						
						vibspd_ops[j] = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x00f0) >> 2;
						track_status->ops_status[j].vibrato_speed = vibspd_ops[j];

						if (vibspd_ops[j] == 0)
						{
							vibspd_ops[j] = ins->ops[j].vibrato_speed;
							track_status->ops_status[j].vibrato_speed = ins->ops[j].vibrato_speed;
						}
						
						if (vibdep_ops[j] == 0)
						{
							vibdep_ops[j] = ins->vibrato_depth;
							track_status->ops_status[j].vibrato_depth = ins->ops[j].vibrato_depth;
						}
					}
				}
			}
			
			if ((track_status->pattern->step[track_status->pattern_step].command[i] & 0xff00) == MUS_FX_TREMOLO)
			{
				ctrl |= MUS_CTRL_TREM;
				
				tremdep = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x000f) << 2;
				
				track_status->tremolo_depth = tremdep;
				
				tremspd = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x00f0) >> 2;
				
				track_status->tremolo_speed = tremspd;

				if (!tremspd)
				{
					tremspd = ins->tremolo_speed;
					track_status->tremolo_speed = ins->tremolo_speed;
				}
				
				if (!tremdep)
				{
					tremdep = ins->tremolo_depth;
					track_status->tremolo_depth = ins->tremolo_depth;
				}
				
				if(cydchn->fm.flags & CYD_FM_ENABLE_4OP)
				{
					for(int j = 0; j < CYD_FM_NUM_OPS; ++j)
					{
						tremdep_ops[j] = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x000f) << 2;
						track_status->ops_status[j].tremolo_depth = tremdep_ops[j];
						
						tremspd_ops[j] = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x00f0) >> 2;
						track_status->ops_status[j].tremolo_speed = tremspd_ops[j];

						if (!tremspd_ops[j])
						{
							tremspd_ops[j] = ins->ops[j].tremolo_speed;
							track_status->ops_status[j].tremolo_speed = ins->ops[j].tremolo_speed;
						}
						
						if (!tremdep_ops[j])
						{
							tremdep_ops[j] = ins->ops[j].tremolo_depth;
							track_status->ops_status[j].tremolo_depth = ins->ops[j].tremolo_depth;
						}
					}
				}
			}
			
			if ((track_status->pattern->step[track_status->pattern_step].command[i] & 0xff00) == MUS_FX_FM_VIBRATO)
			{
				fm_vibdep = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x000f) << 2;
				
				track_status->fm_vibrato_depth = fm_vibdep;
				
				fm_vibspd = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x00f0) >> 2;
				
				track_status->fm_vibrato_speed = fm_vibspd;

				if (fm_vibspd == 0)
				{
					fm_vibspd = ins->fm_vibrato_speed;
					track_status->fm_vibrato_speed = ins->fm_vibrato_speed;
				}
				
				if (fm_vibdep == 0)
				{
					fm_vibdep = ins->fm_vibrato_depth;
					track_status->fm_vibrato_depth = ins->fm_vibrato_depth;
				}
			}
			
			if ((track_status->pattern->step[track_status->pattern_step].command[i] & 0xff00) == MUS_FX_FM_TREMOLO)
			{
				fm_tremdep = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x000f) << 2;
				
				track_status->fm_tremolo_depth = fm_tremdep;
				
				fm_tremspd = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x00f0) >> 2;
				
				track_status->fm_tremolo_speed = fm_tremspd;

				if (!fm_tremspd)
				{
					fm_tremspd = ins->fm_tremolo_speed;
					track_status->fm_tremolo_speed = ins->fm_tremolo_speed;
				}
				
				if (!fm_tremdep)
				{
					fm_tremdep = ins->fm_tremolo_depth;
					track_status->fm_tremolo_depth = ins->fm_tremolo_depth;
				}
			}
			
			
			if ((track_status->pattern->step[track_status->pattern_step].command[i] & 0xff00) == MUS_FX_PWM)
			{
				track_status->pwm_delay = 0;
				
				track_status->pwm_depth = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x000f) << 4;
				track_status->pwm_speed = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x00f0) >> 4;
				
				if(cydchn->fm.flags & CYD_FM_ENABLE_4OP)
				{
					for(int j = 0; j < CYD_FM_NUM_OPS; ++j)
					{
						track_status->ops_status[j].pwm_delay = 0;
				
						track_status->ops_status[j].pwm_depth = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x000f) << 4;
						track_status->ops_status[j].pwm_speed = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x00f0) >> 4;
					}
				}
			}
		}
	}

#endif

	Sint16 vib = 0;
	Sint16 fm_vib = 0;
	
	Sint16 trem = 0;
	Sint16 fm_trem = 0;
	
	Sint16 ops_vib[CYD_FM_NUM_OPS] = { 0 };
	Sint16 ops_trem[CYD_FM_NUM_OPS] = { 0 };

#ifndef CYD_DISABLE_VIBRATO
	if (((ctrl & MUS_CTRL_VIB) && !(ins->flags & MUS_INST_INVERT_VIBRATO_BIT)) || (!(ctrl & MUS_CTRL_VIB) && (ins->flags & MUS_INST_INVERT_VIBRATO_BIT)))
	{
		track_status->vibrato_position += vibspd;
		vib = mus_shape(track_status->vibrato_position >> 1, ins->vibrato_shape) * vibdep / 64;
		if (track_status->vibrato_delay) --track_status->vibrato_delay;
	}
#endif

	if (((ctrl & MUS_CTRL_TREM) && !(ins->flags & MUS_INST_INVERT_TREMOLO_BIT)) || (!(ctrl & MUS_CTRL_TREM) && (ins->flags & MUS_INST_INVERT_TREMOLO_BIT)))
	{
		if(track_status->tremolo_delay == 0)
		{
			track_status->tremolo_position += tremspd;
			trem = mus_shape(track_status->tremolo_position >> 1, ins->tremolo_shape) * tremdep / 64;
		}
		
		if (track_status->tremolo_delay) --track_status->tremolo_delay;
	}
	
	if(track_status->fm_vibrato_delay != 0)
	{
		--track_status->fm_vibrato_delay;
	}

	if(track_status->fm_vibrato_delay == 0)
	{	
		track_status->fm_vibrato_position += fm_vibspd;
		fm_vib = mus_shape(track_status->fm_vibrato_position >> 1, ins->fm_vibrato_shape) * fm_vibdep / 64;
	}
	
	if(track_status->fm_tremolo_delay != 0)
	{
		--track_status->fm_tremolo_delay;
	}

	if(track_status->fm_tremolo_delay == 0)
	{
		track_status->fm_tremolo_position += fm_tremspd;
		fm_trem = mus_shape(track_status->fm_tremolo_position >> 1, ins->fm_tremolo_shape) * fm_tremdep / 64;
	}
	
#ifndef CYD_DISABLE_PWM
	if(track_status->pwm_delay == 0)
	{
		do_pwm(mus, chan);
	}
	
	else
	{
		--track_status->pwm_delay;
	}
#endif

	if(cydchn->fm.flags & CYD_FM_ENABLE_4OP)
	{
		for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
		{
			if (((ctrl & MUS_CTRL_VIB) && !(ins->ops[i].flags & MUS_INST_INVERT_VIBRATO_BIT)) || (!(ctrl & MUS_CTRL_VIB) && (ins->ops[i].flags & MUS_FM_OP_INVERT_VIBRATO_BIT)))
			{
				if(track_status->ops_status[i].vibrato_delay == 0)
				{
					track_status->ops_status[i].vibrato_position += vibspd_ops[i];
					
					ops_vib[i] = mus_shape(track_status->ops_status[i].vibrato_position >> 1, ins->ops[i].vibrato_shape) * vibdep_ops[i] / 64;
				}
				
				if (track_status->ops_status[i].vibrato_delay) --track_status->ops_status[i].vibrato_delay;
			}

			if (((ctrl & MUS_CTRL_TREM) && !(ins->ops[i].flags & MUS_INST_INVERT_TREMOLO_BIT)) || (!(ctrl & MUS_CTRL_TREM) && (ins->ops[i].flags & MUS_FM_OP_INVERT_TREMOLO_BIT)))
			{
				if(track_status->ops_status[i].tremolo_delay == 0)
				{
					track_status->ops_status[i].tremolo_position += tremspd_ops[i];
					ops_trem[i] = mus_shape(track_status->ops_status[i].tremolo_position >> 1, ins->ops[i].tremolo_shape) * tremdep_ops[i] / 64;
				}
				
				if (track_status->ops_status[i].tremolo_delay) --track_status->ops_status[i].tremolo_delay;
			}
			
			if(track_status->ops_status[i].pwm_delay == 0)
			{
				//do_pwm(mus, chan);
				do_4op_pwm(mus, chan, i);
			}
			
			else
			{
				--track_status->ops_status[i].pwm_delay;
			}
		}
	}
	
	cydchn->fm.fm_vib = fm_vib;
	
	Sint32 note = (mus->channel[chan].fixed_note != 0xffff ? mus->channel[chan].fixed_note : mus->channel[chan].note) + vib + ((Sint16)mus->channel[chan].arpeggio_note << 8);

	if (note < 0) note = 0;
	if (note > FREQ_TAB_SIZE << 8) note = (FREQ_TAB_SIZE - 1) << 8;

	mus_set_note(mus, chan, note, 0, ins->flags & MUS_INST_QUARTER_FREQ ? 4 : 1);
	
	if(track_status->tremolo_delay == 0)
	{
		cydchn->prev_tremolo = cydchn->tremolo;
		cydchn->tremolo = trem;
		cydchn->tremolo_interpolation_counter = 0;	
	}
	
	if(track_status->fm_tremolo_delay == 0)
	{
		cydchn->fm.fm_prev_tremolo = cydchn->fm.fm_tremolo;
		cydchn->fm.fm_tremolo = fm_trem;
		cydchn->fm.fm_tremolo_interpolation_counter = 0;	
	}
	
	if(cydchn->fm.flags & CYD_FM_ENABLE_4OP)
	{
		for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
		{
			//Sint32 note_ops = (mus->channel[chan].ops[i].note) + ops_vib[i] + ((Uint16)mus->channel[chan].ops[i].arpeggio_note << 8);
			Sint32 note_ops = (mus->channel[chan].ops[i].fixed_note != 0xffff ? mus->channel[chan].ops[i].fixed_note : mus->channel[chan].ops[i].note) + ops_vib[i] + ((Sint16)mus->channel[chan].ops[i].arpeggio_note << 8);

			if (note_ops < 0) note_ops = 0;
			if (note_ops > FREQ_TAB_SIZE << 8) note_ops = (FREQ_TAB_SIZE - 1) << 8;

			//mus_set_note(mus, chan, note, 0, ins->flags & MUS_INST_QUARTER_FREQ ? 4 : 1);
			mus_set_fm_op_note(mus, chan, &cydchn->fm, note_ops, i, 0, 1, ins);
			
			if(cydchn->fm.ops[i].flags & CYD_FM_OP_ENABLE_CSM_TIMER)
			{
				Uint32 frequency = get_freq(chn->ops[i].CSM_timer_note);

				cydchn->fm.ops[i].csm.frequency = (Uint64)(ACC_LENGTH) / (Uint64)1024 * (Uint64)(frequency) / (Uint64)mus->cyd->sample_rate;
			}
			
			if(track_status->ops_status[i].tremolo_delay == 0)
			{
				cydchn->fm.ops[i].prev_tremolo = cydchn->fm.ops[i].tremolo;
				cydchn->fm.ops[i].tremolo = ops_trem[i];
				cydchn->fm.ops[i].tremolo_interpolation_counter = 0;	
			}
		}
	}
}


Uint32 mus_ext_sync(MusEngine *mus)
{
	cyd_lock(mus->cyd, 1);

	Uint32 s = ++mus->ext_sync_ticks;

	cyd_lock(mus->cyd, 0);

	return s;
}


int mus_advance_tick(void* udata)
{
	MusEngine *mus = udata;

	if (!(mus->flags & MUS_EXT_SYNC))
	{
		mus->ext_sync_ticks = 1;
	}

	while (mus->ext_sync_ticks-- > 0)
	{
		if (mus->song)
		{
			for (int i = 0; i < mus->song->num_channels; ++i)
			{
				MusTrackStatus *track_status = &mus->song_track[i];
				CydChannel *cydchn = &mus->cyd->channel[i];
				MusChannel *muschn = &mus->channel[i];

				if (mus->song_counter == 0)
				{
					while (track_status->sequence_position < mus->song->num_sequences[i] && mus->song->sequence[i][track_status->sequence_position].position <= mus->song_position)
					{
						track_status->pattern = &mus->song->pattern[mus->song->sequence[i][track_status->sequence_position].pattern];
						
						track_status->pattern_step = mus->song_position - mus->song->sequence[i][track_status->sequence_position].position;
						
						if (track_status->pattern_step >= mus->song->pattern[mus->song->sequence[i][track_status->sequence_position].pattern].num_steps)
							track_status->pattern = NULL;
						track_status->note_offset = mus->song->sequence[i][track_status->sequence_position].note_offset;
						++track_status->sequence_position;
					}
				}

				int delay = 0;

				if (track_status->pattern)
				{
					for(int i1 = 0; i1 < MUS_MAX_COMMANDS; ++i1)
					{
						if ((track_status->pattern->step[track_status->pattern_step].command[i1] & 0xFFF0) == MUS_FX_EXT_NOTE_DELAY)
						{
							delay = track_status->pattern->step[track_status->pattern_step].command[i1] & 0xf;
						}
					}
				}

				if (mus->song_counter == delay)
				{
					if (track_status->pattern)
					{
						if (1 || track_status->pattern_step == 0)
						{
							Uint8 note = track_status->pattern->step[track_status->pattern_step].note < 0xf0 ?
								track_status->pattern->step[track_status->pattern_step].note + track_status->note_offset :
								track_status->pattern->step[track_status->pattern_step].note;
							Uint8 inst = track_status->pattern->step[track_status->pattern_step].instrument;
							MusInstrument *pinst = NULL;
							
							bool update_adsr = true; //update ADSR params; must not update them if no instrument is present (to help to utilize direct ADSR control commands)
							cydchn->fm.update_ops_adsr = true;

							if (inst == MUS_NOTE_NO_INSTRUMENT)
							{
								pinst = muschn->instrument;
								update_adsr = false;
								cydchn->fm.update_ops_adsr = false;
							}
							
							else
							{
								if (inst < mus->song->num_instruments)
								{
									pinst = &mus->song->instrument[inst];
									muschn->instrument = pinst;
								}
							}
							
							if(note == MUS_NOTE_CUT)
							{
								if(muschn->instrument != NULL)
								{
									if (!(muschn->flags & MUS_CHN_DISABLED))
									{
										cydchn->adsr.volume = 0;
										track_status->volume = 0;
									}
									
									if(muschn->instrument->fm_flags && CYD_FM_ENABLE_4OP)
									{
										for(int i1 = 0; i1 < CYD_FM_NUM_OPS; ++i1)
										{
											cydchn->fm.ops[i1].adsr.volume = 0;
											track_status->ops_status[i1].volume = 0;
										}
									}
								}
							}
							
							if (note == MUS_NOTE_MACRO_RELEASE)
							{
								if(muschn->instrument != NULL)
								{
									for(int pr = 0; pr < muschn->instrument->num_macros; ++pr)
									{
										for(int i = 0; i < MUS_PROG_LEN; ++i)
										{
											if((muschn->instrument->program[pr][i] & 0xff00) == MUS_FX_RELEASE_POINT)
											{
												muschn->program_tick[pr] = i + 1;
												break;
											}
										}
									}
									
									if(muschn->instrument->fm_flags & CYD_FM_ENABLE_4OP)
									{
										for(int i1 = 0; i1 < CYD_FM_NUM_OPS; ++i1)
										{
											for(int pr = 0; pr < muschn->instrument->ops[i1].num_macros; ++pr)
											{
												for(int j = 0; j < MUS_PROG_LEN; ++j)
												{
													if((muschn->instrument->ops[i1].program[pr][j] & 0xff00) == MUS_FX_RELEASE_POINT)
													{
														muschn->ops[i1].program_tick[pr] = j + 1;
														break;
													}
												}
											}
										}
									}
								}
							}
							
							if (note == MUS_NOTE_RELEASE_WITHOUT_MACRO)
							{
								cyd_enable_gate(mus->cyd, &mus->cyd->channel[i], 0);
							}

							if (note == MUS_NOTE_RELEASE)
							{
								cyd_enable_gate(mus->cyd, &mus->cyd->channel[i], 0);
								
								if(muschn->instrument != NULL)
								{
									for(int pr = 0; pr < muschn->instrument->num_macros; ++pr)
									{
										for(int i = 0; i < MUS_PROG_LEN; ++i)
										{
											if((muschn->instrument->program[pr][i] & 0xff00) == MUS_FX_RELEASE_POINT)
											{
												muschn->program_tick[pr] = i + 1;
												break;
											}
										}
									}
									
									if(muschn->instrument->fm_flags & CYD_FM_ENABLE_4OP)
									{
										for(int i1 = 0; i1 < CYD_FM_NUM_OPS; ++i1)
										{
											for(int pr = 0; pr < muschn->instrument->ops[i1].num_macros; ++pr)
											{
												for(int j = 0; j < MUS_PROG_LEN; ++j)
												{
													if((muschn->instrument->ops[i1].program[pr][j] & 0xff00) == MUS_FX_RELEASE_POINT)
													{
														muschn->ops[i1].program_tick[pr] = j + 1;
														break;
													}
												}
											}
										}
									}
								}
							}
							
							else if (pinst && note != MUS_NOTE_NONE && note != MUS_NOTE_CUT && note != MUS_NOTE_MACRO_RELEASE && note != MUS_NOTE_RELEASE_WITHOUT_MACRO)
							{
								track_status->slide_speed = 0;
								int speed = pinst->slide_speed | 1;
								
								int speed_ops[CYD_FM_NUM_OPS] = { 0 };
								
								if(pinst->fm_flags & CYD_FM_ENABLE_4OP)
								{
									for(int j = 0; j < CYD_FM_NUM_OPS; ++j)
									{
										track_status->ops_status[j].slide_speed = 0;
										speed_ops[j] = pinst->ops[j].slide_speed | 1;
									}
								}
								
								Uint8 ctrl = track_status->pattern->step[track_status->pattern_step].ctrl;
								
								for(int i1 = 0; i1 < MUS_MAX_COMMANDS; ++i1)
								{
									if ((track_status->pattern->step[track_status->pattern_step].command[i1] & 0xff00) == MUS_FX_SLIDE)
									{
										ctrl |= MUS_CTRL_SLIDE | MUS_CTRL_LEGATO;
										speed &= 0xf00;
										speed |= (track_status->pattern->step[track_status->pattern_step].command[i1] & 0xff);
										
										if(pinst->fm_flags & CYD_FM_ENABLE_4OP)
										{
											for(int j = 0; j < CYD_FM_NUM_OPS; ++j)
											{
												speed_ops[j] &= 0xf00;
												speed_ops[j] |= (track_status->pattern->step[track_status->pattern_step].command[i1] & 0xff);
											}
										}
									}
									
									if ((track_status->pattern->step[track_status->pattern_step].command[i1] & 0xff00) == MUS_FX_FAST_SLIDE)
									{
										ctrl |= MUS_CTRL_SLIDE | MUS_CTRL_LEGATO;
										speed = ((track_status->pattern->step[track_status->pattern_step].command[i1] & 0xff) << 4);
										
										if(pinst->fm_flags & CYD_FM_ENABLE_4OP)
										{
											for(int j = 0; j < CYD_FM_NUM_OPS; ++j)
											{
												speed_ops[j] = ((track_status->pattern->step[track_status->pattern_step].command[i1] & 0xff) << 4);
											}
										}
									}
								}

								if (ctrl & MUS_CTRL_SLIDE)
								{
									if (ctrl & MUS_CTRL_LEGATO)
									{
										mus_set_slide(mus, i, (((Uint16)note + pinst->base_note - MIDDLE_C) << 8) + pinst->finetune);
										
										if(pinst->fm_flags & CYD_FM_ENABLE_4OP)
										{
											for(int j = 0; j < CYD_FM_NUM_OPS; ++j)
											{
												if(pinst->fm_flags & CYD_FM_ENABLE_3CH_EXP_MODE)
												{
													mus->channel[i].ops[j].target_note = (((Uint16)note + pinst->ops[j].base_note - MIDDLE_C) << 8) + pinst->ops[j].finetune;
												}
												
												else
												{
													mus->channel[i].ops[j].target_note = (((Uint16)note + pinst->base_note - MIDDLE_C) << 8) + (Sint32)mus->cyd->channel[i].fm.ops[j].detune * DETUNE + coarse_detune_table[mus->cyd->channel[i].fm.ops[j].coarse_detune];
												}
											}
										}
									}
									
									else
									{
										Uint16 oldnote = muschn->note;
										
										Uint16 ops_notes[CYD_FM_NUM_OPS] = { 0 };
										
										if(pinst->fm_flags & CYD_FM_ENABLE_4OP)
										{
											for(int j = 0; j < CYD_FM_NUM_OPS; ++j)
											{
												ops_notes[j] = muschn->ops[j].note;
											}
										}
										
										mus_trigger_instrument_internal(mus, i, pinst, note << 8, mus->song->default_panning[i] == mus->cyd->channel[i].init_panning ? (mus->song->default_panning[i] + CYD_PAN_CENTER) : -1, update_adsr);
										muschn->note = oldnote;
										
										if(pinst->fm_flags & CYD_FM_ENABLE_4OP)
										{
											for(int j = 0; j < CYD_FM_NUM_OPS; ++j)
											{
												muschn->ops[j].note = ops_notes[j];
											}
										}
									}
									
									track_status->slide_speed = speed;
									
									if(pinst->fm_flags & CYD_FM_ENABLE_4OP)
									{
										for(int j = 0; j < CYD_FM_NUM_OPS; ++j)
										{
											track_status->ops_status[j].slide_speed = speed_ops[j];
										}
									}
								}
								
								else if (ctrl & MUS_CTRL_LEGATO)
								{
									mus_set_note(mus, i, (((Uint32)note + pinst->base_note - MIDDLE_C) << 8) + pinst->finetune, 1, pinst->flags & MUS_INST_QUARTER_FREQ ? 4 : 1);
									muschn->target_note = (((Uint32)note + pinst->base_note - MIDDLE_C) << 8) + pinst->finetune;
									
									if(pinst->fm_flags & CYD_FM_ENABLE_4OP)
									{
										for(int j = 0; j < CYD_FM_NUM_OPS; ++j)
										{
											if(pinst->fm_flags & CYD_FM_ENABLE_3CH_EXP_MODE)
											{
												mus->channel[i].ops[j].target_note = mus->channel[i].ops[j].note = (((Uint16)note + pinst->ops[j].base_note - MIDDLE_C) << 8) + pinst->ops[j].finetune;
											}
											
											else
											{
												mus->channel[i].ops[j].target_note = mus->channel[i].ops[j].note = (((Uint16)note + pinst->base_note - MIDDLE_C) << 8) + (Sint32)mus->cyd->channel[i].fm.ops[j].detune * DETUNE + coarse_detune_table[mus->cyd->channel[i].fm.ops[j].coarse_detune];
											}
										}
									}
								}
								
								else
								{
									Uint8 prev_vol_track = track_status->volume;
									Uint8 prev_vol_cyd = cydchn->adsr.volume;
									
									Uint8 prev_vol_track_ops[CYD_FM_NUM_OPS] = { 0 };
									Uint8 prev_vol_cyd_ops[CYD_FM_NUM_OPS] = { 0 };
									
									if(pinst->fm_flags & CYD_FM_ENABLE_4OP)
									{
										for(int j = 0; j < CYD_FM_NUM_OPS; ++j)
										{
											prev_vol_track_ops[j] = track_status->ops_status[j].volume;
											prev_vol_cyd_ops[j] = cydchn->fm.ops[j].adsr.volume;
										}
									}
									
									mus_trigger_instrument_internal(mus, i, pinst, note << 8, mus->song->default_panning[i] == mus->cyd->channel[i].init_panning ? (mus->song->default_panning[i] + CYD_PAN_CENTER) : -1, update_adsr);
									muschn->target_note = (((Uint16)note + pinst->base_note - MIDDLE_C) << 8) + pinst->finetune;

									if (inst == MUS_NOTE_NO_INSTRUMENT)
									{
										track_status->volume = prev_vol_track;
										cydchn->adsr.volume = prev_vol_cyd;
										
										if(pinst->fm_flags & CYD_FM_ENABLE_4OP)
										{
											for(int j = 0; j < CYD_FM_NUM_OPS; ++j)
											{
												track_status->ops_status[j].volume = prev_vol_track_ops[j];
												cydchn->fm.ops[j].adsr.volume = prev_vol_cyd_ops[j];
											}
										}
									}
								}

								if (inst != MUS_NOTE_NO_INSTRUMENT)
								{
									if (pinst->flags & MUS_INST_RELATIVE_VOLUME)
									{
										track_status->volume = MAX_VOLUME;
										cydchn->adsr.volume = (muschn->flags & MUS_CHN_DISABLED)
											? 0
											: (int)pinst->volume * (int)mus->volume / MAX_VOLUME * (int)mus->play_volume / MAX_VOLUME * (int)muschn->volume / MAX_VOLUME;
									}
									
									else
									{
										track_status->volume = pinst->volume;
										cydchn->adsr.volume = (muschn->flags & MUS_CHN_DISABLED) ? 0 : (int)pinst->volume * (int)mus->volume / MAX_VOLUME * (int)mus->play_volume / MAX_VOLUME * (int)muschn->volume / MAX_VOLUME;
									}
								}
							}
						}
					}
				}

				if (track_status->pattern) mus_exec_track_command(mus, i, mus->song_counter == delay);
			}

			++mus->song_counter;
			
			if (mus->song_counter >= ((!(mus->song_position & 1)) ? mus->song->song_speed : mus->song->song_speed2))
			{
				for (int i = 0; i < mus->cyd->n_channels; ++i)
				{
					MusTrackStatus *track_status = &mus->song_track[i];

					if (track_status->pattern)
					{
						int flag = 1;
						
						for(int i1 = 0; i1 < MUS_MAX_COMMANDS; ++i1)
						{
							Uint16 command = track_status->pattern->step[track_status->pattern_step].command[i1];
							
							if ((command & 0xff00) == MUS_FX_LOOP_PATTERN)
							{
								Uint16 step = command & 0xff;
								track_status->pattern_step = step;
								flag = 0;
							}
							
							else if ((command & 0xff00) == MUS_FX_SKIP_PATTERN)
							{
								mus->song_position += my_max(track_status->pattern->num_steps - track_status->pattern_step - 1, 0);
								track_status->pattern = NULL;
								track_status->pattern_step = 0;
								flag = 0;
								break;
							}
							
							else if ((command & 0xfff0) == MUS_FX_FT2_PATTERN_LOOP)
							{
								if(command & 0xf) //loop end
								{
									if(!(track_status->in_loop))
									{
										track_status->loops_left = (command & 0xf);
										track_status->in_loop = true;
										
										//track_status->loops_left--;
										
										for(int j = track_status->pattern_step; j >= 0; --j)
										{
											for(int com = 0; com < MUS_MAX_COMMANDS; ++com)
											{
												if(track_status->pattern->step[j].command[com] == MUS_FX_FT2_PATTERN_LOOP) //search for loop start
												{
													mus->song_position -= (track_status->pattern_step - j + 1); //jump to loop start
													
													int delta = track_status->pattern_step - j + 1;
													
													for(int chns = 0; chns < mus->song->num_channels; ++chns)
													{
														if(mus->song_track[chns].pattern_step >= delta)
														{
															mus->song_track[chns].pattern_step -= (delta);
															mus->song_track[chns].sequence_position -= (delta);
														}
													}
													
													goto out;
												}
											}
										}
									}
									
									else
									{
										track_status->loops_left--;
										
										if(track_status->loops_left == 0)
										{
											track_status->in_loop = false;
											goto out;
										}
										
										for(int j = track_status->pattern_step; j >= 0; --j)
										{
											for(int com = 0; com < MUS_MAX_COMMANDS; ++com)
											{
												if(track_status->pattern->step[j].command[com] == MUS_FX_FT2_PATTERN_LOOP) //search for loop start
												{
													mus->song_position -= (track_status->pattern_step - j + 1); //jump to loop start
													
													int delta = track_status->pattern_step - j + 1;
													
													for(int chns = 0; chns < mus->song->num_channels; ++chns)
													{
														if(mus->song_track[chns].pattern_step >= delta)
														{
															mus->song_track[chns].pattern_step -= (delta);
															mus->song_track[chns].sequence_position -= (delta);
														}
													}
													
													goto out;
												}
											}
										}
									}
								}
								
								else //loop start
								{
									
								}
								
								out:;
								break;
							}
						}	
						
						if(flag != 0)
						{
							++track_status->pattern_step;
						}
						
						if (track_status->pattern && track_status->pattern_step >= track_status->pattern->num_steps)
						{
							track_status->pattern = NULL;
							track_status->pattern_step = 0;
						}
					}
				}
				
				mus->song_counter = 0;
				
				++mus->song_position;
				
				if (mus->song_position >= mus->song->song_length)
				{
					if (mus->song->flags & MUS_NO_REPEAT)
						return 0;

					mus->song_position = mus->song->loop_point;
					
					for (int i = 0; i < mus->cyd->n_channels; ++i)
					{
						MusTrackStatus *track_status = &mus->song_track[i];

						track_status->pattern = NULL;
						track_status->pattern_step = 0;
						track_status->sequence_position = 0;
					}
				}
			}
		}

		for (int i = 0; i < mus->cyd->n_channels; ++i)
		{
			if (mus->channel[i].flags & MUS_CHN_PLAYING)
			{
				mus_advance_channel(mus, i);
			}
		}

#ifndef CYD_DISABLE_MULTIPLEX
		if (mus->song && (mus->song->flags & MUS_ENABLE_MULTIPLEX) && mus->song->multiplex_period > 0)
		{
			for (int i = 0; i < mus->cyd->n_channels; ++i)
			{
				CydChannel *cydchn = &mus->cyd->channel[i];

				if ((mus->multiplex_ctr / mus->song->multiplex_period) == i)
				{
					update_volumes(mus, &mus->song_track[i], &mus->channel[i], cydchn, mus->song_track[i].volume);
				}
				
				else
				{
					cydchn->adsr.volume = 0;
				}
			}

			if (++mus->multiplex_ctr >= mus->song->num_channels * mus->song->multiplex_period)
			{
				mus->multiplex_ctr = 0;
			}
		}
#endif
	}
	
	for(int i = 0; i < mus->cyd->n_channels; ++i)
	{
		MusTrackStatus *track_status = &mus->song_track[i];
		CydChannel *cydchn = &mus->cyd->channel[i];
		MusChannel *muschn = &mus->channel[i];
		
		if(cydchn->fm.flags & CYD_FM_ENABLE_ADDITIVE)
		{
			update_volumes(mus, track_status, muschn, cydchn, mus->song_track[i].volume); //wasn't there
		}
	}
	
	return 1;
}


void mus_set_song(MusEngine *mus, MusSong *song, Uint16 position)
{
	cyd_lock(mus->cyd, 1);
	cyd_reset(mus->cyd);
	mus->song = song;

	if (song != NULL)
	{
		mus->song_counter = 0;
		mus->multiplex_ctr = 0;
#ifndef CYD_DISABLE_INACCURACY
		mus->pitch_mask = (~0) << song->pitch_inaccuracy;
#endif
	}

	mus->song_position = position;
	mus->play_volume = MAX_VOLUME;

	for (int i = 0; i < MUS_MAX_CHANNELS; ++i)
	{
		mus->song_track[i].pattern = NULL;
		mus->song_track[i].pattern_step = 0;
		mus->song_track[i].sequence_position = 0;
		mus->song_track[i].last_ctrl = 0;
		mus->song_track[i].note_offset = 0;
		mus->song_track[i].extarp1 = mus->song_track[i].extarp2 = 0;
		
		mus->song_track[i].in_loop = false;
		mus->song_track[i].loops_left = 0;

		if (song)
		{
			mus->channel[i].volume = song->default_volume[i];
#ifdef STEREOOUTPUT
			if (i < mus->cyd->n_channels)
			{
				cyd_set_panning(mus->cyd, &mus->cyd->channel[i], song->default_panning[i] + CYD_PAN_CENTER);
				
				mus->cyd->channel[i].init_panning = song->default_panning[i] + CYD_PAN_CENTER;
			}
#endif
		}
		else
		{
			mus->channel[i].volume = MAX_VOLUME;
		}
	}
	
	for(int chan = 0; chan < MUS_MAX_CHANNELS; ++chan) //so we reset the damn channels even if no prog reset flag is there
	{
		mus->channel[chan].program_flags = 0;
		
		for(int n = 0; n < MUS_MAX_NESTEDNESS; ++n)
		{
			mus->channel[chan].nestedness[n] = 0;
		}
		
		for(int pr = 0; pr < MUS_MAX_MACROS_INST; ++pr)
		{
			for(int n = 0; n < MUS_MAX_NESTEDNESS; ++n)
			{
				mus->channel[chan].program_loop[pr][n] = 0;
				
				mus->channel[chan].program_loop_addresses[pr][n][0] = 0;
				mus->channel[chan].program_loop_addresses[pr][n][1] = 0;
				
				mus->channel[chan].program_counter[pr] = 0;
				mus->channel[chan].program_tick[pr] = 0;
			}
		}
		
		for(int op = 0; op < CYD_FM_NUM_OPS; ++op)
		{
			mus->channel[chan].ops[op].program_flags = 0;
			
			for(int n = 0; n < MUS_MAX_NESTEDNESS; ++n)
			{
				mus->channel[chan].ops[op].nestedness[n] = 0;
			}
			
			for(int pr = 0; pr < MUS_MAX_MACROS_OP; ++pr)
			{
				for(int n = 0; n < MUS_MAX_NESTEDNESS; ++n)
				{
					mus->channel[chan].ops[op].program_loop[pr][n] = 0;
					
					mus->channel[chan].ops[op].program_loop_addresses[pr][n][0] = 0;
					mus->channel[chan].ops[op].program_loop_addresses[pr][n][1] = 0;
					
					mus->channel[chan].ops[op].program_counter[pr] = 0;
					mus->channel[chan].ops[op].program_tick[pr] = 0;
				}
			}
		}
	}

	cyd_lock(mus->cyd, 0);
}


int mus_poll_status(MusEngine *mus, int *song_position, int *pattern_position, MusPattern **pattern, MusChannel *channel, int *cyd_env, int *mus_note, Uint64 *time_played)
{
	cyd_lock(mus->cyd, 1);

	if (song_position) *song_position = mus->song_position;

	if (pattern_position)
	{
		for (int i = 0; i < MUS_MAX_CHANNELS; ++i)
		{
			pattern_position[i] = mus->song_track[i].pattern_step;
		}
	}

	if (pattern)
	{
		for (int i = 0; i < MUS_MAX_CHANNELS; ++i)
		{
			pattern[i] = mus->song_track[i].pattern;
		}
	}

	if (channel)
	{
		memcpy(channel, mus->channel, sizeof(mus->channel));
	}

	if (cyd_env)
	{
		for (int i = 0; i < my_min(mus->cyd->n_channels, MUS_MAX_CHANNELS); ++i)
		{
			if (mus->cyd->channel[i].flags & CYD_CHN_ENABLE_YM_ENV)
				cyd_env[i] = mus->cyd->channel[i].adsr.volume;
			else
			{
				cyd_env[i] = cyd_env_output(mus->cyd, mus->cyd->channel[i].flags, &mus->cyd->channel[i].adsr, MAX_VOLUME);
				
				if(mus->cyd->channel[i].flags & CYD_CHN_ENABLE_FM)
				{
					Uint32 temp = 0;
					
					if(cyd_env_output(mus->cyd, temp, &mus->cyd->channel[i].fm.adsr, MAX_VOLUME) > cyd_env[i])
					{
						cyd_env[i] = cyd_env_output(mus->cyd, temp, &mus->cyd->channel[i].fm.adsr, MAX_VOLUME);
					}
					
					if(mus->cyd->channel[i].fm.flags & CYD_FM_ENABLE_4OP)
					{
						for(int j = 0; j < CYD_FM_NUM_OPS; ++j)
						{
							if(cyd_fm_op_env_output(mus->cyd, mus->cyd->channel[i].fm.ops[j].flags, &mus->cyd->channel[i].fm.ops[j].adsr, MAX_VOLUME) > cyd_env[i])
							{
								cyd_env[i] = cyd_fm_op_env_output(mus->cyd, mus->cyd->channel[i].fm.ops[j].flags, &mus->cyd->channel[i].fm.ops[j].adsr, MAX_VOLUME);
							}
						}
					}
				}
			}
		}
	}

	if (mus_note)
	{
		for (int i = 0; i < my_min(mus->cyd->n_channels, MUS_MAX_CHANNELS); ++i)
		{
			mus_note[i] = mus->channel[i].note;
		}
	}

	if (time_played)
	{
		*time_played = mus->cyd->samples_played * 1000 / mus->cyd->sample_rate;
	}

	cyd_lock(mus->cyd, 0);

	return mus->song != NULL;
}


int mus_load_instrument(const char *path, MusInstrument *inst, CydWavetableEntry *wavetable_entries)
{
	RWops *ctx = RWFromFile(path, "rb");

	if (ctx)
	{
		int r = mus_load_instrument_RW2(ctx, inst, wavetable_entries);

		my_RWclose(ctx);

		return r;
	}

	return 0;
}


static void load_wavetable_entry(Uint8 version, CydWavetableEntry* e, RWops *ctx)
{
	VER_READ(version, 12, 0xff, &e->flags, 0);
	VER_READ(version, 12, 0xff, &e->sample_rate, 0);
	VER_READ(version, 12, 0xff, &e->samples, 0);
	VER_READ(version, 12, 0xff, &e->loop_begin, 0);
	VER_READ(version, 12, 0xff, &e->loop_end, 0);
	VER_READ(version, 12, 0xff, &e->base_note, 0);

	FIX_ENDIAN(e->flags);
	FIX_ENDIAN(e->sample_rate);
	FIX_ENDIAN(e->samples);
	FIX_ENDIAN(e->loop_begin);
	FIX_ENDIAN(e->loop_end);
	FIX_ENDIAN(e->base_note);
	
	if(version < 35)
	{
		e->base_note += (12 * 5) << 8;
	}

	if (e->samples > 0)
	{
		if (version < 15)
		{
			Sint16 *data = malloc(sizeof(data[0]) * e->samples);

			my_RWread(ctx, data, sizeof(data[0]), e->samples);

			cyd_wave_entry_init(e, data, e->samples, CYD_WAVE_TYPE_SINT16, 1, 1, 1);

			free(data);
		}
		
		else
		{
			Uint32 data_size = 0;
			VER_READ(version, 15, 0xff, &data_size, 0);
			FIX_ENDIAN(data_size);
			Uint8 *compressed = malloc(sizeof(Uint8) * data_size);

			my_RWread(ctx, compressed, sizeof(Uint8), (data_size + 7) / 8); // data_size is in bits

			Sint16 *data = NULL;

#ifndef CYD_DISABLE_WAVETABLE
			data = bitunpack(compressed, data_size, e->samples, (e->flags >> 3) & 3);
#endif

			if (data)
			{
				cyd_wave_entry_init(e, data, e->samples, CYD_WAVE_TYPE_SINT16, 1, 1, 1);
				free(data);
			}
			
			else
			{
				warning("Sample data unpack failed");
			}

			free(compressed);
		}
	}
}


static int find_and_load_wavetable(Uint8 version, RWops *ctx, CydWavetableEntry *wavetable_entries)
{
	if(version < 28)
	{
		for (int i = 0; i < 128; ++i)
		{
			CydWavetableEntry *e = &wavetable_entries[i];

			if (e->samples == 0)
			{
				load_wavetable_entry(version, e, ctx);
				return i;
			}
		}
	}
	
	else
	{
		for (int i = 0; i < CYD_WAVE_MAX_ENTRIES; ++i)
		{
			CydWavetableEntry *e = &wavetable_entries[i];

			if (e->samples == 0)
			{
				load_wavetable_entry(version, e, ctx);
				return i;
			}
		}
	}

	return 0;
}


int mus_load_instrument_RW(Uint8 version, RWops *ctx, MusInstrument *inst, CydWavetableEntry *wavetable_entries)
{
	mus_get_default_instrument(inst);
	
	if(version >= 34 && version < 41)
	{
		Uint16 temp_f = 0;
		_VER_READ(&temp_f, 0);
		
		inst->flags = temp_f;
	}
	
	else
	{
		_VER_READ(&inst->flags, 0);
	}
	
	_VER_READ(&inst->cydflags, 0);
	_VER_READ(&inst->adsr, 0);
	
	if((inst->cydflags & CYD_CHN_ENABLE_FILTER) && version > 33)
	{
		inst->slope = ((inst->adsr.s & 0b11100000) >> 5);
		inst->flttype = (((inst->adsr.a) & 0b11000000) >> 5) | (((inst->adsr.d) & 0b01000000) >> 6);
	}
	
	inst->adsr.a &= 0b00111111;
	inst->adsr.d &= 0b00111111;
	inst->adsr.s &= 0b00011111;
	inst->adsr.r &= 0b00111111;
	
	if(inst->flags & MUS_INST_USE_VOLUME_ENVELOPE)
	{
		VER_READ(version, 40, 0xff, &inst->vol_env_flags, 0);
		VER_READ(version, 40, 0xff, &inst->num_vol_points, 0);
		
		VER_READ(version, 40, 0xff, &inst->vol_env_fadeout, 0);
		
		if(inst->vol_env_flags & MUS_ENV_SUSTAIN)
		{
			VER_READ(version, 40, 0xff, &inst->vol_env_sustain, 0);
		}
	
		if(inst->vol_env_flags & MUS_ENV_LOOP)
		{
			VER_READ(version, 40, 0xff, &inst->vol_env_loop_start, 0);
			VER_READ(version, 40, 0xff, &inst->vol_env_loop_end, 0);
		}
		
		for(int i = 0; i < inst->num_vol_points; ++i)
		{
			VER_READ(version, 40, 0xff, &inst->volume_envelope[i].x, 0);
			VER_READ(version, 40, 0xff, &inst->volume_envelope[i].y, 0);
		}
	}
	
	if(inst->flags & MUS_INST_USE_PANNING_ENVELOPE)
	{
		VER_READ(version, 40, 0xff, &inst->pan_env_flags, 0);
		VER_READ(version, 40, 0xff, &inst->num_pan_points, 0);
		
		VER_READ(version, 40, 0xff, &inst->pan_env_fadeout, 0);
		
		if(inst->pan_env_flags & MUS_ENV_SUSTAIN)
		{
			VER_READ(version, 40, 0xff, &inst->pan_env_sustain, 0);
		}
	
		if(inst->pan_env_flags & MUS_ENV_LOOP)
		{
			VER_READ(version, 40, 0xff, &inst->pan_env_loop_start, 0);
			VER_READ(version, 40, 0xff, &inst->pan_env_loop_end, 0);
		}
		
		for(int i = 0; i < inst->num_pan_points; ++i)
		{
			VER_READ(version, 40, 0xff, &inst->panning_envelope[i].x, 0);
			VER_READ(version, 40, 0xff, &inst->panning_envelope[i].y, 0);
		}
	}
	
	if(inst->cydflags & CYD_CHN_ENABLE_FIXED_NOISE_PITCH)
	{
		VER_READ(version, 33, 0xff, &inst->noise_note, 0);
		
		if(version < 35)
		{
			inst->noise_note += 12 * 5;
		}
	}
	
	if((inst->cydflags & CYD_CHN_ENABLE_VOLUME_KEY_SCALING) && version >= 32)
	{
		VER_READ(version, 32, 0xff, &inst->vol_ksl_level, 0); 
	}
	
	if((inst->cydflags & CYD_CHN_ENABLE_ENVELOPE_KEY_SCALING) && version >= 32)
	{
		VER_READ(version, 32, 0xff, &inst->env_ksl_level, 0);
	}
	
	if(((inst->cydflags & CYD_CHN_ENABLE_SYNC) && version >= 31) || version < 31)
	{
		_VER_READ(&inst->sync_source, 0);
	}
	
	if(((inst->cydflags & CYD_CHN_ENABLE_RING_MODULATION) && version >= 31) || version < 31)
	{
		_VER_READ(&inst->ring_mod, 0);
	}
	
	_VER_READ(&inst->pw, 0);
	_VER_READ(&inst->volume, 0);
	
	Uint8 progsteps = 0;
	
	if(version < 38)
	{
		progsteps = 0;
		_VER_READ(&progsteps, 0);
		
		if(progsteps > 0 && version >= 32)
		{
			for (int i = 0; i < progsteps / 8 + 1; ++i)
			{
				VER_READ(version, 32, 0xff, &inst->program_unite_bits[0][i], 0);
			}
		}
		
		if (progsteps)
		{
			//_VER_READ(&inst->program, (int)progsteps * sizeof(inst->program[0]));
			_VER_READ(inst->program[0], (int)progsteps * sizeof(inst->program[0][0]));
		}
	}
	
	else
	{
		if(inst->flags & MUS_INST_SEVERAL_MACROS)
		{
			VER_READ(version, 38, 0xff, &inst->num_macros, 0);
		}
		
		for(int pr = 0; pr < inst->num_macros; ++pr)
		{
			if(pr > 0)
			{
				inst->program[pr] = (Uint16*)malloc(MUS_PROG_LEN * sizeof(Uint16));
				inst->program_unite_bits[pr] = (Uint8*)malloc((MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
				
				for (int p = 0; p < MUS_PROG_LEN; ++p)
				{
					inst->program[pr][p] = MUS_FX_NOP;
				}
				
				for (int p = 0; p < MUS_PROG_LEN / 8 + 1; ++p)
				{
					inst->program_unite_bits[pr][p] = 0;
				}
			}
			
			Uint8 len = 32;
			VER_READ(version, 38, 0xff, &len, 0);
			
			if (len)
			{
				memset(inst->program_names[pr], 0, sizeof(inst->program_names[pr]));
				_VER_READ(inst->program_names[pr], len);
				inst->program_names[pr][sizeof(inst->program_names[pr]) - 1] = '\0';
			}
			
			progsteps = 0;
			_VER_READ(&progsteps, 0);
			
			if(progsteps > 0)
			{
				for (int i = 0; i < progsteps / 8 + 1; ++i)
				{
					VER_READ(version, 38, 0xff, &inst->program_unite_bits[pr][i], 0);
				}
			}
			
			if (progsteps)
			{
				//_VER_READ(&inst->program, (int)progsteps * sizeof(inst->program[0]));
				_VER_READ(inst->program[pr], (int)progsteps * sizeof(inst->program[pr][0]));
			}
			
			_VER_READ(&inst->prog_period[pr], 0);
		}
	}
	
	if(version < 32) //to account for extended command range
	{
		for(int i = 0; i < progsteps; i++)
		{
			if((inst->program[0][i] & 0xff00) != 0xfc00 && (inst->program[0][i] & 0xff00) != MUS_FX_LABEL && (inst->program[0][i] & 0xff00) != MUS_FX_LOOP && (inst->program[0][i] & 0xff00) != MUS_FX_JUMP)
			{
				if(inst->program[0][i] & 0x8000)
				{
					inst->program_unite_bits[0][i / 8] |= (1 << (i & 7));
				}
				
				else
				{
					inst->program_unite_bits[0][i / 8] &= ~(1 << (i & 7));
				}
			}
			
			else if((inst->program[0][i] & 0xff00) != 0xff00 && (inst->program[0][i] & 0xff00) != 0xfe00 && (inst->program[0][i] & 0xff00) != 0xfc00  && (inst->program[0][i] & 0xff00) != 0xfd00)
			{
				inst->program[0][i] = inst->program[0][i] - 0x8000;
			}
			
			if(((inst->program[0][i] & 0xff00) != 0xff00) && ((inst->program[0][i] & 0xff00) != 0xfe00) && ((inst->program[0][i] & 0xff00) != 0xfd00) && ((inst->program[0][i] & 0xff00) != 0xfc00))
			{
				inst->program[0][i] = inst->program[0][i] & 0x7fff;
			}
			
			if((inst->program[0][i] & 0xff00) == 0xfc00 || (inst->program[0][i] & 0xff00) == 0x7c00)
			{
				inst->program[0][i] = 0x7c00 + (inst->program[0][i] & 0xff);
			}
		}
	}
	
	if(version >= 32)
	{
		for(int i = 0; i < progsteps; i++)
		{
			if((inst->program[0][i] & 0xff00) == MUS_FX_LABEL || (inst->program[0][i] & 0xff00) == MUS_FX_LOOP || (inst->program[0][i] & 0xff00) == MUS_FX_JUMP)
			{
				if(inst->program_unite_bits[0][i / 8] & (1 << (i & 7)))
				{
					inst->program_unite_bits[0][i / 8] &= ~(1 << (i & 7));
				}
			}
		}
	}
	
	if(version < 33)
	{
		for(int i = 0; i < progsteps; i++)
		{
			if(((inst->program[0][i] & (version < 32 ? 0x7f00 : 0xff00)) == MUS_FX_SET_PANNING) 
				|| ((inst->program[0][i] & (version < 32 ? 0x7f00 : 0xff00)) == MUS_FX_PAN_LEFT) 
				|| ((inst->program[0][i] & (version < 32 ? 0x7f00 : 0xff00)) == MUS_FX_PAN_RIGHT)) 
				//to account for extended panning range
			{
				Uint8 temp = inst->program[0][i] & 0xff;
				inst->program[0][i] &= 0xff00;
				inst->program[0][i] += my_min(temp * 2, 255);
			}
		}
	}
	
	if(version < 35)
	{
		for(int i = 0; i < progsteps; i++)
		{
			if(((inst->program[0][i] & (version < 32 ? 0x7f00 : 0xff00)) == MUS_FX_ARPEGGIO_ABS) || ((inst->program[0][i] & (version < 32 ? 0x7f00 : 0xff00)) == MUS_FX_SET_NOISE_CONSTANT_PITCH)) 
			//to account for negative octaves
			{
				Uint8 temp = inst->program[0][i] & 0xff;
				inst->program[0][i] &= 0xff00;
				inst->program[0][i] |= (temp + 12 * 5);
			}
		}
	}
	
	if(version < 38)
	{
		_VER_READ(&inst->prog_period[0], 0);
	}
	
	if(((inst->flags & MUS_INST_SAVE_LFO_SETTINGS) && version >= 31) || version < 31)
	{
		_VER_READ(&inst->vibrato_speed, 0);
		_VER_READ(&inst->vibrato_depth, 0);
		_VER_READ(&inst->pwm_speed, 0);
		_VER_READ(&inst->pwm_depth, 0);
		
		if(version < 34 && inst->pwm_depth > 1)
		{
			inst->pwm_depth /= 2;
		}
		
		VER_READ(version, 30, 0xff, &inst->pwm_delay, 0); //wasn't there
		
		VER_READ(version, 30, 0xff, &inst->tremolo_speed, 0); //wasn't there
		VER_READ(version, 30, 0xff, &inst->tremolo_depth, 0); //wasn't there
		VER_READ(version, 30, 0xff, &inst->tremolo_shape, 0); //wasn't there
		VER_READ(version, 30, 0xff, &inst->tremolo_delay, 0); //wasn't there
	}
	
	if(version > 35)
	{
		_VER_READ(&inst->slide_speed, 0);
	}
	
	else
	{
		Uint8 temp_sl_sp = 0;
		VER_READ(version, 1, 0xff, &temp_sl_sp, 0);
		
		inst->slide_speed = temp_sl_sp;
	}
	
	_VER_READ(&inst->base_note, 0);
	
	if(version < 35)
	{
		//debug("ttttttt");
		inst->base_note += (12 * 5);
	}

	if (version >= 20)
	{
		_VER_READ(&inst->finetune, 0);
	}

	Uint8 len = 16;
	VER_READ(version, 11, 0xff, &len, 0);
	
	if (len)
	{
		memset(inst->name, 0, sizeof(inst->name));
		_VER_READ(inst->name, my_min(len, sizeof(inst->name)));
		inst->name[sizeof(inst->name) - 1] = '\0';
	}
	
	if(((inst->cydflags & CYD_CHN_ENABLE_FILTER) && version >= 31) || version < 31)
	{
		if(version < 33)
		{
			VER_READ(version, 1, 0xff, &inst->cutoff, 0);
			VER_READ(version, 1, 0xff, &inst->resonance, 0);
		}
		
		else
		{
			Uint16 temp = 0;
			VER_READ(version, 33, 0xff, &temp, 0);
			
			inst->cutoff = temp & 0x0fff;
			inst->resonance = (temp & 0xf000) >> 12;
		}
		
		if(version < 34)
		{
			VER_READ(version, 1, 0xff, &inst->flttype, 0);
			VER_READ(version, 30, 0xff, &inst->slope, 0); //wasn't there
		}
	}
	
	if(((inst->flags & MUS_INST_YM_BUZZ) && version >= 31) || version < 31)
	{
		VER_READ(version, 7, 0xff, &inst->ym_env_shape, 0);
		VER_READ(version, 7, 0xff, &inst->buzz_offset, 0);
	}
	
	if(((inst->cydflags & CYD_CHN_ENABLE_FX) && version >= 31) || version < 31)
	{
		VER_READ(version, 10, 0xff, &inst->fx_bus, 0);
	}
	
	if(((inst->flags & MUS_INST_SAVE_LFO_SETTINGS) && version >= 31) || version < 31)
	{
		VER_READ(version, 11, 0xff, &inst->vibrato_shape, 0);
		VER_READ(version, 11, 0xff, &inst->vibrato_delay, 0);
		VER_READ(version, 11, 0xff, &inst->pwm_shape, 0);
	}
	
	if(((inst->cydflags & CYD_CHN_ENABLE_LFSR) && version >= 31) || version < 31)
	{
		VER_READ(version, 18, 0xff, &inst->lfsr_type, 0);
	}
	
	if(version < 34)
	{
		VER_READ(version, 28, 0xff, &inst->mixmode, 0); //wasn't there
	}
	
	else
	{
		inst->mixmode = ((inst->pw & 0xf000) >> 12);
		inst->pw &= 0x0fff;
	}
	
	VER_READ(version, 12, 0xff, &inst->wavetable_entry, 0);
	
	if(((inst->cydflags & CYD_CHN_ENABLE_FM) && version >= 31) || version < 31)
	{
		VER_READ(version, 23, 0xff, &inst->fm_flags, 0);
		VER_READ(version, 23, 0xff, &inst->fm_modulation, 0);
		
		if(inst->fm_flags & CYD_FM_ENABLE_VOLUME_KEY_SCALING)
		{
			VER_READ(version, 32, 0xff, &inst->fm_vol_ksl_level, 0);
		}
		
		if(inst->fm_flags & CYD_FM_ENABLE_ENVELOPE_KEY_SCALING)
		{
			VER_READ(version, 32, 0xff, &inst->fm_env_ksl_level, 0);
		}
		
		if(version < 34)
		{
			VER_READ(version, 23, 0xff, &inst->fm_feedback, 0);
		}
		
		VER_READ(version, 23, 0xff, &inst->fm_harmonic, 0);
		
		VER_READ(version, 23, 0xff, &inst->fm_adsr, 0);
		
		if(version >= 34)
		{
			inst->fm_freq_LUT = inst->fm_adsr.a >> 6;
			inst->fm_feedback = inst->fm_adsr.s >> 5;
		}
		
		inst->fm_adsr.a &= 0b00111111;
		inst->fm_adsr.d &= 0b00111111;
		inst->fm_adsr.s &= 0b00011111;
		inst->fm_adsr.r &= 0b00111111;
		
		VER_READ(version, 25, 0xff, &inst->fm_attack_start, 0);
		
		VER_READ(version, 29, 0xff, &inst->fm_base_note, 0); //wasn't there
		VER_READ(version, 29, 0xff, &inst->fm_finetune, 0); //wasn't there
		
		if(version < 35)
		{
			inst->fm_base_note += (12 * 5);
		}
		
		if(version < 34)
		{
			VER_READ(version, 31, 0xff, &inst->fm_freq_LUT, 0);
		}
		
		if(inst->fm_flags & CYD_FM_SAVE_LFO_SETTINGS)
		{
			VER_READ(version, 31, 0xff, &inst->fm_vibrato_speed, 0); //wasn't there
			VER_READ(version, 31, 0xff, &inst->fm_vibrato_depth, 0); //wasn't there
			VER_READ(version, 31, 0xff, &inst->fm_vibrato_shape, 0); //wasn't there
			VER_READ(version, 31, 0xff, &inst->fm_vibrato_delay, 0); //wasn't there
			
			VER_READ(version, 31, 0xff, &inst->fm_tremolo_speed, 0); //wasn't there
			VER_READ(version, 31, 0xff, &inst->fm_tremolo_depth, 0); //wasn't there
			VER_READ(version, 31, 0xff, &inst->fm_tremolo_shape, 0); //wasn't there
			VER_READ(version, 31, 0xff, &inst->fm_tremolo_delay, 0); //wasn't there
		}
		
		if((inst->fm_flags & CYD_FM_ENABLE_4OP) && version > 33)
		{
			VER_READ(version, 34, 0xff, &inst->alg, 0);
			VER_READ(version, 34, 0xff, &inst->fm_4op_vol, 0);
			
			for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
			{
				VER_READ(version, 34, 0xff, &inst->ops[i].flags, 0);
				VER_READ(version, 34, 0xff, &inst->ops[i].cydflags, 0);
				
				if(version > 35)
				{
					VER_READ(version, 34, 0xff, &inst->ops[i].slide_speed, 0);
				}
				
				else
				{
					Uint8 temp_sl_sp = 0;
					VER_READ(version, 34, 0xff, &temp_sl_sp, 0);
					
					inst->ops[i].slide_speed = temp_sl_sp;
				}
				
				if(inst->fm_flags & CYD_FM_ENABLE_3CH_EXP_MODE)
				{
					VER_READ(version, 34, 0xff, &inst->ops[i].base_note, 0);
					
					if(version < 35)
					{
						inst->ops[i].base_note += (12 * 5);
					}
					
					VER_READ(version, 34, 0xff, &inst->ops[i].finetune, 0);
				}
				
				else
				{
					VER_READ(version, 34, 0xff, &inst->ops[i].harmonic, 0);
					
					Uint8 temp_detune;
					VER_READ(version, 34, 0xff, &temp_detune, 0);
					
					inst->ops[i].detune = (Sint8)(temp_detune >> 4) - 7;
					inst->ops[i].coarse_detune = (temp_detune >> 2) & 0x3;
				}
				
				Uint8 temp_feedback_ssgeg;
				VER_READ(version, 34, 0xff, &temp_feedback_ssgeg, 0);
				
				inst->ops[i].feedback = temp_feedback_ssgeg & 0xF;
				
				if(inst->ops[i].cydflags & CYD_FM_OP_ENABLE_SSG_EG)
				{
					inst->ops[i].ssg_eg_type = (temp_feedback_ssgeg >> 4) & 0x7;
				}
				
				if(version < 39)
				{
					VER_READ(version, 34, 0xff, &inst->ops[i].adsr.a, 0);
					VER_READ(version, 34, 0xff, &inst->ops[i].adsr.d, 0);
					VER_READ(version, 34, 0xff, &inst->ops[i].adsr.s, 0);
					VER_READ(version, 34, 0xff, &inst->ops[i].adsr.r, 0);
				}
				
				else
				{
					VER_READ(version, 34, 0xff, &inst->ops[i].adsr, 0); //added sustain rate param
				}
				
				if(inst->ops[i].flags & MUS_FM_OP_USE_VOLUME_ENVELOPE)
				{
					VER_READ(version, 41, 0xff, &inst->ops[i].vol_env_flags, 0);
					VER_READ(version, 41, 0xff, &inst->ops[i].num_vol_points, 0);
					
					VER_READ(version, 41, 0xff, &inst->ops[i].vol_env_fadeout, 0);
					
					if(inst->ops[i].vol_env_flags & MUS_ENV_SUSTAIN)
					{
						VER_READ(version, 41, 0xff, &inst->ops[i].vol_env_sustain, 0);
					}
				
					if(inst->ops[i].vol_env_flags & MUS_ENV_LOOP)
					{
						VER_READ(version, 41, 0xff, &inst->ops[i].vol_env_loop_start, 0);
						VER_READ(version, 41, 0xff, &inst->ops[i].vol_env_loop_end, 0);
					}
					
					for(int j = 0; j < inst->ops[i].num_vol_points; ++j)
					{
						VER_READ(version, 41, 0xff, &inst->ops[i].volume_envelope[j].x, 0);
						VER_READ(version, 41, 0xff, &inst->ops[i].volume_envelope[j].y, 0);
					}
				}
				
				if((inst->ops[i].cydflags & CYD_FM_OP_ENABLE_FILTER) && version < 40)
				{
					inst->ops[i].slope = ((inst->ops[i].adsr.s & 0b11100000) >> 5);
					inst->ops[i].flttype = (((inst->ops[i].adsr.a) & 0b11000000) >> 5) | (((inst->ops[i].adsr.d) & 0b01000000) >> 6);
				}
				
				inst->ops[i].adsr.s &= 0b00011111;
				
				if(version < 40)
				{
					inst->ops[i].adsr.r &= 0b00111111;
					inst->ops[i].adsr.a &= 0b00111111;
					inst->ops[i].adsr.d &= 0b00111111;
				}
				
				if(inst->ops[i].cydflags & CYD_FM_OP_ENABLE_FIXED_NOISE_PITCH)
				{
					VER_READ(version, 34, 0xff, &inst->ops[i].noise_note, 0);
					
					if(version < 35)
					{
						inst->ops[i].noise_note += 12 * 5;
					}
				}
				
				if(inst->ops[i].cydflags & CYD_FM_OP_ENABLE_VOLUME_KEY_SCALING)
				{
					VER_READ(version, 34, 0xff, &inst->ops[i].vol_ksl_level, 0);
				}
				
				if(inst->ops[i].cydflags & CYD_FM_OP_ENABLE_ENVELOPE_KEY_SCALING)
				{
					VER_READ(version, 34, 0xff, &inst->ops[i].env_ksl_level, 0);
				}
				
				if(inst->ops[i].cydflags & CYD_FM_OP_ENABLE_SYNC)
				{
					VER_READ(version, 34, 0xff, &inst->ops[i].sync_source, 0);
				}
				
				if(inst->ops[i].cydflags & CYD_FM_OP_ENABLE_RING_MODULATION)
				{
					VER_READ(version, 34, 0xff, &inst->ops[i].ring_mod, 0);
				}
				
				VER_READ(version, 34, 0xff, &inst->ops[i].pw, 0);
				
				inst->ops[i].mixmode = ((inst->ops[i].pw & 0xf000) >> 12);
				inst->ops[i].pw &= 0x0fff;
				
				VER_READ(version, 34, 0xff, &inst->ops[i].volume, 0);
				
				if(version < 38)
				{
					Uint8 progsteps = 0;
					_VER_READ(&progsteps, 0);
					
					if(progsteps > 0)
					{
						for (int i1 = 0; i1 < progsteps / 8 + 1; ++i1)
						{
							VER_READ(version, 34, 0xff, &inst->ops[i].program_unite_bits[0][i1], 0);
						}
					}
					
					if (progsteps)
					{
						_VER_READ(inst->ops[i].program[0], (int)progsteps * sizeof(inst->ops[i].program[0][0]));
					}
				}
				
				else
				{
					if(inst->ops[i].flags & MUS_FM_OP_SEVERAL_MACROS)
					{
						_VER_READ(&inst->ops[i].num_macros, 0);
					}
					
					for(int pr = 0; pr < inst->ops[i].num_macros; ++pr)
					{
						if(pr > 0)
						{
							inst->ops[i].program[pr] = (Uint16*)malloc(MUS_PROG_LEN * sizeof(Uint16));
							inst->ops[i].program_unite_bits[pr] = (Uint8*)malloc((MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
							
							for (int p = 0; p < MUS_PROG_LEN; ++p)
							{
								inst->ops[i].program[pr][p] = MUS_FX_NOP;
							}
							
							for (int p = 0; p < MUS_PROG_LEN / 8 + 1; ++p)
							{
								inst->ops[i].program_unite_bits[pr][p] = 0;
							}
						}
						
						Uint8 len = 32;
						VER_READ(version, 38, 0xff, &len, 0);
						
						if (len)
						{
							memset(inst->ops[i].program_names[pr], 0, sizeof(inst->ops[i].program_names[pr]));
							_VER_READ(inst->ops[i].program_names[pr], my_min(len, sizeof(inst->ops[i].program_names[pr])));
							inst->ops[i].program_names[pr][sizeof(inst->ops[i].program_names[pr]) - 1] = '\0';
						}
						
						Uint8 progsteps = 0;
						_VER_READ(&progsteps, 0);
						
						if(progsteps > 0)
						{
							for (int i1 = 0; i1 < progsteps / 8 + 1; ++i1)
							{
								VER_READ(version, 38, 0xff, &inst->ops[i].program_unite_bits[pr][i1], 0);
							}
						}
						
						if (progsteps)
						{
							_VER_READ(inst->ops[i].program[pr], (int)progsteps * sizeof(inst->ops[i].program[pr][0]));
						}
						
						_VER_READ(&inst->ops[i].prog_period[pr], 0);
					}
				}
				
				if(version < 35)
				{
					for(int j = 0; j < progsteps; j++)
					{
						if((inst->ops[i].program[0][j] & 0xff00) == MUS_FX_ARPEGGIO_ABS || (inst->ops[i].program[0][j] & 0xff00) == MUS_FX_SET_NOISE_CONSTANT_PITCH) 
						//to account for negative octaves
						{
							Uint8 temp = inst->ops[i].program[0][j] & 0xff;
							inst->ops[i].program[0][j] &= 0xff00;
							inst->ops[i].program[0][j] |= (temp + 12 * 5);
						}
					}
				}
				
				if(version < 38)
				{
					VER_READ(version, 34, 0xff, &inst->ops[i].prog_period[0], 0);
				}
				
				if(inst->ops[i].flags & MUS_FM_OP_SAVE_LFO_SETTINGS)
				{
					VER_READ(version, 34, 0xff, &inst->ops[i].vibrato_speed, 0);
					VER_READ(version, 34, 0xff, &inst->ops[i].vibrato_depth, 0);
					VER_READ(version, 34, 0xff, &inst->ops[i].vibrato_shape, 0);
					VER_READ(version, 34, 0xff, &inst->ops[i].vibrato_delay, 0);
					
					VER_READ(version, 34, 0xff, &inst->ops[i].pwm_speed, 0);
					VER_READ(version, 34, 0xff, &inst->ops[i].pwm_depth, 0);
					VER_READ(version, 34, 0xff, &inst->ops[i].pwm_shape, 0);
					VER_READ(version, 34, 0xff, &inst->ops[i].pwm_delay, 0);
					
					VER_READ(version, 34, 0xff, &inst->ops[i].tremolo_speed, 0);
					VER_READ(version, 34, 0xff, &inst->ops[i].tremolo_depth, 0);
					VER_READ(version, 34, 0xff, &inst->ops[i].tremolo_shape, 0);
					VER_READ(version, 34, 0xff, &inst->ops[i].tremolo_delay, 0);
				}
				
				VER_READ(version, 34, 0xff, &inst->ops[i].trigger_delay, 0);
				
				if(inst->ops[i].cydflags & CYD_FM_OP_ENABLE_CSM_TIMER)
				{
					VER_READ(version, 37, 0xff, &inst->ops[i].CSM_timer_note, 0);
					VER_READ(version, 37, 0xff, &inst->ops[i].CSM_timer_finetune, 0);
				}
				
				if(inst->ops[i].cydflags & CYD_FM_OP_ENABLE_FILTER)
				{
					VER_READ(version, 34, 0xff, &inst->ops[i].cutoff, 0);
					
					inst->ops[i].resonance = inst->ops[i].cutoff >> 12;
					inst->ops[i].cutoff &= 0xfff;
					
					VER_READ(version, 40, 0xff, &inst->ops[i].flttype, 0);
				}
				
				if (inst->ops[i].cydflags & CYD_FM_OP_ENABLE_WAVE)
				{
					VER_READ(version, 34, 0xff, &inst->ops[i].wavetable_entry, 0);
				}
			}
		}
	}

	VER_READ(version, 23, 0xff, &inst->fm_wave, 0);
	
	if(version < 29)
	{
		inst->fm_base_note = inst->base_note;
		inst->fm_finetune = 0;
	}
	
	if(version < 30)
	{
		if(inst->cutoff == 0x7ff) //extended filter cutoff range
		{
			inst->cutoff = 0xfff;
		}
		
		else
		{
			inst->cutoff *= 2;
		}
	}

#ifndef CYD_DISABLE_WAVETABLE
	if (wavetable_entries)
	{
		if(version < 37)
		{
			if (inst->wavetable_entry == 0xff)
			{
				inst->wavetable_entry = find_and_load_wavetable(version, ctx, wavetable_entries);
			}

			if (version >= 23)
			{
				if (inst->fm_wave == 0xff)
				{
					inst->fm_wave = find_and_load_wavetable(version, ctx, wavetable_entries);
				}
				
				else if (inst->fm_wave == 0xfe)
				{
					inst->fm_wave = inst->wavetable_entry;
				}
			}
		}
		
		else
		{
			if ((inst->wavetable_entry == 0xff) && ((inst->cydflags & CYD_CHN_ENABLE_WAVE) || ((inst->fm_flags & CYD_FM_ENABLE_WAVE) && (inst->wavetable_entry == inst->fm_wave))))
			{
				inst->wavetable_entry = find_and_load_wavetable(version, ctx, wavetable_entries);
			}

			if (version >= 23)
			{
				if ((inst->fm_wave == 0xff) && (inst->wavetable_entry != inst->fm_wave && (inst->fm_flags & CYD_FM_ENABLE_WAVE)))
				{
					inst->fm_wave = find_and_load_wavetable(version, ctx, wavetable_entries);
				}
				
				else if (inst->fm_wave == 0xfe)
				{
					inst->fm_wave = inst->wavetable_entry;
				}
			}
			
			if(inst->wavetable_entry > 0xfd)
			{
				inst->wavetable_entry = 0;
			}
			
			if(inst->fm_wave > 0xfd)
			{
				inst->fm_wave = 0;
			}
		}
		
		if(version < 37)
		{
			if((inst->fm_flags & CYD_FM_ENABLE_4OP) && version > 33)
			{
				for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
				{
					if ((inst->ops[i].cydflags & CYD_FM_OP_ENABLE_WAVE) && inst->ops[i].wavetable_entry == 0xff)
					{
						inst->ops[i].wavetable_entry = find_and_load_wavetable(version, ctx, wavetable_entries);
					}
				}
			}
		}
		
		else
		{
			if(inst->fm_flags & CYD_FM_ENABLE_4OP)
			{
				Uint8 wave_entries[CYD_FM_NUM_OPS] = { 0xFF };
				
				for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
				{
					if(inst->ops[i].cydflags & CYD_FM_OP_ENABLE_WAVE)
					{
						wave_entries[i] = inst->ops[i].wavetable_entry;
					}
				}
				
				for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
				{
					if (inst->ops[i].cydflags & CYD_FM_OP_ENABLE_WAVE)
					{
						bool need_load_wave = true;
						
						if(i >= 1)
						{
							for(int j = 0; j < i; ++j)
							{
								if(wave_entries[j] == wave_entries[i] && wave_entries[i] != 0xff && wave_entries[j] != 0xff && (inst->ops[j].cydflags & CYD_FM_OP_ENABLE_WAVE))
								{
									need_load_wave = false;
									inst->ops[i].wavetable_entry = inst->ops[j].wavetable_entry;
									goto end;
								}
							}
						}
						
						end:;
						
						if(need_load_wave)
						{
							inst->ops[i].wavetable_entry = find_and_load_wavetable(version, ctx, wavetable_entries);
						}
					}
				}
			}
		}
	}
#endif

	/* The file format is little-endian, the following only does something on big-endian machines */

	FIX_ENDIAN(inst->flags);
	FIX_ENDIAN(inst->cydflags);
	FIX_ENDIAN(inst->pw);
	
	if(inst->flags & MUS_INST_USE_VOLUME_ENVELOPE)
	{
		FIX_ENDIAN(inst->vol_env_fadeout);
		
		for(int i = 0; i < inst->num_vol_points; ++i)
		{
			FIX_ENDIAN(inst->volume_envelope[i].x);
		}
	}
	
	if(inst->flags & MUS_INST_USE_PANNING_ENVELOPE)
	{
		FIX_ENDIAN(inst->pan_env_fadeout);
		
		for(int i = 0; i < inst->num_pan_points; ++i)
		{
			FIX_ENDIAN(inst->panning_envelope[i].x);
		}
	}
	
	if(version >= 36)
	{
		FIX_ENDIAN(inst->slide_speed);
		
		inst->sine_acc_shift = (inst->slide_speed & 0xf000) >> 12;
		inst->slide_speed &= 0xfff;
	}
	
	FIX_ENDIAN(inst->cutoff);
	FIX_ENDIAN(inst->buzz_offset);

	for (int i = 0; i < progsteps; ++i)
	{
		FIX_ENDIAN(inst->program[0][i]);
	}

	FIX_ENDIAN(inst->fm_flags);
	
	if(inst->fm_flags & CYD_FM_ENABLE_4OP)
	{
		for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
		{
			FIX_ENDIAN(inst->ops[i].flags);
			FIX_ENDIAN(inst->ops[i].cydflags);
			FIX_ENDIAN(inst->ops[i].pw);
			
			if(version >= 36)
			{
				FIX_ENDIAN(inst->ops[i].slide_speed);
				
				inst->ops[i].sine_acc_shift = (inst->ops[i].slide_speed & 0xf000) >> 12;
				inst->ops[i].slide_speed &= 0xfff;
			}
			
			if(inst->ops[i].flags & MUS_FM_OP_USE_VOLUME_ENVELOPE)
			{
				FIX_ENDIAN(inst->ops[i].vol_env_fadeout);
				
				for(int j = 0; j < inst->ops[i].num_vol_points; ++j)
				{
					FIX_ENDIAN(inst->ops[i].volume_envelope[j].x);
				}
			}
			
			FIX_ENDIAN(inst->ops[i].cutoff);
			
			for (int j = 0; j < progsteps; ++j)
			{
				FIX_ENDIAN(inst->ops[i].program[0][j]);
			}
		}
	}
	
	if(version < 31)
	{
		if((inst->vibrato_speed != 0) || (inst->vibrato_depth != 0) || 
		(inst->pwm_speed != 0) || (inst->pwm_depth != 0) || 
		(inst->pwm_delay != 0) || (inst->tremolo_speed != 0) || 
		(inst->tremolo_depth != 0) || (inst->vibrato_delay != 0) || 
		(inst->tremolo_delay != 0))
		{
			inst->flags |= MUS_INST_SAVE_LFO_SETTINGS;
		}
	}

	if (version < 26)
	{
		inst->adsr.a *= ENVELOPE_SCALE;
		inst->adsr.d *= ENVELOPE_SCALE;
		inst->adsr.r *= ENVELOPE_SCALE;

		inst->fm_adsr.a *= ENVELOPE_SCALE;
		inst->fm_adsr.d *= ENVELOPE_SCALE;
		inst->fm_adsr.r *= ENVELOPE_SCALE;
	}

	return 1;
}


int mus_load_instrument_RW2(RWops *ctx, MusInstrument *inst, CydWavetableEntry *wavetable_entries)
{
	char id[9];

	id[8] = '\0';

	my_RWread(ctx, id, 8, sizeof(id[0]));

	if (strcmp(id, MUS_INST_SIG) == 0)
	{
		Uint8 version = 0;
		my_RWread(ctx, &version, 1, sizeof(version));

		if (version > MUS_VERSION)
			return 0;

		mus_load_instrument_RW(version, ctx, inst, wavetable_entries);

		return 1;
	}
	
	else
	{
		debug("Instrument signature does not match");
		return 0;
	}
}


void mus_get_default_instrument(MusInstrument *inst)
{
	mus_free_inst_programs(inst);
	
	memset(inst, 0, sizeof(*inst));
	
	inst->flags = MUS_INST_DRUM | MUS_INST_SET_PW | MUS_INST_SET_CUTOFF | MUS_INST_SAVE_LFO_SETTINGS;
	inst->pw = 0x600;
	inst->cydflags = CYD_CHN_ENABLE_TRIANGLE | CYD_CHN_ENABLE_KEY_SYNC;
	
	inst->fm_flags = CYD_FM_SAVE_LFO_SETTINGS | CYD_FM_FOUROP_USE_MAIN_INST_PROG;
	inst->fm_vol_ksl_level = 0x80;
	inst->fm_env_ksl_level = 0x80;
	inst->fm_4op_vol = 0x80;
	
	inst->alg = 1;
	
	inst->adsr.a = 1 * ENVELOPE_SCALE;
	inst->adsr.d = 12 * ENVELOPE_SCALE;
	inst->volume = MAX_VOLUME;
	inst->vol_ksl_level = 0x80; //wasn't there
	inst->env_ksl_level = 0x80; //wasn't there
	
	inst->num_vol_points = 2;
	inst->volume_envelope[0].x = 0;
	inst->volume_envelope[0].y = 0;
	inst->volume_envelope[1].x = 0x80;
	inst->volume_envelope[1].y = 0x80;
	
	inst->num_pan_points = 2;
	inst->panning_envelope[0].x = 0;
	inst->panning_envelope[0].y = 0;
	inst->panning_envelope[1].x = 0x80;
	inst->panning_envelope[1].y = 0x80;
	
	inst->base_note = MIDDLE_C;
	inst->noise_note = MIDDLE_C;
	inst->fm_base_note = MIDDLE_C; //wasn't there
	
	inst->finetune = 0;
	
	for(int i = 0; i < MUS_MAX_MACROS_INST; ++i)
	{
		inst->prog_period[i] = 2;
	}
	
	inst->cutoff = 4095;
	inst->slide_speed = 0x80;
	inst->vibrato_speed = 0x20;
	inst->vibrato_depth = 0x20;
	inst->vibrato_shape = MUS_SHAPE_SINE;
	inst->vibrato_delay = 0;
	
	inst->program[0] = (Uint16*)malloc(MUS_PROG_LEN * sizeof(Uint16));
	inst->program_unite_bits[0] = (Uint8*)malloc((MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
	
	inst->num_macros = 1;

	for (int p = 0; p < MUS_PROG_LEN; ++p)
	{
		inst->program[0][p] = MUS_FX_NOP;
	}
	
	for (int p = 0; p < MUS_PROG_LEN / 8 + 1; ++p)
	{
		inst->program_unite_bits[0][p] = 0;
	}
	
	for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
	{
		inst->ops[i].flags = MUS_FM_OP_SET_PW | MUS_FM_OP_SET_CUTOFF | MUS_FM_OP_SAVE_LFO_SETTINGS | MUS_FM_OP_RELATIVE_VOLUME;
		inst->ops[i].pw = 0x600;
		inst->ops[i].cydflags = CYD_FM_OP_ENABLE_SINE | CYD_FM_OP_ENABLE_KEY_SYNC | CYD_FM_OP_ENABLE_EXPONENTIAL_VOLUME;
		
		inst->ops[i].adsr.a = 1 * ENVELOPE_SCALE;
		inst->ops[i].adsr.d = 12 * ENVELOPE_SCALE;
		
		inst->ops[i].num_vol_points = 2;
		inst->ops[i].volume_envelope[0].x = 0;
		inst->ops[i].volume_envelope[0].y = 0;
		inst->ops[i].volume_envelope[1].x = 0x80;
		inst->ops[i].volume_envelope[1].y = 0x80;
		
		inst->ops[i].volume = MAX_VOLUME;
		inst->ops[i].vol_ksl_level = 0x80;
		inst->ops[i].env_ksl_level = 0x80;
		
		inst->ops[i].base_note = MIDDLE_C;
		inst->ops[i].noise_note = MIDDLE_C;
		
		inst->ops[i].finetune = 0;
		
		inst->ops[i].detune = 0;
		inst->ops[i].coarse_detune = 0;
		
		for(int j = 0; j < MUS_MAX_MACROS_OP; ++j)
		{
			inst->ops[i].prog_period[j] = 2;
		}
		
		inst->ops[i].cutoff = 4095;
		inst->ops[i].slide_speed = 0x80;
		inst->ops[i].vibrato_speed = 0x20;
		inst->ops[i].vibrato_depth = 0x20;
		inst->ops[i].vibrato_shape = MUS_SHAPE_SINE;
		inst->ops[i].vibrato_delay = 0;
		
		inst->ops[i].CSM_timer_note = MIDDLE_C;
		inst->ops[i].CSM_timer_finetune = 0;
		
		inst->ops[i].program[0] = (Uint16*)malloc(MUS_PROG_LEN * sizeof(Uint16));
		inst->ops[i].program_unite_bits[0] = (Uint8*)malloc((MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
		
		inst->ops[i].num_macros = 1;

		for (int p = 0; p < MUS_PROG_LEN; ++p)
		{
			inst->ops[i].program[0][p] = MUS_FX_NOP;
		}
		
		for (int p = 0; p < MUS_PROG_LEN / 8 + 1; ++p)
		{
			inst->ops[i].program_unite_bits[0][p] = 0;
		}
	}
	
	inst->ops[2].volume = inst->ops[3].volume = 0;
	inst->ops[1].volume = 0x38;
}


void mus_get_empty_instrument(MusInstrument *inst)
{
	mus_free_inst_programs(inst);
	
	memset(inst, 0, sizeof(*inst));
	
	for(int i = 0; i < MUS_MAX_MACROS_INST; ++i)
	{
		inst->prog_period[i] = 1;
	}
	
	inst->num_vol_points = 2;
	inst->volume_envelope[0].x = 0;
	inst->volume_envelope[0].y = 0;
	inst->volume_envelope[1].x = 0x80;
	inst->volume_envelope[1].y = 0x80;
	
	inst->program[0] = (Uint16*)malloc(MUS_PROG_LEN * sizeof(Uint16));
	inst->program_unite_bits[0] = (Uint8*)malloc((MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
	
	inst->num_macros = 1;
	
	inst->fm_4op_vol = 0x80;
	
	inst->base_note = MIDDLE_C;
	inst->noise_note = MIDDLE_C;
	inst->fm_base_note = MIDDLE_C; //wasn't there

	for (int p = 0; p < MUS_PROG_LEN; ++p)
	{
		inst->program[0][p] = MUS_FX_NOP;
	}
	
	for (int p = 0; p < MUS_PROG_LEN / 8 + 1; ++p)
	{
		inst->program_unite_bits[0][p] = 0;
	}
	
	for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
	{
		for(int j = 0; j < MUS_MAX_MACROS_OP; ++j)
		{
			inst->ops[i].prog_period[j] = 1;
		}
		
		inst->ops[i].program[0] = (Uint16*)malloc(MUS_PROG_LEN * sizeof(Uint16));
		inst->ops[i].program_unite_bits[0] = (Uint8*)malloc((MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
		
		inst->ops[i].num_macros = 1;
		
		inst->ops[i].num_vol_points = 2;
		inst->ops[i].volume_envelope[0].x = 0;
		inst->ops[i].volume_envelope[0].y = 0;
		inst->ops[i].volume_envelope[1].x = 0x80;
		inst->ops[i].volume_envelope[1].y = 0x80;
		
		inst->ops[i].base_note = MIDDLE_C;
		inst->ops[i].noise_note = MIDDLE_C;
		
		inst->ops[i].CSM_timer_note = MIDDLE_C;

		for (int p = 0; p < MUS_PROG_LEN; ++p)
		{
			inst->ops[i].program[0][p] = MUS_FX_NOP;
		}
		
		for (int p = 0; p < MUS_PROG_LEN / 8 + 1; ++p)
		{
			inst->ops[i].program_unite_bits[0][p] = 0;
		}
	}
}


void mus_set_fx(MusEngine *mus, MusSong *song)
{
	cyd_lock(mus->cyd, 1);
	
	for(int f = 0; f < CYD_MAX_FX_CHANNELS; ++f)
	{
		cydfx_set(&mus->cyd->fx[f], &song->fx[f], mus->cyd->sample_rate);
	}

#ifndef CYD_DISABLE_INACCURACY
	mus->pitch_mask = (~0) << song->pitch_inaccuracy;
#endif
	
	cyd_lock(mus->cyd, 0);
}


static void inner_load_fx(RWops *ctx, CydFxSerialized *fx, int version)
{
	Uint8 padding;

	debug("fx @ %u", (Uint32)my_RWtell(ctx));

	if (version >= 22)
	{
		Uint8 len = 16;
		my_RWread(ctx, &len, 1, 1);
		if (len)
		{
			memset(fx->name, 0, sizeof(fx->name));
			//_VER_READ(fx->name, my_min(len, (version < 28 ? (32 * sizeof(char)) : sizeof(fx->name)))); //_VER_READ(fx->name, my_min(len, sizeof(fx->name)));
			
			if(version < 28)
			{
				_VER_READ(fx->name, my_min(len, 32));
			}
			
			else
			{
				_VER_READ(fx->name, my_min(len, sizeof(fx->name)));
			}
			
			fx->name[(version < 28 ? (32 * sizeof(char)) : sizeof(fx->name)) - 1] = '\0'; //fx->name[sizeof(fx->name) - 1] = '\0';
		}
	}

	my_RWread(ctx, &fx->flags, 1, 4);
	
	if((fx->flags & CYDFX_ENABLE_CRUSH) || version < 32)
	{
		my_RWread(ctx, &fx->crush.bit_drop, 1, 1);
	}
	
	if((fx->flags & CYDFX_ENABLE_CHORUS) || version < 32)
	{
		my_RWread(ctx, &fx->chr.rate, 1, 1);
		my_RWread(ctx, &fx->chr.min_delay, 1, 1);
		my_RWread(ctx, &fx->chr.max_delay, 1, 1);
		my_RWread(ctx, &fx->chr.sep, 1, 1);
	}

	Uint8 spread = 0;

	if (version < 27)
		my_RWread(ctx, &spread, 1, 1);

	if (version < 21)
		my_RWread(ctx, &padding, 1, 1);

	int taps = CYDRVB_TAPS;

	if (version < 27)
	{
		taps = 8;
		fx->rvb.taps_quant = 8;
	}
	
	if(version < 32)
	{
		if(version < 28)
		{
			if (version < 27)
			{
				taps = 8;
				fx->rvb.taps_quant = 8;
			}
			
			else
			{
				taps = 16;
				fx->rvb.taps_quant = 16;
			}
			
			for (int i = 0; i < taps; ++i)
			{
				my_RWread(ctx, &fx->rvb.tap[i].delay, 2, 1);
				my_RWread(ctx, &fx->rvb.tap[i].gain, 2, 1);

				if (version >= 27)
				{
					my_RWread(ctx, &fx->rvb.tap[i].panning, 1, 1);
					my_RWread(ctx, &fx->rvb.tap[i].flags, 1, 1);
					
					if(version < 33)
					{
						fx->rvb.tap[i].panning = my_min(fx->rvb.tap[i].panning * 2, 255);
					}
				}
				
				else
				{
					fx->rvb.tap[i].flags = 1;

					if (spread > 0)
						fx->rvb.tap[i].panning = CYD_PAN_LEFT;
					else
						fx->rvb.tap[i].panning = CYD_PAN_CENTER;
				}

				FIX_ENDIAN(fx->rvb.tap[i].gain);
				FIX_ENDIAN(fx->rvb.tap[i].delay);
			}
			
			if (version < 27)
			{
				if (spread == 0)
				{
					for (int i = 8; i < taps; ++i)
					{
						fx->rvb.tap[i].flags = 0;
						fx->rvb.tap[i].delay = 1000;
						fx->rvb.tap[i].gain = CYDRVB_LOW_LIMIT;
					}
				}
				
				else
				{
					for (int i = 8; i < taps; ++i)
					{
						fx->rvb.tap[i].flags = 1;
						fx->rvb.tap[i].panning = CYD_PAN_RIGHT;
						fx->rvb.tap[i].delay = my_min(2000, fx->rvb.tap[i - 8].delay + (fx->rvb.tap[i - 8].delay * spread) / 2000);
						fx->rvb.tap[i].gain = fx->rvb.tap[i - 8].gain;
					}
				}
			}
			
			if(version <= 27) //wasn't there
			{
				for (int i = taps; i < CYDRVB_TAPS; ++i)
				{
					fx->rvb.tap[i].flags = 0;
				}
			}
			
			if(version >= 32) //wasn't there
			{
				for (int i = taps; i < fx->rvb.taps_quant; ++i)
				{
					fx->rvb.tap[i].flags = 0;
				}
				
				if(fx->rvb.taps_quant == 0)
				{
					fx->rvb.tap[0].flags = 1;
				}
			}
		}
		
		else
		{
			for (int i = 0; i < taps; ++i)
			{
				my_RWread(ctx, &fx->rvb.tap[i].delay, 2, 1);
				my_RWread(ctx, &fx->rvb.tap[i].gain, 2, 1);

				if (version >= 27)
				{
					my_RWread(ctx, &fx->rvb.tap[i].panning, 1, 1);
					my_RWread(ctx, &fx->rvb.tap[i].flags, 1, 1);
					
					if(version < 33)
					{
						fx->rvb.tap[i].panning = my_min(fx->rvb.tap[i].panning * 2, 255);
					}
				}
				
				else
				{
					fx->rvb.tap[i].flags = 1;

					if (spread > 0)
						fx->rvb.tap[i].panning = CYD_PAN_LEFT;
					else
						fx->rvb.tap[i].panning = CYD_PAN_CENTER;
				}

				FIX_ENDIAN(fx->rvb.tap[i].gain);
				FIX_ENDIAN(fx->rvb.tap[i].delay);
			}
		
			if (version < 27)
			{
				if (spread == 0)
				{
					for (int i = 8; i < CYDRVB_TAPS; ++i)
					{
						fx->rvb.tap[i].flags = 0;
						fx->rvb.tap[i].delay = 1000;
						fx->rvb.tap[i].gain = CYDRVB_LOW_LIMIT;
					}
				}
				
				else
				{
					for (int i = 8; i < CYDRVB_TAPS; ++i)
					{
						fx->rvb.tap[i].flags = 1;
						fx->rvb.tap[i].panning = CYD_PAN_RIGHT;
						fx->rvb.tap[i].delay = my_min(CYDRVB_SIZE, fx->rvb.tap[i - 8].delay + (fx->rvb.tap[i - 8].delay * spread) / 2000);
						fx->rvb.tap[i].gain = fx->rvb.tap[i - 8].gain;
					}
				}
			}
			
			if(version <= 27) //wasn't there
			{
				for (int i = taps; i < CYDRVB_TAPS; ++i)
				{
					fx->rvb.tap[i].flags = 0;
				}
			}
		}
	}
	
	else
	{
		if(fx->flags & CYDFX_ENABLE_REVERB)
		{
			my_RWread(ctx, &fx->rvb.taps_quant, 1, 1);
			
			for (int i = 0; i < fx->rvb.taps_quant; ++i)
			{
				my_RWread(ctx, &fx->rvb.tap[i].delay, 2, 1);
				my_RWread(ctx, &fx->rvb.tap[i].gain, 2, 1);
				my_RWread(ctx, &fx->rvb.tap[i].panning, 1, 1);
				my_RWread(ctx, &fx->rvb.tap[i].flags, 1, 1);
				
				if(version < 33)
				{
					fx->rvb.tap[i].panning = my_min(fx->rvb.tap[i].panning * 2, 255);
				}
			}
			
			for(int i = fx->rvb.taps_quant; i < CYDRVB_TAPS; i++)
			{
				fx->rvb.tap[i].flags = 0;
			}
		}
	}
	
		/*if (version < 27)
		{
			if (spread == 0)
			{
				for (int i = 8; i < CYDRVB_TAPS; ++i)
				{
					fx->rvb.tap[i].flags = 0;
					fx->rvb.tap[i].delay = 1000;
					fx->rvb.tap[i].gain = CYDRVB_LOW_LIMIT;
				}
			}
			else
			{
				for (int i = 8; i < CYDRVB_TAPS; ++i)
				{
					fx->rvb.tap[i].flags = 1;
					fx->rvb.tap[i].panning = CYD_PAN_RIGHT;
					fx->rvb.tap[i].delay = my_min(CYDRVB_SIZE, fx->rvb.tap[i - 8].delay + (fx->rvb.tap[i - 8].delay * spread) / 2000);
					fx->rvb.tap[i].gain = fx->rvb.tap[i - 8].gain;
				}
			}
		}*/

	if((fx->flags & CYDFX_ENABLE_CRUSH) || version < 32)
	{
		my_RWread(ctx, &fx->crushex.downsample, 1, 1);
	}

	if (version < 19)
	{
		fx->crushex.gain = 128;
	}
	
	else
	{
		if(((fx->flags & CYDFX_ENABLE_CRUSH_DITHER) && version == 32) || ((version > 32) && (fx->flags & CYDFX_ENABLE_CRUSH)) || version < 32)
		{
			my_RWread(ctx, &fx->crushex.gain, 1, 1);
		}
	}
	
	if(version == 32)
	{
		fx->crushex.gain = 128;
	}

	FIX_ENDIAN(fx->flags);
}


int mus_load_fx_RW(RWops *ctx, CydFxSerialized *fx)
{
	char id[9];
	id[8] = '\0';

	my_RWread(ctx, id, 8, sizeof(id[0]));

	if (strcmp(id, MUS_FX_SIG) == 0)
	{
		Uint8 version = 0;
		my_RWread(ctx, &version, 1, sizeof(version));

		debug("FX version = %u", version);

		inner_load_fx(ctx, fx, version);

		return 1;
	}
	
	else
		return 0;
}


int mus_load_fx_file(FILE *f, CydFxSerialized *fx)
{
	RWops *rw = RWFromFP(f, 0);

	if (rw)
	{
		int r = mus_load_fx_RW(rw, fx);

		my_RWclose(rw);

		return r;
	}

	return 0;
}


int mus_load_fx(const char *path, CydFxSerialized *fx)
{
	RWops *rw = RWFromFile(path, "rb");

	if (rw)
	{
		int r = mus_load_fx_RW(rw, fx);

		my_RWclose(rw);

		return r;
	}

	return 0;
}


void delay(int milliseconds)
{
    long pause;
    clock_t now,then;

    pause = milliseconds*(CLOCKS_PER_SEC/1000);
    now = then = clock();
    while( (now-then) < pause )
        now = clock();
}


int mus_load_song_RW(RWops *ctx, MusSong *song, CydWavetableEntry *wavetable_entries)
{
	char id[9];
	id[8] = '\0';

	my_RWread(ctx, id, 8, sizeof(id[0]));

	if (strcmp(id, MUS_SONG_SIG) == 0)
	{
		Uint8 version = 0;
		my_RWread(ctx, &version, 1, sizeof(version));

		debug("Song version = %u", version);

		if (version > MUS_VERSION)
		{
			debug("Unsupported song version");
			return 0;
		}
		
		if(version < 28)
		{
			song->use_old_filter = true; //so correct filter is used
		}
		
		else
		{
			song->use_old_filter = false;
		}

		if (version >= 6)
			my_RWread(ctx, &song->num_channels, 1, sizeof(song->num_channels));
		else
		{
			if (version > 3)
				song->num_channels = 4;
			else
				song->num_channels = 3;
		}

		my_RWread(ctx, &song->time_signature, 1, sizeof(song->time_signature));

		if (version >= 17)
		{
			my_RWread(ctx, &song->sequence_step, 1, sizeof(song->sequence_step));
		}

		my_RWread(ctx, &song->num_instruments, 1, sizeof(song->num_instruments));
		
		debug("Song has %d instruments", song->num_instruments);
		
		my_RWread(ctx, &song->num_patterns, 1, sizeof(song->num_patterns));
		my_RWread(ctx, &song->num_sequences, 1, sizeof(song->num_sequences[0]) * (int)song->num_channels); //my_RWread(ctx, song->num_sequences, 1, sizeof(song->num_sequences[0]) * (int)song->num_channels);
		my_RWread(ctx, &song->song_length, 1, sizeof(song->song_length));

		my_RWread(ctx, &song->loop_point, 1, sizeof(song->loop_point));

		if (version >= 12)
			my_RWread(ctx, &song->master_volume, 1, 1);

		my_RWread(ctx, &song->song_speed, 1, sizeof(song->song_speed));
		my_RWread(ctx, &song->song_speed2, 1, sizeof(song->song_speed2));
		
		Uint8 temp8;
		my_RWread(ctx, &temp8, 1, sizeof(temp8));
		song->song_rate = temp8;

		if (version > 2) my_RWread(ctx, &song->flags, 1, sizeof(song->flags));
		else song->flags = 0;
		
		if((song->flags & MUS_16_BIT_RATE) && version >= 32)
		{
			my_RWread(ctx, &song->song_rate, 1, sizeof(song->song_rate));
		}

		if (version >= 9) my_RWread(ctx, &song->multiplex_period, 1, sizeof(song->multiplex_period));
		else song->multiplex_period = 3;

		if (version >= 16)
		{
			my_RWread(ctx, &song->pitch_inaccuracy, 1, sizeof(song->pitch_inaccuracy));
		}
		else
		{
			song->pitch_inaccuracy = 0;
		}

		/* The file format is little-endian, the following only does something on big-endian machines */

		FIX_ENDIAN(song->song_length);
		FIX_ENDIAN(song->loop_point);
		FIX_ENDIAN(song->time_signature);
		FIX_ENDIAN(song->sequence_step);
		FIX_ENDIAN(song->num_patterns);
		FIX_ENDIAN(song->flags);

		for (int i = 0; i < (int)song->num_channels; ++i)
		{
			FIX_ENDIAN(song->num_sequences[i]);
		}

		Uint8 title_len = 16 + 1; // old length
		
		if(version < 28)
		{
			if (version >= 11)
			{
				my_RWread(ctx, &title_len, 1, 1);
			}

			if (version >= 5)
			{
				memset(song->title, 0, sizeof(song->title)); //memset(song->title, 0, sizeof(song->title) / 2);
				my_RWread(ctx, song->title, 1, my_min(320, title_len)); //my_RWread(ctx, song->title, 1, my_min(32, title_len));
				song->title[title_len] = '\0'; //song->title[32 - 1] = '\0';
			}
		}
		
		else
		{
			if (version >= 11)
			{
				my_RWread(ctx, &title_len, 1, 1);
			}

			if (version >= 5)
			{
				memset(song->title, 0, sizeof(song->title));
				my_RWread(ctx, song->title, 1, my_min(sizeof(song->title), title_len));
				song->title[sizeof(song->title) - 1] = '\0';
			}
		}
		
		debug("Name: \"%s\", its length: %d", song->title, title_len);

		Uint8 n_fx = 0;

		if (version >= 10)
		{
			my_RWread(ctx, &n_fx, 1, sizeof(n_fx));
		}
		
		else if (song->flags & MUS_ENABLE_REVERB)
		{
			n_fx = 1;
		}

		if (n_fx > 0)
		{
			debug("Song has %u effects", n_fx);
			if (version >= 10)
			{
				memset(&song->fx, 0, sizeof(song->fx[0]) * (n_fx > CYD_MAX_FX_CHANNELS ? CYD_MAX_FX_CHANNELS : n_fx));

				debug("Loading fx at offset %x (%d/%d)", (Uint32)my_RWtell(ctx), (int)sizeof(song->fx[0]) * (n_fx > CYD_MAX_FX_CHANNELS ? CYD_MAX_FX_CHANNELS : n_fx), (int)sizeof(song->fx[0]));

				for (int fx = 0; fx < n_fx; ++fx)
				{
					inner_load_fx(ctx, &song->fx[(fx >= CYD_MAX_FX_CHANNELS ? CYD_MAX_FX_CHANNELS - 1 : fx)], version);
				}
			}
			
			else
			{
				for (int fx = 0; fx < n_fx; ++fx)
				{
					song->fx[(fx >= CYD_MAX_FX_CHANNELS ? CYD_MAX_FX_CHANNELS - 1 : fx)].flags = CYDFX_ENABLE_REVERB;
					if (song->flags & MUS_ENABLE_CRUSH) song->fx[(fx >= CYD_MAX_FX_CHANNELS ? CYD_MAX_FX_CHANNELS - 1 : fx)].flags |= CYDFX_ENABLE_CRUSH;

					for (int i = 0; i < 8; ++i)
					{
						Sint32 g, d;
						my_RWread(ctx, &g, 1, sizeof(g));
						my_RWread(ctx, &d, 1, sizeof(d));

						song->fx[(fx >= CYD_MAX_FX_CHANNELS ? CYD_MAX_FX_CHANNELS - 1 : fx)].rvb.tap[i].gain = g;
						song->fx[(fx >= CYD_MAX_FX_CHANNELS ? CYD_MAX_FX_CHANNELS - 1 : fx)].rvb.tap[i].delay = d;
						song->fx[(fx >= CYD_MAX_FX_CHANNELS ? CYD_MAX_FX_CHANNELS - 1 : fx)].rvb.tap[i].panning = CYD_PAN_CENTER;
						song->fx[(fx >= CYD_MAX_FX_CHANNELS ? CYD_MAX_FX_CHANNELS - 1 : fx)].rvb.tap[i].flags = 1;

						FIX_ENDIAN(song->fx[(fx >= CYD_MAX_FX_CHANNELS ? CYD_MAX_FX_CHANNELS - 1 : fx)].rvb.tap[i].gain);
						FIX_ENDIAN(song->fx[(fx >= CYD_MAX_FX_CHANNELS ? CYD_MAX_FX_CHANNELS - 1 : fx)].rvb.tap[i].delay);
					}
				}
			}
		}


		for (int i = 0; i < MUS_MAX_CHANNELS; ++i)
		{
			song->default_volume[i] = MAX_VOLUME;
			song->default_panning[i] = 0;
		}

		if (version >= 13)
		{
			debug("Loading default volumes at offset %x", (Uint32)my_RWtell(ctx));
			my_RWread(ctx, &song->default_volume[0], sizeof(song->default_volume[0]), song->num_channels);
			debug("Loading default panning at offset %x", (Uint32)my_RWtell(ctx));
			my_RWread(ctx, &song->default_panning[0], sizeof(song->default_panning[0]), song->num_channels);
			
			if(version < 33)
			{
				for(int i = 0; i < song->num_channels; ++i)
				{
					song->default_panning[i] = my_min(song->default_panning[i] * 2, 255);
				}
			}
		}

		if (song->instrument == NULL)
		{
			song->instrument = malloc((size_t)song->num_instruments * sizeof(song->instrument[0]));
			memset(song->instrument, 0, (size_t)song->num_instruments * sizeof(song->instrument[0]));
		}

		for (int i = 0; i < song->num_instruments; ++i)
		{
			mus_load_instrument_RW(version, ctx, &song->instrument[i], NULL);
		}

		for (int i = 0; i < song->num_channels; ++i)
		{
			if (song->num_sequences[i] > 0)
			{
				if (song->sequence[i] == NULL)
					song->sequence[i] = malloc((size_t)song->num_sequences[i] * sizeof(song->sequence[0][0]));

				if (version < 8)
				{
					my_RWread(ctx, song->sequence[i], song->num_sequences[i], sizeof(song->sequence[i][0]));
				}
				
				else
				{
					for (int s = 0; s < song->num_sequences[i]; ++s)
					{
						my_RWread(ctx, &song->sequence[i][s].position, 1, sizeof(song->sequence[i][s].position));
						
						if(song->flags & MUS_8_BIT_PATTERN_INDEX)
						{
							Uint8 temp = 0;
							my_RWread(ctx, &temp, 1, sizeof(temp));
							
							song->sequence[i][s].pattern = temp;
						}
						
						else
						{
							my_RWread(ctx, &song->sequence[i][s].pattern, 1, sizeof(song->sequence[i][s].pattern));
						}
						
						my_RWread(ctx, &song->sequence[i][s].note_offset, 1, sizeof(song->sequence[i][s].note_offset));
					}
				}

				for (int s = 0; s < song->num_sequences[i]; ++s)
				{
					FIX_ENDIAN(song->sequence[i][s].position);
					FIX_ENDIAN(song->sequence[i][s].pattern);
				}
			}
		}

		if (song->pattern == NULL)
		{
			song->pattern = calloc((size_t)song->num_patterns, sizeof(song->pattern[0]));
			//memset(song->pattern, 0, (size_t)song->num_patterns * sizeof(song->pattern[0]));
		}

		for (int i = 0; i < song->num_patterns; ++i)
		{
			Uint16 steps;
			my_RWread(ctx, &steps, 1, sizeof(song->pattern[i].num_steps));

			FIX_ENDIAN(steps);

			if (song->pattern[i].step == NULL)
			{
				song->pattern[i].step = calloc((size_t)steps, sizeof(song->pattern[i].step[0])); //song->pattern[i].step = calloc((size_t)steps, sizeof(song->pattern[i].step[0]));
			}
			
			else if (steps > song->pattern[i].num_steps)
			{
				song->pattern[i].step = calloc((size_t)steps, sizeof(song->pattern[i].step[0])); //realloc(song->pattern[i].step, (size_t)steps * sizeof(song->pattern[i].step[0]));
			}

			song->pattern[i].num_steps = steps;

			if (version >= 24)
				my_RWread(ctx, &song->pattern[i].color, 1, sizeof(song->pattern[i].color));
			else
				song->pattern[i].color = 0;

			if (version < 8)
			{
				size_t s = sizeof(song->pattern[i].step[0]);
				if (version < 2)
					s = sizeof(Uint8) * 3;
				else
					s = sizeof(Uint8) * 3 + sizeof(Uint16) + 1; // aligment issue in version 6 songs

				for (int step = 0; step < song->pattern[i].num_steps; ++step)
				{
					my_RWread(ctx, &song->pattern[i].step[step], 1, s);
					FIX_ENDIAN(song->pattern[i].step[step].command[0]);
				}
			}
			
			else
			{
				int len = song->pattern[i].num_steps / 2 + (song->pattern[i].num_steps & 1);

				Uint8 *packed = malloc(sizeof(Uint8) * len);
				Uint8 *current = packed;

				my_RWread(ctx, packed, sizeof(Uint8), len);

				for (int s = 0; s < song->pattern[i].num_steps; ++s)
				{
					Uint8 bits = (s & 1 || s == song->pattern[i].num_steps - 1) ? (*current & 0xf) : (*current >> 4);

					if (bits & MUS_PAK_BIT_NOTE)
					{
						my_RWread(ctx, &song->pattern[i].step[s].note, 1, sizeof(song->pattern[i].step[s].note));
						
						if(version < 35 && song->pattern[i].step[s].note != MUS_NOTE_NONE && song->pattern[i].step[s].note != MUS_NOTE_RELEASE && song->pattern[i].step[s].note != MUS_NOTE_CUT && song->pattern[i].step[s].note != MUS_NOTE_MACRO_RELEASE)
						{
							song->pattern[i].step[s].note += 12 * 5;
						}
					}
					
					else
						song->pattern[i].step[s].note = MUS_NOTE_NONE;

					if (bits & MUS_PAK_BIT_INST)
						my_RWread(ctx, &song->pattern[i].step[s].instrument, 1, sizeof(song->pattern[i].step[s].instrument));
					else
						song->pattern[i].step[s].instrument = MUS_NOTE_NO_INSTRUMENT;
					
					
					Uint8 temp_ctrl = 0;
					Uint8 num_additional_commands = 0;

					if (bits & MUS_PAK_BIT_CTRL)
					{
						my_RWread(ctx, &song->pattern[i].step[s].ctrl, 1, sizeof(song->pattern[i].step[s].ctrl));
						
						temp_ctrl = song->pattern[i].step[s].ctrl; //wasn't there
						//debug("ctrl bit %d",temp_ctrl);
						num_additional_commands = (temp_ctrl >> 4) & 7;

						//if(version < 32)
						//{

						
							if (version >= 14)
								bits |= song->pattern[i].step[s].ctrl & ~15;
							
							song->pattern[i].step[s].ctrl &= 15;
						//}		
					}
					
					else
						song->pattern[i].step[s].ctrl = 0;

					if (bits & MUS_PAK_BIT_CMD)
						my_RWread(ctx, &song->pattern[i].step[s].command[0], 1, sizeof(song->pattern[i].step[s].command[0]));
					else
						song->pattern[i].step[s].command[0] = 0;

					FIX_ENDIAN(song->pattern[i].step[s].command[0]);
					
					
					
					if(((song->pattern[i].step[s].command[0] & (version < 32 ? 0x7f00 : 0xff00)) == MUS_FX_SET_PANNING 
					|| (song->pattern[i].step[s].command[0] & (version < 32 ? 0x7f00 : 0xff00)) == MUS_FX_PAN_LEFT 
					|| (song->pattern[i].step[s].command[0] & (version < 32 ? 0x7f00 : 0xff00)) == MUS_FX_PAN_RIGHT) && version < 33) 
					//to account for extended panning range
					{
						Uint8 temp = song->pattern[i].step[s].command[0] & 0xff;
						song->pattern[i].step[s].command[0] &= 0xff00;
						song->pattern[i].step[s].command[0] += my_min(temp * 2, 255);
					}
					
					if (bits & MUS_PAK_BIT_CTRL)
					{
						//debug("coding bits loaded %d, ctrl bit %d", num_additional_commands, temp_ctrl);
						if(version >= 32)
						{
							for(int k = 0; k < num_additional_commands; k++)
							{
								my_RWread(ctx, &song->pattern[i].step[s].command[k + 1], 1, sizeof(song->pattern[i].step[s].command[k + 1]));
								
								FIX_ENDIAN(song->pattern[i].step[s].command[k + 1]);
								//debug("dumb read, %d", temp_ctrl);
								
								if(((song->pattern[i].step[s].command[k + 1] & (version < 32 ? 0x7f00 : 0xff00)) == MUS_FX_SET_PANNING 
								|| (song->pattern[i].step[s].command[k + 1] & (version < 32 ? 0x7f00 : 0xff00)) == MUS_FX_PAN_LEFT 
								|| (song->pattern[i].step[s].command[k + 1] & (version < 32 ? 0x7f00 : 0xff00)) == MUS_FX_PAN_RIGHT) && version < 33) 
								//to account for extended panning range
								{
									Uint8 temp = song->pattern[i].step[s].command[k + 1] & 0xff;
									song->pattern[i].step[s].command[k + 1] &= 0xff00;
									song->pattern[i].step[s].command[k + 1] += my_min(temp * 2, 255);
								}
							}
						}
					}
					
					if(version < 35)
					{
						for(int k = 0; k < MUS_MAX_COMMANDS; k++)
						{
							if(((song->pattern[i].step[s].command[k] & 0xff00) == MUS_FX_SET_NOISE_CONSTANT_PITCH || (song->pattern[i].step[s].command[k] & 0xff00) == MUS_FX_ARPEGGIO_ABS))
							//to account for negative octaves
							{
								Uint8 temp = song->pattern[i].step[s].command[k] & 0xff;
								song->pattern[i].step[s].command[k] &= 0xff00;
								song->pattern[i].step[s].command[k] |= (temp + 12 * 5);
							}
						}
					}
					
					if (bits & MUS_PAK_BIT_VOLUME)
					{
						my_RWread(ctx, &song->pattern[i].step[s].volume, 1, sizeof(song->pattern[i].step[s].volume));
					}
					
					else
					{
						song->pattern[i].step[s].volume = MUS_NOTE_NO_VOLUME;
					}

					if (s & 1) ++current;
				}

				free(packed);
			}
			
			for(int j = 1; j < MUS_MAX_COMMANDS; ++j)
			{
				int counter = 0; //how many commands are in current column
				
				for(int s = 0; s < song->pattern[i].num_steps; ++s) //expand pattern to the rightmost column where at least one command is
				{
					if(song->pattern[i].step[s].command[j] != 0)
					{
						counter++;
					}
				}
				
				if(counter != 0)
				{
					song->pattern[i].command_columns = j;
				}
			}
		}
		
		if(version < 28)
		{
			for (int i = 0; i < 128; ++i)
			{
				cyd_wave_entry_init(&wavetable_entries[i], NULL, 0, 0, 0, 0, 0);
			}
		}
		
		else
		{
			for (int i = 0; i < CYD_WAVE_MAX_ENTRIES; ++i)
			{
				cyd_wave_entry_init(&wavetable_entries[i], NULL, 0, 0, 0, 0, 0);
			}
		}
		
		if(version < 28)
		{
			if (version >= 12)
			{
				Uint8 max_wt = 0;
				my_RWread(ctx, &max_wt, 1, sizeof(Uint8));

				for (int i = 0; i < max_wt; ++i)
				{
					load_wavetable_entry(version, &wavetable_entries[i], ctx);
				}

				song->wavetable_names = malloc(max_wt * sizeof(char*));
				memset(song->wavetable_names, 0, max_wt * sizeof(char*));

				if (version >= 26)
				{
					for (int i = 0; i < max_wt; ++i)
					{
						Uint8 len = 0;
						song->wavetable_names[i] = malloc(32 + 1);
						memset(song->wavetable_names[i], 0, 32 + 1);

						my_RWread(ctx, &len, 1, 1);
						my_RWread(ctx, song->wavetable_names[i], len, sizeof(char));
					}
				}
				
				else
				{
					for (int i = 0; i < max_wt; ++i)
					{
						song->wavetable_names[i] = malloc(32 + 1);
						memset(song->wavetable_names[i], 0, 32 + 1);
					}
				}

				song->num_wavetables = max_wt;
			}
			else
				song->num_wavetables = 0;
		}
		
		else
		{
			if (version >= 12)
			{
				Uint8 max_wt = 0;
				my_RWread(ctx, &max_wt, 1, sizeof(Uint8));

				for (int i = 0; i < max_wt; ++i)
				{
					load_wavetable_entry(version, &wavetable_entries[i], ctx);
				}

				song->wavetable_names = malloc(max_wt * sizeof(char*));
				memset(song->wavetable_names, 0, max_wt * sizeof(char*));

				if (version >= 26)
				{
					for (int i = 0; i < max_wt; ++i)
					{
						Uint8 len = 0;
						song->wavetable_names[i] = malloc(MUS_WAVETABLE_NAME_LEN + 1);
						memset(song->wavetable_names[i], 0, MUS_WAVETABLE_NAME_LEN + 1);

						my_RWread(ctx, &len, 1, 1);
						my_RWread(ctx, song->wavetable_names[i], len, sizeof(char));
					}
				}

				else
				{
					for (int i = 0; i < max_wt; ++i)
					{
						song->wavetable_names[i] = malloc(MUS_WAVETABLE_NAME_LEN + 1);
						memset(song->wavetable_names[i], 0, MUS_WAVETABLE_NAME_LEN + 1);
					}
				}

				song->num_wavetables = max_wt;
			}
			else
				song->num_wavetables = 0;
		}
		
		if(version < 30) //to account for cutoff range extended from 000-7ff to 000-fff
		{
			for (int h = 0; h < song->num_instruments; ++h)
			{
				for (int p = 0; p < MUS_PROG_LEN; ++p)
				{
					if ((song->instrument[h].program[0][p] & 0xF000) == 0x6000)
					{
						song->instrument[h].program[0][p] = 0x6000 + (song->instrument[h].program[0][p] & 0x0FFF) * 2;
					}
				}
			}

			for (int p = 0; p < song->num_patterns; ++p)
			{
				for (int w = 0; w < song->pattern[p].num_steps; ++w)
				{
					if ((song->pattern[p].step[w].command[0] & 0xF000) == 0x6000)
					{
						song->pattern[p].step[w].command[0] = 0x6000 + (song->pattern[p].step[w].command[0] & 0x0FFF) * 2;
					}
				}
			}
		}
		
#ifndef STANDALONE_COMPILE
		//optimize_duplicate_patterns(song, false);
#endif

		return 1;
	}

	return 0;
}


int mus_load_song(const char *path, MusSong *song, CydWavetableEntry *wavetable_entries)
{
	RWops *ctx = RWFromFile(path, "rb");

	if (ctx)
	{
		int r = mus_load_song_RW(ctx, song, wavetable_entries);
		my_RWclose(ctx);

		return r;
	}

	return 0;
}


void mus_free_song(MusSong *song)
{
	for (int i = 0; i < song->num_instruments; ++i)
	{
		mus_free_inst_programs(&song->instrument[i]);
	}
	
	free(song->instrument);

	for (int i = 0; i < MUS_MAX_CHANNELS; ++i)
	{
		free(song->sequence[i]);
	}

	for (int i = 0; i < song->num_patterns; ++i)
	{
		free(song->pattern[i].step);
	}

	for (int i = 0; i < song->num_wavetables; ++i)
	{
		free(song->wavetable_names[i]);
	}

	free(song->wavetable_names);

	free(song->pattern);
}


void mus_release(MusEngine *mus, int chan)
{
	cyd_lock(mus->cyd, 1);
	cyd_enable_gate(mus->cyd, &mus->cyd->channel[chan], 0);
	
	if(mus->channel[chan].instrument != NULL)
	{
		for(int pr = 0; pr < mus->channel[chan].instrument->num_macros; ++pr)
		{
			for(int i = 0; i < MUS_PROG_LEN; ++i)
			{
				if((mus->channel[chan].instrument->program[pr][i] & 0xff00) == MUS_FX_RELEASE_POINT)
				{
					mus->channel[chan].program_tick[pr] = i + 1;
					break;
				}
			}
		}
		
		if(mus->channel[chan].instrument->fm_flags & CYD_FM_ENABLE_4OP)
		{
			for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
			{
				for(int pr = 0; pr < mus->channel[chan].instrument->ops[i].num_macros; ++pr)
				{
					for(int j = 0; j < MUS_PROG_LEN; ++j)
					{
						if((mus->channel[chan].instrument->ops[i].program[pr][j] & 0xff00) == MUS_FX_RELEASE_POINT)
						{
							mus->channel[chan].ops[i].program_tick[pr] = j + 1;
							break;
						}
					}
				}
			}
		}
	}

	cyd_lock(mus->cyd, 0);
}


int mus_load_instrument_file(Uint8 version, FILE *f, MusInstrument *inst, CydWavetableEntry *wavetable_entries)
{
	RWops *rw = RWFromFP(f, 0);

	if (rw)
	{
		int r = mus_load_instrument_RW(version, rw, inst, wavetable_entries);

		my_RWclose(rw);

		return r;
	}

	return 0;
}


int mus_load_instrument_file2(FILE *f, MusInstrument *inst, CydWavetableEntry *wavetable_entries)
{
	RWops *rw = RWFromFP(f, 0);

	if (rw)
	{
		int r = mus_load_instrument_RW2(rw, inst, wavetable_entries);

		my_RWclose(rw);

		return r;
	}

	return 0;
}


int mus_load_song_file(FILE *f, MusSong *song, CydWavetableEntry *wavetable_entries)
{
	RWops *rw = RWFromFP(f, 0);

	if (rw)
	{
		int r = mus_load_song_RW(rw, song, wavetable_entries);

		my_RWclose(rw);

		return r;
	}

	return 0;
}

#ifndef STANDALONE_COMPILE
int mus_load_patch_RW(RWops *ctx, WgSettings *settings) //wasn't there
{
	char id[15];
	id[14] = '\0';

	my_RWread(ctx, id, 14, sizeof(id[0]));

	if (strcmp(id, MUS_WAVEGEN_PATCH_SIG) == 0)
	{
		Uint8 version = 0;
		my_RWread(ctx, &version, 1, sizeof(version));
		
		if(version <= 228)
		{
			my_RWread(ctx, &settings->num_oscs, sizeof(settings->num_oscs), 1);
			my_RWread(ctx, &settings->length, sizeof(settings->length), 1);
			
			int num_osc_value = *(&settings->num_oscs);
			
			for(int i = 0; i < num_osc_value; i++)
			{
				my_RWread(ctx, &settings->chain[i].osc, sizeof(settings->chain[i].osc), 1);
				my_RWread(ctx, &settings->chain[i].op, sizeof(settings->chain[i].op), 1);
				my_RWread(ctx, &settings->chain[i].mult, sizeof(settings->chain[i].mult), 1);
				my_RWread(ctx, &settings->chain[i].shift, sizeof(settings->chain[i].shift), 1);
				my_RWread(ctx, &settings->chain[i].exp, sizeof(settings->chain[i].exp), 1);
				
				if(version >= 33)
				{
					my_RWread(ctx, &settings->chain[i].vol, sizeof(settings->chain[i].vol), 1);
				}
				
				else
				{
					Uint8 temp;
					my_RWread(ctx, &temp, sizeof(temp), 1);
					settings->chain[i].vol = temp;
					
					settings->chain[i].shift *= 16;
				}
				
				my_RWread(ctx, &settings->chain[i].exp_c, sizeof(settings->chain[i].exp_c), 1);
				my_RWread(ctx, &settings->chain[i].flags, sizeof(settings->chain[i].flags), 1);
			}
		}
		
		return 1;
	}
	
	
	
	return 0;
}

int mus_load_wavepatch(FILE *f, WgSettings *settings) //wasn't there
{
	RWops *rw = RWFromFP(f, 0);

	if (rw)
	{
		int r = mus_load_patch_RW(rw, settings);

		my_RWclose(rw);

		return r;
	}
	
	return 0;
}
#endif

Uint32 mus_get_playtime_at(MusSong *song, int position)
{
	Uint32 ticks = 0;
	int pos = 0;
	int seq_pos[MUS_MAX_CHANNELS] = {0}, pattern_pos[MUS_MAX_CHANNELS] = {0};
	MusPattern *pattern[MUS_MAX_CHANNELS] = {0};
	int spd1 = song->song_speed, spd2 = song->song_speed2, rate = song->song_rate;

	while (pos < position)
	{
		for (int t = 0; t < song->num_channels; ++t)
		{
			if (seq_pos[t] < song->num_sequences[t])
			{
				if (song->sequence[t][seq_pos[t]].position == pos)
				{
					if (seq_pos[t] < song->num_sequences[t])
					{
						pattern_pos[t] = 0;

						pattern[t] = &song->pattern[song->sequence[t][seq_pos[t]].pattern];

						seq_pos[t]++;
					}
				}

				if (pattern[t] && pattern_pos[t] < pattern[t]->num_steps)
				{
					for(int i = 0; i < MUS_MAX_COMMANDS; ++i)
					{
						Uint16 command = pattern[t]->step[pattern_pos[t]].command[i];

						if ((command & 0xff00) == MUS_FX_SET_SPEED)
						{
							spd1 = command & 0xf;
							spd2 = (command & 0xf0) >> 4;

							if (!spd2)
								spd2 = spd1;
						}
						
						else if ((command & 0xff00) == MUS_FX_SET_RATE)
						{
							rate = command & 0xff;

							if (rate < 1)
								rate = 1;
						}
						
						else if ((command & 0xff00) == MUS_FX_SKIP_PATTERN)
						{
							pos += pattern[t]->num_steps - 1 - pattern_pos[t];
						}
					}
				}

			}

			pattern_pos[t]++;
		}

		int spd = pos & 1 ? spd2 : spd1;

		ticks += (1000 * spd) / rate;

		++pos;
	}

	return ticks;
}


void mus_set_channel_volume(MusEngine* mus, int chan, int volume)
{
	MusChannel *chn = &mus->channel[chan];
	CydChannel *cydchn = &mus->cyd->channel[chan];
	MusTrackStatus *track_status = &mus->song_track[chan];

	chn->volume = my_min(volume, MAX_VOLUME);
	update_volumes(mus, track_status, chn, cydchn, track_status->volume);
}