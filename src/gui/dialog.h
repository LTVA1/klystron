#pragma once

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

#include "SDL.h"
#include "gui/bevdefs.h"
#include "gfx/font.h"

int checkbox(GfxDomain *dest, const SDL_Event *event, const SDL_Rect *area, GfxSurface *gfx, const Font * font, int offset, int offset_pressed, int decal, const char* label, Uint32 *flags, Uint32 mask);
int spinner(GfxDomain *dest, const SDL_Event *event, const SDL_Rect *area, GfxSurface *gfx, int param);
int button_event(GfxDomain *dest, const SDL_Event *event, const SDL_Rect *area, GfxSurface *gfx, int offset, int offset_pressed, int decal, void (*action)(void*,void*,void*), void *param1, void *param2, void *param3);
int button_text_event(GfxDomain *dest, const SDL_Event *event, const SDL_Rect *area, GfxSurface *gfx, const Font *font, int offset, int offset_pressed, const char *label, void (*action)(void*,void*,void*), void *param1, void *param2, void *param3);
int generic_edit_text(SDL_Event *e, char *edit_buffer, size_t edit_buffer_size, int *editpos);
