#include "diskopload.h"
#include "pack.h"

#ifndef STANDALONE_COMPILE
#include "../../../src/wavegen.h"
#endif

#ifndef STANDALONE_COMPILE

#include "../../../src/mused.h" //wasn't there

#endif

RWops * RWFromFP(FILE *f, int close)
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


RWops * RWFromFile(const char *name, const char *mode)
{
#ifdef USESDL_RWOPS
	return SDL_RWFromFile(name, mode);
#else
	FILE *f = fopen(name, mode);

	if (!f) return NULL;

	return RWFromFP(f, 1);
#endif
}

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

void load_wavetable_entry(Uint8 version, CydWavetableEntry* e, RWops *ctx)
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
			Sint16 *data = calloc(1, sizeof(data[0]) * e->samples);

			my_RWread(ctx, data, sizeof(data[0]), e->samples);

			cyd_wave_entry_init(e, data, e->samples, CYD_WAVE_TYPE_SINT16, 1, 1, 1);

			free(data);
		}
		
		else
		{
			Uint32 data_size = 0;
			VER_READ(version, 15, 0xff, &data_size, 0);
			FIX_ENDIAN(data_size);
			Uint8 *compressed = calloc(1, sizeof(Uint8) * data_size);

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
				inst->program[pr] = (Uint16*)calloc(1, MUS_PROG_LEN * sizeof(Uint16));
				inst->program_unite_bits[pr] = (Uint8*)calloc(1, (MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
				
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
		if(version < 33 || version >= 42)
		{
			VER_READ(version, 1, 0xff, &inst->cutoff, 0);
			VER_READ(version, 1, 0xff, &inst->resonance, 0);
		}
		
		if(version >= 33 && version < 42)
		{
			Uint16 temp = 0;
			VER_READ(version, 33, 0xff, &temp, 0);
			
			inst->cutoff = temp & 0x0fff;
			inst->resonance = (temp & 0xf000) >> 12;
		}
		
		if(version < 42)
		{
			inst->resonance *= 16;
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
	
	if(inst->flags & MUS_INST_USE_LOCAL_SAMPLES)
	{
		mus_free_inst_samples(inst);
		
		VER_READ(version, 42, 0xff, &inst->num_local_samples, 0);
		
		inst->local_samples = (CydWavetableEntry**)calloc(1, sizeof(CydWavetableEntry*) * inst->num_local_samples);
		inst->local_sample_names = (char**)calloc(1, sizeof(char*) * inst->num_local_samples);
		
		for(int i = 0; i < inst->num_local_samples; i++)
		{
			inst->local_samples[i] = (CydWavetableEntry*)calloc(1, sizeof(CydWavetableEntry));
			cyd_wave_entry_init(inst->local_samples[i], NULL, 0, 0, 0, 0, 0);
			
			inst->local_sample_names[i] = (char*)calloc(1, sizeof(char) * MUS_WAVETABLE_NAME_LEN);
			memset(inst->local_sample_names[i], 0, sizeof(char) * MUS_WAVETABLE_NAME_LEN);
			
			Uint8 wave_name_len = 0;
			
			VER_READ(version, 42, 0xff, &wave_name_len, 0);
			
			if(wave_name_len > 0)
			{
				_VER_READ(inst->local_sample_names[i], sizeof(inst->local_sample_names[i][0]) * wave_name_len);
			}
			
			load_wavetable_entry(version, inst->local_samples[i], ctx);
		}
		
		if(inst->flags & MUS_INST_BIND_LOCAL_SAMPLES_TO_NOTES)
		{
			Uint8 num_note_binds = 0;
			
			VER_READ(version, 42, 0xff, &num_note_binds, 0);
			
			for(int i = 0; i < num_note_binds; i++)
			{
				Uint8 note = 0;
				
				VER_READ(version, 42, 0xff, &note, 0);
				
				inst->note_to_sample_array[note].note = note;
				
				VER_READ(version, 42, 0xff, &inst->note_to_sample_array[note].flags, 0);
				VER_READ(version, 42, 0xff, &inst->note_to_sample_array[note].sample, 0);
			}
		}
	}
	
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
							inst->ops[i].program[pr] = (Uint16*)calloc(1, MUS_PROG_LEN * sizeof(Uint16));
							inst->ops[i].program_unite_bits[pr] = (Uint8*)calloc(1, (MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
							
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
					if(version < 42)
					{
						VER_READ(version, 34, 0xff, &inst->ops[i].cutoff, 0);
						
						inst->ops[i].resonance = inst->ops[i].cutoff >> 12;
						inst->ops[i].cutoff &= 0xfff;
					}
					
					else
					{
						VER_READ(version, 34, 0xff, &inst->ops[i].cutoff, 0);
						VER_READ(version, 34, 0xff, &inst->ops[i].resonance, 0);
					}
					
					if(version < 42)
					{
						inst->ops[i].resonance *= 16;
					}
					
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
	
	if(version < 42) // account for extended resonance range
	{
		for(int i = 0; i < inst->num_macros; i++)
		{
			for(int j = 0; j < MUS_PROG_LEN; j++)
			{
				if((inst->program[i][j] & 0xff00) == MUS_FX_RESONANCE_SET)
				{
					inst->program[i][j] = (inst->program[i][j] & 0xff00) | ((inst->program[i][j] & 0xff) * 16);
				}
			}
		}
		
		if(inst->fm_flags & CYD_FM_ENABLE_4OP)
		{
			for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
			{
				for(int j = 0; j < inst->ops[i].num_macros; j++)
				{
					for(int k = 0; k < MUS_PROG_LEN; k++)
					{
						if((inst->ops[i].program[j][k] & 0xff00) == MUS_FX_RESONANCE_SET)
						{
							inst->ops[i].program[j][k] = (inst->ops[i].program[j][k] & 0xff00) | ((inst->ops[i].program[j][k] & 0xff) * 16);
						}
					}
				}
			}
		}
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
		
		if(song->flags & MUS_SAVE_GROOVE)
		{
			Uint8 num_grooves = 0;
			
			my_RWread(ctx, &num_grooves, 1, sizeof(num_grooves));
			
			for(int i = 0; i < num_grooves; i++)
			{
				my_RWread(ctx, &song->groove_length[i], 1, sizeof(song->groove_length[i]));
				
				for(int j = 0; j < song->groove_length[i]; j++)
				{
					my_RWread(ctx, &song->grooves[i][j], 1, sizeof(song->grooves[i][j]));
				}
			}
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
			song->instrument = calloc(1, (size_t)song->num_instruments * sizeof(song->instrument[0]));
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
					song->sequence[i] = calloc(1, (size_t)song->num_sequences[i] * sizeof(song->sequence[0][0]));

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
				free(song->pattern[i].step);
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

				Uint8 *packed = calloc(1, sizeof(Uint8) * len);
				Uint8 *current = packed;

				my_RWread(ctx, packed, sizeof(Uint8), len);

				for (int s = 0; s < song->pattern[i].num_steps; ++s)
				{
					Uint8 bits = (s & 1 || s == song->pattern[i].num_steps - 1) ? (*current & 0xf) : (*current >> 4);

					if (bits & MUS_PAK_BIT_NOTE)
					{
						my_RWread(ctx, &song->pattern[i].step[s].note, 1, sizeof(song->pattern[i].step[s].note));
						
						if(version < 35 && song->pattern[i].step[s].note != MUS_NOTE_NONE && song->pattern[i].step[s].note != MUS_NOTE_RELEASE && song->pattern[i].step[s].note != MUS_NOTE_CUT && song->pattern[i].step[s].note != MUS_NOTE_MACRO_RELEASE && song->pattern[i].step[s].note != MUS_NOTE_RELEASE_WITHOUT_MACRO)
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
					
					if(version < 41)
					{
						for(int k = 0; k < MUS_MAX_COMMANDS; k++)
						{
							if((song->pattern[i].step[s].command[k] & 0xff00) == MUS_FX_VIBRATO)
							//to account for different vibrato speed and depth
							{
								Uint8 temp = song->pattern[i].step[s].command[k] & 0xff;
								
								temp = ((temp & 0xf) / 4) | ((((temp & 0xf) >> 4) / 2) << 4);
								
								song->pattern[i].step[s].command[k] &= 0xff00;
								song->pattern[i].step[s].command[k] |= temp;
							}
						}
					}
					
					if(version < 42)
					{
						for(int k = 0; k < MUS_MAX_COMMANDS; k++)
						{
							if((song->pattern[i].step[s].command[k] & 0xff00) == MUS_FX_RESONANCE_SET)
							//to account for extended filter resonance range
							{
								Uint8 temp = song->pattern[i].step[s].command[k] & 0xff;
								
								temp = temp << 4;
								
								song->pattern[i].step[s].command[k] &= 0xff00;
								song->pattern[i].step[s].command[k] |= temp;
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
		}

#ifndef STANDALONE_COMPILE
		for(int ch = 0; ch < song->num_channels; ch++)
		{
			Uint8 cols = 0;

			for(int i = 0; i < song->num_sequences[ch]; i++)
			{
				MusPattern* pat = &song->pattern[song->sequence[ch][i].pattern];

				for(int s = 0; s < pat->num_steps; s++)
				{
					MusStep* step = &pat->step[s];

					for(int com = 0; com < MUS_MAX_COMMANDS; com++)
					{
						if(step->command[com] && com > cols)
						{
							cols = com;
						}
					}
				}
			}

			mused.command_columns[ch] = cols;
		}
#endif
		
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

				song->wavetable_names = calloc(1, max_wt * sizeof(char*));
				memset(song->wavetable_names, 0, max_wt * sizeof(char*));

				if (version >= 26)
				{
					for (int i = 0; i < max_wt; ++i)
					{
						Uint8 len = 0;
						song->wavetable_names[i] = calloc(1, 32 + 1);
						memset(song->wavetable_names[i], 0, 32 + 1);

						my_RWread(ctx, &len, 1, 1);
						my_RWread(ctx, song->wavetable_names[i], len, sizeof(char));
					}
				}
				
				else
				{
					for (int i = 0; i < max_wt; ++i)
					{
						song->wavetable_names[i] = calloc(1, 32 + 1);
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

				song->wavetable_names = calloc(1, max_wt * sizeof(char*));
				memset(song->wavetable_names, 0, max_wt * sizeof(char*));

				if (version >= 26)
				{
					for (int i = 0; i < max_wt; ++i)
					{
						Uint8 len = 0;
						song->wavetable_names[i] = calloc(1, MUS_WAVETABLE_NAME_LEN + 1);
						memset(song->wavetable_names[i], 0, MUS_WAVETABLE_NAME_LEN + 1);

						my_RWread(ctx, &len, 1, 1);
						my_RWread(ctx, song->wavetable_names[i], len, sizeof(char));
					}
				}

				else
				{
					for (int i = 0; i < max_wt; ++i)
					{
						song->wavetable_names[i] = calloc(1, MUS_WAVETABLE_NAME_LEN + 1);
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