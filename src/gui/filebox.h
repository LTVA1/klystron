#ifndef FILEBOX_H
#define FILEBOX_H

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

enum { FB_CANCEL = 1, FB_OK = 2 };
enum { FB_SAVE, FB_OPEN };

#include <stdlib.h>
#include "SDL.h"
#include "gfx/gfx.h"
#include "gfx/font.h"

int filebox(const char *title, int mode, char *buffer, size_t buffer_size, const char *extension, GfxDomain *domain, GfxSurface *gfx, const Font *smallfont, const Font *largefont);
void filebox_add_favorite(const char *path);
void filebox_remove_favorite(const char *path);
void filebox_quit(const char *path);
void filebox_init(const char *path);

#endif
