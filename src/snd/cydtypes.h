#ifndef CYDTYPES_H
#define CYDTYPES_H

/* SDL-equivalent types */

#include "SDL.h"

#ifndef USENATIVEAPIS

typedef SDL_mutex * CydMutex;

#else

# ifdef WIN32

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x500
#endif

#include <windows.h>

/*typedef BYTE Uint8;
typedef CHAR Sint8;
typedef WORD Uint16;
typedef SHORT Sint16;
typedef DWORD Uint32;
typedef INT Sint32;
typedef unsigned long long Uint64;*/
typedef CRITICAL_SECTION CydMutex;

# else

#  error USENATIVEAPIS: Platform not supported

# endif

#endif

#ifdef LOWRESWAVETABLE
typedef Uint64 CydWaveAcc;
typedef Sint64 CydWaveAccSigned;
#else
typedef Uint64 CydWaveAcc;
typedef Sint64 CydWaveAccSigned;
#endif

#endif
