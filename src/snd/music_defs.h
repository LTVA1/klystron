#pragma once

#define MUS_PROG_LEN 255
#define CYD_FM_NUM_OPS 4

#define MUS_MAX_ENVELOPE_POINTS 16 /* how many points in volume/panning envelope you can have */
#define MUS_MAX_ENVELOPE_POINT_X 10000 /* with 1/100th of a second resolution this gives max length of 100 seconds */

enum
{
	MUS_ENV_SUSTAIN = 1,
	MUS_ENV_LOOP = 2,
};