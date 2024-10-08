#ifndef SLIDER_H
#define SLIDER_H

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
#include "gfx/gfx.h"

typedef struct
{
	// Total elements (bounds) of target
	int first, last;
	// First and last visible elements
	int visible_first, visible_last;
	// Ye int that shall be modified by the slider
	int *position;
	// Snap to this resolution
	int granularity;
	// Fixes issues with combined curson position + scroll position
	// I don't like this either -- but it works (in this situation)
	int margin;
	// Direction
	int orientation;
	
	/* internal */
	int drag_begin_coordinate, drag_begin_position, drag_area_size;	
	
	GfxSurface *gfx;
} SliderParam;

enum
{
	SLIDER_VERTICAL, SLIDER_HORIZONTAL
};

int quant(int v, int g);
void slider(GfxDomain * domain, const SDL_Rect *area, const SDL_Event *event, void *param);
void slider_set_params(SliderParam *param, int first, int last, int first_visible, int last_visible, int *position, int granularity, int orientation, GfxSurface *gfx);

void check_mouse_wheel_event(const SDL_Event *event, const SDL_Rect *area, SliderParam *slider);

void slider_move_position(int *cursor, int *scroll, SliderParam *param, int d);

#endif
