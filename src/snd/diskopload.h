#pragma once

#ifndef DISKOPLOAD_H
#define DISKOPLOAD_H

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

#include "cydtypes.h"
#include "cydentry.h"
#include <stdbool.h>
#include "music.h"

int mus_load_instrument(const char *path, MusInstrument *inst, CydWavetableEntry *wavetable_entries);
int mus_load_instrument_RW(Uint8 version, RWops *ctx, MusInstrument *inst, CydWavetableEntry *wavetable_entries);
int mus_load_fx(const char *path, CydFxSerialized *fx);
int mus_load_song_RW(RWops *ctx, MusSong *song, CydWavetableEntry *wavetable_entries);
int mus_load_song(const char *path, MusSong *song, CydWavetableEntry *wavetable_entries);
int mus_load_instrument_file(Uint8 version, FILE *f, MusInstrument *inst, CydWavetableEntry *wavetable_entries);
int mus_load_song_file(FILE *f, MusSong *song, CydWavetableEntry *wavetable_entries);
int mus_load_wavepatch(FILE *f, WgSettings *settings); //wasn't there

#endif