#include "docommand.h"

extern const Uint16 resonance_table[4];

#ifndef GENERATE_VIBRATO_TABLES

extern const Sint8 rnd_table[VIB_TAB_SIZE];

extern const Sint8 sine_table[VIB_TAB_SIZE];

#else

extern Sint8 rnd_table[VIB_TAB_SIZE];
extern Sint8 sine_table[VIB_TAB_SIZE];

#endif

void do_command(MusEngine *mus, int chan, int tick, Uint16 inst, int from_program, int ops_index, int prog_number) //ops_index 0 - do for main instrument and fm mod. only, 1-FE - do for op (ops_index - 1), FF - do for all ops and main instrument
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

		case MUS_FX_BUZZ_TOGGLE:
		{
			if ((tick == (inst & 0xf) && (inst & 0xf)) || (mus->channel[chan].prog_period[prog_number] <= 1 && (inst & 0xf) == 1))
			{
				if(chn->instrument)
				{
					cydchn->flags |= CYD_CHN_ENABLE_YM_ENV;
					cyd_set_env_shape(cydchn, cydchn->ym_env_shape);
					mus_set_buzz_frequency(mus, chan, chn->note);
				}
			}

			if(!(inst & 0xf))
			{
				cydchn->flags &= ~CYD_CHN_ENABLE_YM_ENV;
			}
		}
		break;
		
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

		case MUS_FX_GLISSANDO_CONTROL:
		{
			switch(ops_index)
			{
				case 0:
				case 0xFF:
				{
					if(inst & 0xf)
					{
						chn->flags |= MUS_CHN_GLISSANDO;
					}

					else
					{
						chn->flags &= ~MUS_CHN_GLISSANDO;
					}
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							if(inst & 0xf)
							{
								chn->ops[i].flags |= MUS_FM_OP_GLISSANDO;
							}

							else
							{
								chn->ops[i].flags &= ~MUS_FM_OP_GLISSANDO;
							}
						}
					}
					
					break;
				}
				
				default:
				{
					if(inst & 0xf)
					{
						chn->ops[ops_index - 1].flags |= MUS_FM_OP_GLISSANDO;
					}

					else
					{
						chn->ops[ops_index - 1].flags &= ~MUS_CHN_GLISSANDO;
					}
					break;
				}
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
							cydchn->fm.ops[i].adsr.r = (inst & 0x3f);
							
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
										cydflt_set_coeff_old(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, resonance_table[(track_status->ops_status[i].filter_resonance >> 4) & 3]);
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
								cydflt_set_coeff_old(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, resonance_table[(track_status->ops_status[ops_index - 1].filter_resonance >> 4) & 3]);
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
										cydflt_set_coeff_old(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, resonance_table[(track_status->ops_status[i].filter_resonance >> 4) & 3]);
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
								cydflt_set_coeff_old(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, resonance_table[(track_status->ops_status[ops_index - 1].filter_resonance >> 4) & 3]);
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
		
		case MUS_FX_RESONANCE_UP:
		{
			switch(ops_index)
			{
				case 0:
				case 0xFF:
				{
					Sint16 temp = track_status->filter_resonance + (inst & 0xff);
					
					if(temp >= 0 && temp <= 0xff)
					{
						track_status->filter_resonance += (inst & 0xff); //was `inst & 3;`
						cyd_set_filter_coeffs(mus->cyd, cydchn, track_status->filter_cutoff, track_status->filter_resonance); //cyd_set_filter_coeffs(mus->cyd, cydchn, track_status->filter_cutoff, inst & 3);
					}
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							Sint16 temp_op = track_status->ops_status[i].filter_resonance + (inst & 0xff);
					
							if(temp_op >= 0 && temp_op <= 0xff)
							{
								track_status->ops_status[i].filter_resonance += (inst & 0xff);
								
								for(int j = 0; j < (int)pow(2, cydchn->fm.ops[i].flt_slope); ++j)
								{
									for (int sub = 0; sub < CYD_SUB_OSCS; ++sub)
									{
										if(mus->cyd->flags & CYD_USE_OLD_FILTER)
										{
											cydflt_set_coeff_old(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, resonance_table[(track_status->ops_status[i].filter_resonance >> 4) & 3]);
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
					Sint16 temp = track_status->ops_status[ops_index - 1].filter_resonance + (inst & 0xff);
					
					if(temp >= 0 && temp <= 0xff)
					{
						track_status->ops_status[ops_index - 1].filter_resonance += (inst & 0xff);
						
						for(int j = 0; j < (int)pow(2, cydchn->fm.ops[ops_index - 1].flt_slope); ++j)
						{
							for (int sub = 0; sub < CYD_SUB_OSCS; ++sub)
							{
								if(mus->cyd->flags & CYD_USE_OLD_FILTER)
								{
									cydflt_set_coeff_old(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, resonance_table[(track_status->ops_status[ops_index - 1].filter_resonance >> 4) & 3]);
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
		
		case MUS_FX_RESONANCE_DOWN:
		{
			switch(ops_index)
			{
				case 0:
				case 0xFF:
				{
					Sint16 temp = track_status->filter_resonance - (inst & 0xff);
					
					if(temp >= 0 && temp <= 0xff)
					{
						track_status->filter_resonance -= (inst & 0xff); //was `inst & 3;`
						cyd_set_filter_coeffs(mus->cyd, cydchn, track_status->filter_cutoff, track_status->filter_resonance); //cyd_set_filter_coeffs(mus->cyd, cydchn, track_status->filter_cutoff, inst & 3);
					}
					
					if(ops_index == 0xFF)
					{
						for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
						{
							Sint16 temp_op = track_status->ops_status[i].filter_resonance - (inst & 0xff);
					
							if(temp_op >= 0 && temp_op <= 0xff)
							{
								track_status->ops_status[i].filter_resonance -= (inst & 0xff);
								
								for(int j = 0; j < (int)pow(2, cydchn->fm.ops[i].flt_slope); ++j)
								{
									for (int sub = 0; sub < CYD_SUB_OSCS; ++sub)
									{
										if(mus->cyd->flags & CYD_USE_OLD_FILTER)
										{
											cydflt_set_coeff_old(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, resonance_table[(track_status->ops_status[i].filter_resonance >> 4) & 3]);
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
					Sint16 temp = track_status->ops_status[ops_index - 1].filter_resonance - (inst & 0xff);
					
					if(temp >= 0 && temp <= 0xff)
					{
						track_status->ops_status[ops_index - 1].filter_resonance -= (inst & 0xff);
						
						for(int j = 0; j < (int)pow(2, cydchn->fm.ops[ops_index - 1].flt_slope); ++j)
						{
							for (int sub = 0; sub < CYD_SUB_OSCS; ++sub)
							{
								if(mus->cyd->flags & CYD_USE_OLD_FILTER)
								{
									cydflt_set_coeff_old(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, resonance_table[(track_status->ops_status[ops_index - 1].filter_resonance >> 4) & 3]);
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
												cydflt_set_coeff_old(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, resonance_table[(track_status->ops_status[i].filter_resonance >> 4) & 3]);
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
										cydflt_set_coeff_old(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, resonance_table[(track_status->ops_status[ops_index - 1].filter_resonance >> 4) & 3]);
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

	if (tick == 0)// || chn->prog_period[prog_number] < 2)
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
		
		switch (inst & 0xfff0)
		{
			case MUS_FX_EXT_TOGGLE_FILTER:
			{
				switch(ops_index)
				{
					case 0:
					case 0xFF:
					{
						if(inst & 0xf)
						{
							cydchn->flags |= CYD_CHN_ENABLE_FILTER;
						}
						
						else
						{
							cydchn->flags &= ~(CYD_CHN_ENABLE_FILTER);
						}
						
						if(ops_index == 0xFF)
						{
							for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
							{
								if(inst & 0xf)
								{
									cydchn->fm.ops[i].flags |= CYD_FM_OP_ENABLE_FILTER;
								}
								
								else
								{
									cydchn->fm.ops[i].flags &= ~(CYD_FM_OP_ENABLE_FILTER);
								}
							}
						}
						
						break;
					}
					
					default:
					{
						if(inst & 0xf)
						{
							cydchn->fm.ops[ops_index - 1].flags |= CYD_FM_OP_ENABLE_FILTER;
						}
						
						else
						{
							cydchn->fm.ops[ops_index - 1].flags &= ~(CYD_FM_OP_ENABLE_FILTER);
						}
						
						break;
					}
				}
				break;
			}
			
			default: break;
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
												cydflt_set_coeff_old(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, resonance_table[(track_status->ops_status[i].filter_resonance >> 4) & 3]);
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
										cydflt_set_coeff_old(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, resonance_table[(track_status->ops_status[ops_index - 1].filter_resonance >> 4) & 3]);
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
												cydflt_set_coeff_old(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, resonance_table[(track_status->ops_status[i].filter_resonance >> 4) & 3]);
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
										cydflt_set_coeff_old(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, resonance_table[(track_status->ops_status[ops_index - 1].filter_resonance >> 4) & 3]);
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
													cydflt_set_coeff_old(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, resonance_table[(track_status->ops_status[i].filter_resonance >> 4) & 3]);
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
													cydflt_set_coeff_old(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, resonance_table[(track_status->ops_status[i].filter_resonance >> 4) & 3]);
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
											cydflt_set_coeff_old(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, resonance_table[(track_status->ops_status[ops_index - 1].filter_resonance >> 4) & 3]);
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
											cydflt_set_coeff_old(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, resonance_table[(track_status->ops_status[ops_index - 1].filter_resonance >> 4) & 3]);
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
							track_status->filter_resonance = inst & 0xff; //was `inst & 3;`
							cyd_set_filter_coeffs(mus->cyd, cydchn, track_status->filter_cutoff, inst & 0xff); //cyd_set_filter_coeffs(mus->cyd, cydchn, track_status->filter_cutoff, inst & 3);
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									track_status->ops_status[i].filter_resonance = (inst & 0xff);
									
									for(int j = 0; j < (int)pow(2, cydchn->fm.ops[i].flt_slope); ++j)
									{
										for (int sub = 0; sub < CYD_SUB_OSCS; ++sub)
										{
											if(mus->cyd->flags & CYD_USE_OLD_FILTER)
											{
												cydflt_set_coeff_old(&cydchn->fm.ops[i].flts[j][sub], track_status->ops_status[i].filter_cutoff, resonance_table[(track_status->ops_status[i].filter_resonance >> 4) & 3]);
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
							track_status->ops_status[ops_index - 1].filter_resonance = (inst & 0xff);
							
							for(int j = 0; j < (int)pow(2, cydchn->fm.ops[ops_index - 1].flt_slope); ++j)
							{
								for (int sub = 0; sub < CYD_SUB_OSCS; ++sub)
								{
									if(mus->cyd->flags & CYD_USE_OLD_FILTER)
									{
										cydflt_set_coeff_old(&cydchn->fm.ops[ops_index - 1].flts[j][sub], track_status->ops_status[ops_index - 1].filter_cutoff, resonance_table[(track_status->ops_status[ops_index - 1].filter_resonance >> 4) & 3]);
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
						mus->flags &= ~(MUS_ENGINE_USE_GROOVE);
						
						if((inst & 0xf) > 0)
						{
							mus->song->song_speed = inst & 0xf;
						}
						
						if ((inst & 0xf0) == 0) mus->song->song_speed2 = mus->song->song_speed;
						
						else if(((inst >> 4) & 0xf) > 0)
						{
							mus->song->song_speed2 = (inst >> 4) & 0xf;
						}
					}
				}
				break;
				
				case MUS_FX_SET_SPEED1:
				{
					if (!from_program)
					{
						if((inst & 0xff) > 0)
						{
							mus->song->song_speed = inst & 0xff;
						}
						
						mus->flags &= ~(MUS_ENGINE_USE_GROOVE);
					}
				}
				break;
				
				case MUS_FX_SET_SPEED2:
				{
					if (!from_program)
					{
						if((inst & 0xff) > 0)
						{
							mus->song->song_speed2 = inst & 0xff;
						}
						
						mus->flags &= ~(MUS_ENGINE_USE_GROOVE);
					}
				}
				break;
				
				case MUS_FX_SET_GROOVE:
				{
					if (!from_program)
					{
						Uint8 groove = (inst & 0xff);
						
						if(groove < MUS_MAX_GROOVES && mus->song->groove_length[groove] > 0)
						{
							mus->groove_number = (inst & 0xff);
							mus->flags |= MUS_ENGINE_USE_GROOVE;
						}
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

				case MUS_FX_PITCH:
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							chn->finetune_note = (inst & 0xff) - 0x80;

							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									chn->ops[i].finetune_note = (inst & 0xff) - 0x80;
								}
							}
							
							break;
						}
						
						default: 
						{
							chn->ops[ops_index - 1].finetune_note = (inst & 0xff) - 0x80;
							break;
						}
					}
				}
				break;

				case MUS_FX_SLIDE_UP_SEMITONES:
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							chn->target_note = chn->note + ((inst & 0xf) << 8);
							track_status->slide_speed = (inst & 0xf0) << 1;

							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									chn->ops[i].target_note = chn->ops[i].note + ((inst & 0xf) << 8);
									track_status->ops_status[i].slide_speed = (inst & 0xf0) << 1;
								}
							}
							
							break;
						}
						
						default: 
						{
							chn->ops[ops_index - 1].target_note = chn->ops[ops_index - 1].note + ((inst & 0xf) << 8);
							track_status->ops_status[ops_index - 1].slide_speed = (inst & 0xf0) << 1;
							break;
						}
					}
				}
				break;

				case MUS_FX_SLIDE_DN_SEMITONES:
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							chn->target_note = chn->note - ((inst & 0xf) << 8);
							track_status->slide_speed = (inst & 0xf0) << 1;

							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									chn->ops[i].target_note = chn->ops[i].note - ((inst & 0xf) << 8);
									track_status->ops_status[i].slide_speed = (inst & 0xf0) << 1;
								}
							}
							
							break;
						}
						
						default: 
						{
							chn->ops[ops_index - 1].target_note = chn->ops[ops_index - 1].note - ((inst & 0xf) << 8);
							track_status->ops_status[ops_index - 1].slide_speed = (inst & 0xf0) << 1;
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

				case MUS_FX_SET_VOLUME_FROM_PROGRAM:
				{
					if(from_program)
					{
						switch(ops_index)
						{
							case 0:
							case 0xFF:
							{
								chn->program_volume = my_min(MAX_VOLUME, inst & 0xff);
								update_volumes(mus, track_status, chn, cydchn, track_status->volume);
								
								if(ops_index == 0xFF)
								{
									for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
									{
										chn->ops[i].program_volume = my_min(MAX_VOLUME, inst & 0xff);
										update_fm_op_volume(mus, track_status, chn, cydchn, track_status->ops_status[i].volume, i);
									}
								}
								
								break;
							}
							
							default:
							{
								chn->ops[ops_index - 1].program_volume = my_min(MAX_VOLUME, inst & 0xff);
								update_fm_op_volume(mus, track_status, chn, cydchn, track_status->ops_status[ops_index - 1].volume, ops_index - 1);
								
								break;
							}
						}
					}
				}
				break;
				
				case MUS_FX_SET_ABSOLUTE_VOLUME:
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							track_status->volume = (inst & 0xff);
							
							Uint32 temp_flags = chn->instrument->flags;
							chn->instrument->flags &= ~(MUS_INST_RELATIVE_VOLUME); //dirty but works
							
							update_volumes(mus, track_status, chn, cydchn, track_status->volume);
							
							chn->instrument->flags = temp_flags;
							
							if(ops_index == 0xFF)
							{
								for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
								{
									track_status->ops_status[i].volume = (inst & 0xff);
									
									Uint32 temp_op_flags = chn->instrument->ops[i].flags;
									chn->instrument->ops[i].flags &= ~(MUS_FM_OP_RELATIVE_VOLUME);
									
									update_fm_op_volume(mus, track_status, chn, cydchn, track_status->ops_status[i].volume, i);
									
									chn->instrument->ops[i].flags = temp_op_flags;
								}
							}
							
							break;
						}
						
						default:
						{
							track_status->ops_status[ops_index - 1].volume = (inst & 0xff);
							
							Uint32 temp_op_flags = chn->instrument->ops[ops_index - 1].flags;
							chn->instrument->ops[ops_index - 1].flags &= ~(MUS_FM_OP_RELATIVE_VOLUME);
							
							update_fm_op_volume(mus, track_status, chn, cydchn, track_status->ops_status[ops_index - 1].volume, ops_index - 1);
							
							chn->instrument->ops[ops_index - 1].flags = temp_op_flags;
							
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
								chn->flags &= ~(MUS_INST_USE_LOCAL_SAMPLES);
								cydchn->flags |= CYD_CHN_ENABLE_WAVE;
								
								mus_set_wavetable_frequency(mus, chan, chn->note);
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
				
				case MUS_FX_SET_LOCAL_SAMPLE:
				{
					switch(ops_index)
					{
						case 0:
						case 0xFF:
						{
							if(chn->instrument)
							{
								if(chn->instrument->local_samples)
								{
									if(chn->instrument->num_local_samples > (inst & 0xff))
									{
										cydchn->wave_entry = chn->instrument->local_samples[inst & 0xff];
										chn->flags |= MUS_INST_USE_LOCAL_SAMPLES;
										cydchn->flags |= CYD_CHN_ENABLE_WAVE;
										
										mus_set_wavetable_frequency(mus, chan, chn->note);
									}
								}
							}
							
							break;
						}
						
						default: break;
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
