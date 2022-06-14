#pragma once

#include "SDL.h"

typedef struct
{
	Uint8 volume;
	
	Uint32 envelope, env_speed;
	Uint8 envelope_state;
	Uint8 a, d, s, r; // s 0-32, adr 0-63
} CydAdsr;
