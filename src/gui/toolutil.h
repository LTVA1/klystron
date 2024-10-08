#ifndef TOOLUTIL_H
#define TOOLUTIL_H

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

#include <stdio.h>
#include "gfx/gfx.h"
#include "gfx/font.h"

FILE *open_dialog(const char *mode, const char *title, const char *filter, GfxDomain *domain, GfxSurface *gfx, const Font *largefont, const Font *smallfont, const char *deffilename);
char * open_dialog_fn(const char *mode, const char *title, const char *filter, GfxDomain *domain, GfxSurface *gfx, const Font *largefont, const Font *smallfont, const char *deffilename, char *filename, int filename_size);
int confirm(GfxDomain *domain, GfxSurface *gfx, const Font *font, const char *msg);
int confirm_ync(GfxDomain *domain, GfxSurface *gfx, const Font *font, const char *msg);
char * expand_tilde(const char * path);

#endif
