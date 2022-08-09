#pragma once

#include "cydtypes.h"
#include "cyd.h"

Sint32 cyd_osc(Uint32 flags, Uint32 accumulator, Uint32 pw, Uint32 random, Uint32 lfsr_acc, Uint8 mixmode, Uint8 sine_acc_shift, CydEngine* cyd); //Sint32 cyd_osc(Uint32 flags, Uint32 accumulator, Uint32 pw, Uint32 random, Uint32 lfsr_acc);
