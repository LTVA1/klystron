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

#include "docommand.h"
#include "cydfx.h" //wasn't there
#include "cyd.h"

#ifndef STANDALONE_COMPILE

#include "../../../src/mused.h" //wasn't there

#endif

#ifdef GENERATE_VIBRATO_TABLES

#include <math.h>

#endif

#ifndef STANDALONE_COMPILE

extern Mused mused;

#endif

#ifndef GENERATE_VIBRATO_TABLES

const Sint8 rnd_table[VIB_TAB_SIZE] = {
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

const Sint8 sine_table[VIB_TAB_SIZE] =
{
	0, 6, 12, 18, 24, 31, 37, 43, 48, 54, 60, 65, 71, 76, 81, 85, 90, 94, 98, 102, 106, 109, 112,
	115, 118, 120, 122, 124, 125, 126, 127, 127, 127, 127, 127, 126, 125, 124, 122, 120, 118, 115, 112,
	109, 106, 102, 98, 94, 90, 85, 81, 76, 71, 65, 60, 54, 48, 43, 37, 31, 24, 18, 12, 6,
	0, -6, -12, -18, -24, -31, -37, -43, -48, -54, -60, -65, -71, -76, -81, -85, -90, -94, -98, -102,
	-106, -109, -112, -115, -118, -120, -122, -124, -125, -126, -127, -127, -128, -127, -127, -126, -125, -124, -122,
	-120, -118, -115, -112, -109, -106, -102, -98, -94, -90, -85, -81, -76, -71, -65, -60, -54, -48, -43, -37, -31, -24, -18, -12, -6
};

#else

Sint8 rnd_table[VIB_TAB_SIZE];
Sint8 sine_table[VIB_TAB_SIZE];

#endif

const Uint16 resonance_table[] = {10, 512, 1300, 1950};

int mus_trigger_instrument_internal(MusEngine* mus, int chan, MusInstrument *ins, Uint16 note, int panning, bool update_adsr);

void mus_free_inst_samples(MusInstrument* inst)
{
	if(inst->local_samples)
	{
		for(int i = 0; i < inst->num_local_samples; i++)
		{
			if(inst->local_samples[i])
			{
				cyd_wave_entry_deinit(inst->local_samples[i]);
				free(inst->local_samples[i]);
				inst->local_samples[i] = NULL;
			}
		}
	}
	
	if(inst->local_sample_names)
	{
		for(int i = 0; i < inst->num_local_samples; i++)
		{
			if(inst->local_sample_names[i])
			{
				free(inst->local_sample_names[i]);
				inst->local_sample_names[i] = NULL;
			}
		}
		
		inst->num_local_samples = 0;
	}
	
	if(inst->local_samples)
	{
		free(inst->local_samples);
		inst->local_samples = NULL;
	}
	
	if(inst->local_sample_names)
	{
		free(inst->local_sample_names);
		inst->local_sample_names = NULL;
	}
}

void mus_free_inst_programs(MusInstrument* inst) //because memory for programs is dynamically allocated we need to free() it when we delete/overwrite instrument
{
	for(int i = 0; i < MUS_MAX_MACROS_INST; ++i)
	{
		if(inst->program[i])
		{
			free(inst->program[i]);
			inst->program[i] = NULL;
		}
		
		if(inst->program_unite_bits[i])
		{
			free(inst->program_unite_bits[i]);
			inst->program_unite_bits[i] = NULL;
		}
	}
	
	for(int op = 0; op < CYD_FM_NUM_OPS; ++op)
	{
		for(int i = 0; i < MUS_MAX_MACROS_OP; ++i)
		{
			if(inst->ops[op].program[i])
			{
				free(inst->ops[op].program[i]);
				inst->ops[op].program[i] = NULL;
			}
			
			if(inst->ops[op].program_unite_bits[i])
			{
				free(inst->ops[op].program_unite_bits[i]);
				inst->ops[op].program_unite_bits[i] = NULL;
			}
		}
	}
}

void update_volumes(MusEngine *mus, MusTrackStatus *ts, MusChannel *chn, CydChannel *cydchn, int volume)
{
	if (chn->instrument && (chn->instrument->flags & MUS_INST_RELATIVE_VOLUME))
	{
		ts->volume = volume;
		cydchn->adsr.volume = (chn->flags & MUS_CHN_DISABLED) ? 0 : (int)chn->instrument->volume * volume / MAX_VOLUME * (int)mus->volume / MAX_VOLUME * (int)mus->play_volume / MAX_VOLUME * (int)chn->volume / MAX_VOLUME * (int)chn->program_volume / MAX_VOLUME;
		
		if((cydchn->fm.flags & CYD_FM_ENABLE_ADDITIVE) && (cydchn->flags & CYD_CHN_ENABLE_FM))
		{
			cydchn->fm.adsr.volume = (chn->flags & MUS_CHN_DISABLED) ? 0 : (int)chn->instrument->fm_modulation * volume / MAX_VOLUME * (int)mus->volume / MAX_VOLUME * (int)mus->play_volume / MAX_VOLUME * (int)chn->volume / MAX_VOLUME * (int)chn->program_volume / MAX_VOLUME;
		}
	}
	
	else
	{
		ts->volume = volume;
		cydchn->adsr.volume = (chn->flags & MUS_CHN_DISABLED) ? 0 : ts->volume * (int)mus->volume / MAX_VOLUME * (int)mus->play_volume / MAX_VOLUME * (int)chn->volume / MAX_VOLUME * (int)chn->program_volume / MAX_VOLUME;
		
		if((cydchn->fm.flags & CYD_FM_ENABLE_ADDITIVE) && (cydchn->flags & CYD_CHN_ENABLE_FM))
		{
			cydchn->fm.adsr.volume = (chn->flags & MUS_CHN_DISABLED) ? 0 : ts->fm_volume * (int)mus->volume / MAX_VOLUME * (int)mus->play_volume / MAX_VOLUME * (int)mus->play_volume / MAX_VOLUME * (int)chn->volume / MAX_VOLUME * (int)chn->program_volume / MAX_VOLUME;
			//debug("2, vol %d", cydchn->fm.adsr.volume);
		}
	}
}

void update_fm_op_volume(MusEngine *mus, MusTrackStatus *track, MusChannel *chn, CydChannel *cydchn, int volume, Uint8 i /*number of operator*/)
{
	if (chn->instrument && (chn->instrument->ops[i].flags & MUS_FM_OP_RELATIVE_VOLUME))
	{
		track->ops_status[i].volume = volume;
		
		cydchn->fm.ops[i].adsr.volume = (chn->flags & MUS_CHN_DISABLED) ? 0 : ((int)chn->instrument->ops[i].volume * volume / MAX_VOLUME /* * (int)mus->volume / MAX_VOLUME */ * (int)mus->play_volume / MAX_VOLUME * (int)chn->volume / MAX_VOLUME * (int)chn->ops[i].program_volume / MAX_VOLUME);
	}
	
	else
	{
		track->ops_status[i].volume = volume;
		
		cydchn->fm.ops[i].adsr.volume = (chn->flags & MUS_CHN_DISABLED) ? 0 : (track->ops_status[i].volume /* * (int)mus->volume / MAX_VOLUME */ * (int)mus->play_volume / MAX_VOLUME * (int)chn->volume / MAX_VOLUME * (int)chn->ops[i].program_volume / MAX_VOLUME);
	}
}

void update_all_volumes(MusEngine *mus)
{
	for (int i = 0; i < MUS_MAX_CHANNELS && i < mus->cyd->n_channels; ++i)
	{
		update_volumes(mus, &mus->song_track[i], &mus->channel[i], &mus->cyd->channel[i], mus->song_track[i].volume);
	}
}


void mus_set_buzz_frequency(MusEngine *mus, int chan, Uint16 note)
{
#ifndef CYD_DISABLE_BUZZ
	MusChannel *chn = &mus->channel[chan];
	if (chn->instrument) //&& chn->instrument->flags & MUS_INST_YM_BUZZ)
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


void mus_set_wavetable_frequency(MusEngine *mus, int chan, Uint16 note)
{
#ifndef CYD_DISABLE_WAVETABLE
	MusChannel *chn = &mus->channel[chan];
	CydChannel *cydchn = &mus->cyd->channel[chan];
	MusTrackStatus *track_status = &mus->song_track[chan];

	if (chn->instrument && ((cydchn->flags & CYD_CHN_ENABLE_WAVE) || (chn->flags & MUS_INST_USE_LOCAL_SAMPLES)) && (cydchn->wave_entry))
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

void mus_set_slide(MusEngine *mus, int chan, Uint32 note)
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
		
		case MUS_NOTE_VOLUME_FADE_DN_FINE:
			do_command(mus, chan, mus->song_counter, MUS_FX_EXT_FADE_VOLUME_DN | ((Uint16)(vol & 0xf)), 0, mus->channel[chan].instrument != NULL ? ((mus->channel[chan].instrument->fm_flags & CYD_FM_FOUROP_USE_MAIN_INST_PROG) ? 0xFF : 0) : 0, 0);
		break;
		
		case MUS_NOTE_VOLUME_FADE_UP_FINE:
			if((vol & 0x0f) > 0) //because otherwise we have 0x80 which is valid volume value
			{
				do_command(mus, chan, mus->song_counter, MUS_FX_EXT_FADE_VOLUME_UP | ((Uint16)(vol & 0xf)), 0, mus->channel[chan].instrument != NULL ? ((mus->channel[chan].instrument->fm_flags & CYD_FM_FOUROP_USE_MAIN_INST_PROG) ? 0xFF : 0) : 0, 0);
			}
			
			else
			{
				goto volume; //if we have 0x80 set the volume lol
			}
		break;

		default:
			volume:;
			
			if (vol <= MAX_VOLUME && track_status->pattern->step[track_status->pattern_step].note != MUS_NOTE_CUT)
			{
				do_command(mus, chan, ((first_tick == 1) ? 0 : mus->song_counter), MUS_FX_SET_VOLUME | (Uint16)(vol), 0, mus->channel[chan].instrument != NULL ? ((mus->channel[chan].instrument->fm_flags & CYD_FM_FOUROP_USE_MAIN_INST_PROG) ? 0xFF : 0) : 0, 0);
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
	chn->ops[i].finetune_note = 0;
	
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
	chn->ops[i].program_volume = MAX_VOLUME;
	
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
					cydflt_set_coeff_old(&fm->ops[i].flts[j][sub], ins->ops[i].cutoff, resonance_table[(ins->ops[i].resonance >> 4) & 3]);
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

	chn->flags = MUS_CHN_PLAYING | (chn->flags & (MUS_CHN_DISABLED | MUS_CHN_GLISSANDO));
	
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
	chn->finetune_note = 0;
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
	
	#ifndef CYD_DISABLE_WAVETABLE
	if(!(ins->flags & MUS_INST_USE_LOCAL_SAMPLES))
	{
		if (ins->cydflags & CYD_CHN_ENABLE_WAVE)
		{
			cyd_set_wave_entry(cydchn, &mus->cyd->wavetable_entries[ins->wavetable_entry]);
		}
		
		else
		{
			cyd_set_wave_entry(cydchn, NULL);
		}
	}
	
	else
	{
		if(ins->flags & MUS_INST_BIND_LOCAL_SAMPLES_TO_NOTES)
		{
			if (ins->cydflags & CYD_CHN_ENABLE_WAVE)
			{
				if(ins->note_to_sample_array[note >> 8].sample != MUS_NOTE_TO_SAMPLE_NONE)
				{
					if(ins->note_to_sample_array[note >> 8].flags & MUS_NOTE_TO_SAMPLE_GLOBAL)
					{
						cyd_set_wave_entry(cydchn, &mus->cyd->wavetable_entries[ins->note_to_sample_array[note >> 8].sample]);
					}
					
					else
					{
						if(ins->note_to_sample_array[note >> 8].sample < ins->num_local_samples)
						{
							cyd_set_wave_entry(cydchn, ins->local_samples[ins->note_to_sample_array[note >> 8].sample]);
						}
						
						else
						{
							cyd_set_wave_entry(cydchn, NULL);
						}
					}
				}
				
				else
				{
					cyd_set_wave_entry(cydchn, NULL);
				}
			}
		}
		
		else
		{
			if (ins->cydflags & CYD_CHN_ENABLE_WAVE)
			{
				if(ins->num_local_samples > ins->local_sample)
				{
					cyd_set_wave_entry(cydchn, ins->local_samples[ins->local_sample]);
				}
				
				else
				{
					cyd_set_wave_entry(cydchn, NULL);
				}
			}
			
			else
			{
				cyd_set_wave_entry(cydchn, NULL);
			}
		}
	}
	
	for(int i = 0; i < CYD_SUB_OSCS; ++i)
	{
		cydchn->subosc[i].wave.end_offset = 0xffff;
		cydchn->subosc[i].wave.start_offset = 0;
		
		cydchn->subosc[i].wave.start_point_track_status = 0;
		cydchn->subosc[i].wave.end_point_track_status = 0;
		
		cydchn->subosc[i].wave.use_start_track_status_offset = false;
		cydchn->subosc[i].wave.use_end_track_status_offset = false;
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
	chn->program_volume = MAX_VOLUME;

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

	mus->channel[chan].buzz_offset = ins->buzz_offset;
	cyd_set_env_shape(cydchn, ins->ym_env_shape);

	if (ins->flags & MUS_INST_YM_BUZZ)
	{
#ifndef CYD_DISABLE_BUZZ
		cydchn->flags |= CYD_CHN_ENABLE_YM_ENV;
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

	if ((ins->flags & MUS_INST_DRUM) && chn->current_tick == 1)
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
				
				vibdep = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x000f) << 4;
				
				track_status->vibrato_depth = vibdep;
				
				vibspd = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x00f0) >> 1;
				
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
						vibdep_ops[j] = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x000f) << 4;
						track_status->ops_status[j].vibrato_depth = vibdep_ops[j];
						
						vibspd_ops[j] = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x00f0) >> 1;
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
				
				tremdep = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x000f) << 4;
				
				track_status->tremolo_depth = tremdep;
				
				tremspd = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x00f0) >> 1;
				
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
						tremdep_ops[j] = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x000f) << 4;
						track_status->ops_status[j].tremolo_depth = tremdep_ops[j];
						
						tremspd_ops[j] = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x00f0) >> 1;
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
				fm_vibdep = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x000f) << 4;
				
				track_status->fm_vibrato_depth = fm_vibdep;
				
				fm_vibspd = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x00f0) >> 1;
				
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
				fm_tremdep = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x000f) << 4;
				
				track_status->fm_tremolo_depth = fm_tremdep;
				
				fm_tremspd = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x00f0) >> 1;
				
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
				track_status->pwm_speed = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x00f0) >> 3;
				
				if(cydchn->fm.flags & CYD_FM_ENABLE_4OP)
				{
					for(int j = 0; j < CYD_FM_NUM_OPS; ++j)
					{
						track_status->ops_status[j].pwm_delay = 0;
				
						track_status->ops_status[j].pwm_depth = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x000f) << 4;
						track_status->ops_status[j].pwm_speed = (track_status->pattern->step[track_status->pattern_step].command[i] & 0x00f0) >> 3;
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

	Sint32 intermediate_note = (mus->channel[chan].fixed_note != 0xffff ? mus->channel[chan].fixed_note : mus->channel[chan].note);

	if(mus->channel[chan].flags & MUS_CHN_GLISSANDO)
	{
		intermediate_note &= MUS_GLISSANDO_MASK;
	}
	
	Sint32 note = intermediate_note + vib + mus->channel[chan].finetune_note + ((Sint16)mus->channel[chan].arpeggio_note << 8);

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

			Sint32 intermediate_note_ops = (mus->channel[chan].ops[i].fixed_note != 0xffff ? mus->channel[chan].ops[i].fixed_note : mus->channel[chan].ops[i].note);

			if(mus->channel[chan].ops[i].flags & MUS_FM_OP_GLISSANDO)
			{
				intermediate_note_ops &= MUS_GLISSANDO_MASK;
			}

			Sint32 note_ops = intermediate_note_ops + ops_vib[i] + mus->channel[chan].ops[i].finetune_note + ((Sint16)mus->channel[chan].ops[i].arpeggio_note << 8);

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
											if(!(muschn->ops[i].flags & MUS_FM_OP_DISABLED))
											{
												cydchn->fm.ops[i1].adsr.volume = 0;
												track_status->ops_status[i1].volume = 0;
											}
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

								if (inst != MUS_NOTE_NO_INSTRUMENT && !(pinst->flags & MUS_INST_KEEP_VOLUME_ON_SLIDE_AND_LEGATO))
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

				if (track_status->pattern) mus_exec_track_command(mus, i, ((mus->song_counter == delay) ? 1 : 0));
			}

			++mus->song_counter;
			
			bool advance_pattern_step = false;
			
			if(!(mus->flags & MUS_ENGINE_USE_GROOVE))
			{
				if(mus->song_counter >= ((!(mus->song_position & 1)) ? mus->song->song_speed : mus->song->song_speed2))
				{
					advance_pattern_step = true;
				}
			}
			
			if(mus->flags & MUS_ENGINE_USE_GROOVE)
			{
				if(mus->song->groove_length[mus->groove_number] > 0)
				{
					if(mus->song_counter >= mus->song->grooves[mus->groove_number][mus->song_position % mus->song->groove_length[mus->groove_number]])
					{
						advance_pattern_step = true;
					}
				}
				
				else // to not have a crash from modulo 0; just play at virtually speed 1
				{
					advance_pattern_step = true;
				}
			}
			
			if(advance_pattern_step)
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
								break;
							}
							
							else if ((command & 0xff00) == MUS_FX_SKIP_PATTERN)
							{
								//mus->song_position += my_max(track_status->pattern->num_steps - track_status->pattern_step - 1 + (command & 0xff), 0);
								mus->song_position += track_status->pattern->num_steps - track_status->pattern_step - 1 + (command & 0xff);
								track_status->pattern = NULL;
								//track_status->pattern_step = (command & 0xff);
								flag = 0;
								break;
							}

							else if ((command & 0xff00) == MUS_FX_JUMP_SEQUENCE_POSITION) //Bxx style effect, jump to order position xx (well, for it to make sense patterns must be aligned)
							{
								if((command & 0xff) < mus->song->num_sequences[i])
								{
									mus->song_position = mus->song->sequence[i][command & 0xff].position - 1;

									for(int chns = 0; chns < mus->song->num_channels; ++chns)
									{
										mus->song_track[chns].pattern = &mus->song->pattern[mus->song->sequence[chns][command & 0xff].pattern];
										mus->song_track[chns].pattern_step = 0;
										mus->song_track[chns].sequence_position = command & 0xff;
									}

									//track_status->pattern = &mus->song->pattern[mus->song->sequence[i][command & 0xff].pattern];
									//track_status->pattern_step = 0;
									flag = 0;
									break;
								}
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
														if(mus->song_track[chns].pattern_step >= delta - 1)
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
														if(mus->song_track[chns].pattern_step >= delta - 1)
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
		CydChannel *cydchn = &mus->cyd->channel[i];
		
		if(cydchn->fm.flags & CYD_FM_ENABLE_ADDITIVE)
		{
			MusTrackStatus *track_status = &mus->song_track[i];
			MusChannel *muschn = &mus->channel[i];
		
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

		mus->channel[chan].finetune_note = 0;

		mus->channel[chan].program_volume = MAX_VOLUME;
		
		mus->channel[chan].flags &= ~MUS_CHN_GLISSANDO; //disable glissando

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

			mus->channel[chan].ops[op].finetune_note = 0;

			mus->channel[chan].ops[op].program_volume = MAX_VOLUME;

			mus->channel[chan].ops[op].flags &= ~MUS_CHN_GLISSANDO;
			
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
	
	if(song != NULL && mus->song != NULL)
	{
		if(song->flags & MUS_USE_GROOVE)
		{
			mus->flags |= MUS_ENGINE_USE_GROOVE;
			mus->groove_number = 0;
		}
		
		else
		{
			mus->flags &= ~(MUS_ENGINE_USE_GROOVE);
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

void mus_free_song(MusSong *song)
{
	for (int i = 0; i < song->num_instruments; ++i)
	{
		mus_free_inst_programs(&song->instrument[i]);
		mus_free_inst_samples(&song->instrument[i]);
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


void delay(int milliseconds)
{
    long pause;
    clock_t now,then;

    pause = milliseconds*(CLOCKS_PER_SEC/1000);
    now = then = clock();
    while( (now-then) < pause )
        now = clock();
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

void mus_get_default_instrument(MusInstrument *inst)
{
	mus_free_inst_programs(inst);
	mus_free_inst_samples(inst);
	
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
	
	inst->program[0] = (Uint16*)calloc(1, MUS_PROG_LEN * sizeof(Uint16));
	inst->program_unite_bits[0] = (Uint8*)calloc(1, (MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
	
	inst->local_samples = (CydWavetableEntry**)calloc(1, sizeof(CydWavetableEntry*));
	inst->local_sample_names = (char**)calloc(1, sizeof(char*));
	
	inst->local_samples[0] = (CydWavetableEntry*)calloc(1, sizeof(CydWavetableEntry));
	inst->local_sample_names[0] = (char*)calloc(1, sizeof(char) * MUS_WAVETABLE_NAME_LEN);
	
	cyd_wave_entry_init(inst->local_samples[0], NULL, 0, 0, 0, 0, 0);
	
	inst->num_local_samples = 1;
	
	inst->num_macros = 1;

	for (int p = 0; p < MUS_PROG_LEN; ++p)
	{
		inst->program[0][p] = MUS_FX_NOP;
	}
	
	for (int p = 0; p < MUS_PROG_LEN / 8 + 1; ++p)
	{
		inst->program_unite_bits[0][p] = 0;
	}
	
	for(int i = 0; i < FREQ_TAB_SIZE; i++)
	{
		inst->note_to_sample_array[i] = (MusInstNoteToSample) { .note = i, .sample = MUS_NOTE_TO_SAMPLE_NONE, .flags = 0 };
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
		
		inst->ops[i].program[0] = (Uint16*)calloc(1, MUS_PROG_LEN * sizeof(Uint16));
		inst->ops[i].program_unite_bits[0] = (Uint8*)calloc(1, (MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
		
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
	mus_free_inst_samples(inst);
	
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
	
	inst->program[0] = (Uint16*)calloc(1, MUS_PROG_LEN * sizeof(Uint16));
	inst->program_unite_bits[0] = (Uint8*)calloc(1, (MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
	
	inst->local_samples = (CydWavetableEntry**)calloc(1, sizeof(CydWavetableEntry*));
	inst->local_sample_names = (char**)calloc(1, sizeof(char*));
	
	inst->local_samples[0] = (CydWavetableEntry*)calloc(1, sizeof(CydWavetableEntry));
	inst->local_sample_names[0] = (char*)calloc(1, sizeof(char) * MUS_WAVETABLE_NAME_LEN);
	
	cyd_wave_entry_init(inst->local_samples[0], NULL, 0, 0, 0, 0, 0);
	
	inst->num_local_samples = 1;
	
	inst->num_macros = 1;

	for (int p = 0; p < MUS_PROG_LEN; ++p)
	{
		inst->program[0][p] = MUS_FX_NOP;
	}
	
	for (int p = 0; p < MUS_PROG_LEN / 8 + 1; ++p)
	{
		inst->program_unite_bits[0][p] = 0;
	}
	
	for(int i = 0; i < FREQ_TAB_SIZE; i++)
	{
		inst->note_to_sample_array[i] = (MusInstNoteToSample) { .note = i, .sample = MUS_NOTE_TO_SAMPLE_NONE, .flags = 0 };
	}
	
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
		
		inst->ops[i].program[0] = (Uint16*)calloc(1, MUS_PROG_LEN * sizeof(Uint16));
		inst->ops[i].program_unite_bits[0] = (Uint8*)calloc(1, (MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
		
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