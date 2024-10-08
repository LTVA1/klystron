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


#include "slider.h"
#include "gui/mouse.h"
#include "gui/bevel.h"
#include "dialog.h"
#include "view.h"

#ifndef STANDALONE_COMPILE

#include "../../../src/mused.h"

#endif

int quant(int v, int g)
{
	return v - v % (g);
}


static void modify_position(void *delta, void *_param, void *unused)
{
	SliderParam *param = _param;
	*param->position = quant(*param->position + CASTPTR(int, delta), param->granularity);
	if (*param->position < param->first) *param->position = param->first;
	if (*param->position > param->last - param->margin) *param->position = param->last - param->margin;
}


static void drag_motion(int x, int y, void *_param)
{
	SliderParam *param = _param;
	if ((param->visible_first <= param->first && param->visible_last >= param->last) || param->drag_area_size == 0)
		return;
	int delta = (param->orientation == SLIDER_HORIZONTAL ? x : y) - param->drag_begin_coordinate;
	*param->position = quant(param->drag_begin_position + delta * (param->last - param->first) / param->drag_area_size, param->granularity);
	if (*param->position > param->last - param->margin) *param->position = param->last - param->margin;
	if (*param->position < param->first) *param->position = param->first;
}


static void drag_begin(void *event, void *_param, void *area)
{
	set_motion_target(drag_motion, _param);
	SliderParam *param = _param;
	param->drag_begin_coordinate = param->orientation == SLIDER_HORIZONTAL ? ((SDL_Event*)event)->button.x : ((SDL_Event*)event)->button.y;
	param->drag_begin_position = *param->position;
	if (param->drag_begin_position > param->last - param->margin) param->drag_begin_position = param->last - param->margin;
	if (param->drag_begin_position < param->first) param->drag_begin_position = param->first;
	param->drag_area_size = ((param->orientation == SLIDER_HORIZONTAL) ? ((SDL_Rect*)area)->w : ((SDL_Rect*)area)->h);
}


void slider(GfxDomain *dest_surface, const SDL_Rect *_area, const SDL_Event *event, void *_param)
{
#ifndef STANDALONE_COMPILE
	if((_param == &mused.program_slider_param) && (mused.show_point_envelope_editor || mused.show_four_op_menu)) return;
	
	if((_param == &mused.point_env_slider_param) && !(mused.show_point_envelope_editor)) return;
	if((_param == &mused.point_env_slider_param) && mused.show_four_op_menu) return;
	
	if(((_param == &mused.four_op_slider_param) && mused.show_four_op_menu) || _param != &mused.four_op_slider_param)
	{
#endif
		SliderParam *param = _param;
		
		SDL_Rect my_area = { _area->x, _area->y, _area->w, _area->h };
		copy_rect(&my_area, _area);
		
#ifndef STANDALONE_COMPILE
		if((_param == &mused.program_slider_param || _param == &mused.point_env_slider_param) && mused.show_fx_lfo_settings)
		{
			my_area.y -= 60;
			my_area.h += 60;
		}
		
		if((_param == &mused.program_slider_param || _param == &mused.point_env_slider_param) && mused.show_timer_lfo_settings)
		{
			my_area.y -= 130;
			my_area.h += 130;
		}
#endif

		int button_size = (param->orientation == SLIDER_HORIZONTAL) ? my_area.h : my_area.w;
		int area_size = ((param->orientation == SLIDER_HORIZONTAL) ? my_area.w : my_area.h) - button_size * 2;
		int area_start = ((param->orientation == SLIDER_HORIZONTAL) ? my_area.x : my_area.y) + button_size;
		int bar_size = area_size;
		int bar_top = area_start;
		int sbsize = my_min(my_area.w, my_area.h);
		bool shrunk = false;
		
		if (param->last != param->first)
		{
			bar_top = (area_size) * param->visible_first / (param->last - param->first) + area_start;
			int bar_bottom = (area_size ) * param->visible_last / (param->last - param->first) + area_start;
			bar_size = my_min(area_size, bar_bottom - bar_top);
			
			if (bar_size < sbsize)
			{
				bar_top = (area_size - sbsize) * param->visible_first / (param->last - param->first) + area_start;
				bar_bottom = bar_top + sbsize;
				bar_size = sbsize;
				
				shrunk = true;
			}
		}
		
		SDL_Rect dragarea = { my_area.x, my_area.y, my_area.w, my_area.h };
		
		if (param->orientation == SLIDER_HORIZONTAL)
		{
			dragarea.x += button_size;
			dragarea.w -= button_size * 2;
		}
		else
		{
			dragarea.y += button_size;
			dragarea.h -= button_size * 2;
		}
		
		bevel(dest_surface, &dragarea, param->gfx, BEV_SLIDER_BG);
		
		{
			SDL_Rect area = { my_area.x, my_area.y, my_area.w, my_area.h };
			
			if (param->orientation == SLIDER_HORIZONTAL)
			{
				area.w = bar_top - dragarea.x;
				area.x = dragarea.x;
			}
			else
			{
				area.h = bar_top - dragarea.y;
				area.y = dragarea.y;
			}
			
			check_event(event, &area, modify_position, MAKEPTR(-(param->visible_last - param->visible_first)), param, 0);
		}
		
		{
			SDL_Rect area = { my_area.x, my_area.y, my_area.w, my_area.h };
			
			if (param->orientation == SLIDER_HORIZONTAL)
			{
				area.x += bar_top - my_area.x;
				area.w = bar_size;
			}
			else
			{
				area.y += bar_top - my_area.y;
				area.h = bar_size;
			}
			
			SDL_Rect motion_area;
			copy_rect(&motion_area, &dragarea);
			
			if (param->orientation == SLIDER_HORIZONTAL)
			{
				motion_area.w -= bar_size;
			}
			else
			{
				motion_area.h -= bar_size;
			}
			
			int pressed = check_event(event, &area, drag_begin, MAKEPTR(event), param, MAKEPTR(shrunk ? &motion_area : &dragarea));
			pressed |= check_drag_event(event, &area, drag_motion, MAKEPTR(param));
			button(dest_surface, &area, param->gfx, pressed ? BEV_SLIDER_HANDLE_ACTIVE : BEV_SLIDER_HANDLE, (param->orientation == SLIDER_HORIZONTAL) ? DECAL_GRAB_HORIZ : DECAL_GRAB_VERT);

#ifndef STANDALONE_COMPILE
			if((_param == &mused.pattern_slider_param) && mused.selection.drag_selection && pressed) //fix selection emerging when you scroll patterns using right vertical slider
			{
				mused.selection.drag_selection_program = false;
				mused.jump_in_program = true;
				mused.selection.start = mused.selection.end = -1;
			}
#endif
		}
		
		{
			SDL_Rect area = { my_area.x, my_area.y, my_area.w, my_area.h };
			
			if (param->orientation == SLIDER_HORIZONTAL)
			{
				area.x = bar_top + bar_size;
				area.w = dragarea.x + dragarea.w - bar_size - bar_top;
			}
			else
			{
				area.y = bar_top + bar_size;
				area.h = dragarea.y + dragarea.h - bar_size - bar_top;
			}
			
			check_event(event, &area, modify_position, MAKEPTR(param->visible_last - param->visible_first), param, 0);
		}
		
		{
			SDL_Rect area = { my_area.x, my_area.y, sbsize, sbsize };
			
			button_event(dest_surface, event, &area, param->gfx, BEV_BUTTON, BEV_BUTTON_ACTIVE, param->orientation == SLIDER_HORIZONTAL ? DECAL_LEFTARROW : DECAL_UPARROW, modify_position, MAKEPTR(-param->granularity), param, 0);
			
			if (param->orientation == SLIDER_HORIZONTAL)
			{
				area.x += area_size + button_size;
			}
			else
			{
				area.y += area_size + button_size;
			}
			
			button_event(dest_surface, event, &area, param->gfx, BEV_BUTTON, BEV_BUTTON_ACTIVE, param->orientation == SLIDER_HORIZONTAL ? DECAL_RIGHTARROW : DECAL_DOWNARROW, modify_position, MAKEPTR(param->granularity), param, 0);
		}
#ifndef STANDALONE_COMPILE
	}
#endif
}


void slider_set_params(SliderParam *param, int first, int last, int first_visible, int last_visible, int *position, int granularity, int orientation, GfxSurface *gfx)
{
	param->first = first;
	param->last = last;
	param->visible_first = first_visible;
	param->visible_last = last_visible;
	param->margin = (last_visible - first_visible);
	
	param->orientation = orientation;
	
	param->position = position;
	param->granularity = granularity;
	
	param->gfx = gfx;
}

void check_mouse_wheel_event(const SDL_Event *event, const SDL_Rect *rect, SliderParam *slider)
{
	switch (event->type)
	{
		case SDL_MOUSEWHEEL:
		{
			int p = (slider->visible_last - slider->visible_first) / 2;
			
			if (event->wheel.y > 0)
			{
				*slider->position -= p;
			}
			
			else
			{
				*slider->position += p;
			}
			
			*slider->position = my_max(my_min(*slider->position, slider->last - (slider->visible_last - slider->visible_first)), slider->first);
		}
		break;
	}
}


void slider_move_position(int *cursor, int *scroll, SliderParam *param, int d)
{
	if (*cursor + d < param->last)
		*cursor += d;
	else
		*cursor = param->last;
	
	if (*cursor < param->first) *cursor = param->first;
	
	if (param->visible_first > *cursor)
		*scroll = *cursor;
		
	if (param->visible_last < *cursor)
		*scroll = *cursor - (param->visible_last - param->visible_first);
}