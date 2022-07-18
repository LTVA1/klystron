#pragma once

#include "SDL.h"
#include <stdbool.h>

typedef struct
{
	Uint8 volume;
	
	Uint32 envelope, env_speed;
	Uint8 envelope_state;
	Uint8 a, d, s, r; // s 0-32, adr 0-63
} CydAdsr;

typedef struct
{
	Uint8 volume;
	
	Sint64 envelope;
	Uint32 env_speed;
	Uint8 envelope_state;
	Uint8 a, d, s, r; // s 0-32, adr 0-63
	
	Uint8 passes; //for SSG-EG
	bool direction; //envelope direction for SSG-EG; 0 means normal, 1 means inverted
} CydFmOpAdsr;
