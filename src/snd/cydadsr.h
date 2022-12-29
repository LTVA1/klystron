#pragma once

#include "SDL.h"
#include <stdbool.h>

#include "music_defs.h"

typedef struct
{
	Uint32 x;
	Uint32 y;
} CydEnvPoint;

typedef struct
{
	Uint8 volume;
	
	Uint32 envelope, env_speed;
	Uint8 envelope_state;
	Uint8 a, d, s, r; // s 0-32, adr 0-63
	
	//=====================
	
	bool use_volume_envelope;
	bool use_panning_envelope;
	
	bool advance_volume_envelope;
	
	Uint8 current_vol_env_point;
	Uint8 next_vol_env_point;
	
	Uint32 vol_env_fadeout;
	
	Sint32 curr_vol_fadeout_value;
	
	Uint32 volume_envelope_output;
	
	Uint8 num_vol_points;
	Uint8 vol_env_loop_start;
	Uint8 vol_env_loop_end;
	Uint8 vol_env_sustain;
	Uint8 vol_env_flags; //1 - sustain, 2 - loop
	
	CydEnvPoint volume_envelope[MUS_MAX_ENVELOPE_POINTS];
	
	bool advance_panning_envelope;
	
	Uint8 current_pan_env_point;
	Uint8 next_pan_env_point;
	
	Uint32 pan_env_fadeout;
	
	Sint32 curr_pan_fadeout_value;
	
	Uint32 pan_env_speed;
	
	Uint8 pan_counter; //so we set panning every 16th step to be easier on CPU
	
	Uint32 panning_envelope_output;
	
	Uint8 num_pan_points;
	Uint8 pan_env_loop_start;
	Uint8 pan_env_loop_end;
	Uint8 pan_env_sustain;
	Uint8 pan_env_flags; //1 - sustain, 2 - loop
	
	Uint32 pan_envelope;
	
	CydEnvPoint panning_envelope[MUS_MAX_ENVELOPE_POINTS];
	
} CydAdsr;

typedef struct
{
	Uint8 volume;
	
	Sint64 envelope;
	Uint32 env_speed;
	Uint8 envelope_state;
	Uint8 a, d, s, r; // s 0-32, adr 0-63
	
	Uint8 sr; //sustain rate, 0 means horizontal sustain, 63 immediate fall down
	
	Uint32 passes; //for SSG-EG
	bool direction; //envelope direction for SSG-EG; 0 means normal, 1 means inverted
} CydFmOpAdsr;
