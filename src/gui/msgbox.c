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

#include "msgbox.h"
#include "gui/bevel.h"
#include "dialog.h"
#include "gfx/gfx.h"
#include "view.h"
#include "gui/mouse.h"

static int draw_box(GfxDomain *dest, GfxSurface *gfx, const Font *font, const SDL_Event *event, const char *msg, int buttons, int *selected)
{
	int w = 0, max_w = 200, h = font->h;
	
	for (const char *c = msg; *c; ++c)
	{
		w += font->w;
		max_w = my_max(max_w, w + 16);
		if (*c == '\n')
		{
			w = 0;
			h += font->h;
		}
	}
	
	SDL_Rect area = { dest->screen_w / 2 - max_w / 2, dest->screen_h / 2 - h / 2 - 8, max_w, h + 16 + 16 + 4 };
	
	bevel(dest, &area, gfx, BEV_MENU);
	SDL_Rect content, pos;
	copy_rect(&content, &area);
	adjust_rect(&content, 8);
	copy_rect(&pos, &content);
	
	font_write(font, dest, &pos, msg);
	update_rect(&content, &pos);
	
	int b = 0;
	for (int i = 0; i < MB_BUTTON_TYPES; ++i)
		if (buttons & (1 << i)) ++b;
		
	*selected = (*selected + b) % b;
	
	pos.w = 50;
	pos.h = 14;
	pos.x = content.x + content.w / 2 - b * (pos.w + ELEMENT_MARGIN) / 2 + ELEMENT_MARGIN / 2;
	pos.y -= 8 + 4;
	
	int r = 0;
	static const char *label[] = { "YES", "NO", "CANCEL", "OK" };
	int idx = 0;
	
	for (int i = 0; i < MB_BUTTON_TYPES; ++i)
	{
		if (buttons & (1 << i))
		{
			int p = button_text_event(dest, event, &pos, gfx, font, BEV_BUTTON, BEV_BUTTON_ACTIVE, label[i], NULL, 0, 0, 0);
			
			if (idx == *selected)
			{	
				if (event->type == SDL_KEYDOWN && (event->key.keysym.sym == SDLK_SPACE || event->key.keysym.sym == SDLK_RETURN))
					p = 1;
			
				bevel(dest, &pos, gfx, BEV_CURSOR);
			}
			
			update_rect(&content, &pos);
			if (p & 1) r = (1 << i);
			++idx;
		}
	}
	
	return r;
}


int msgbox(GfxDomain *domain, GfxSurface *gfx, const Font *font, const char *msg, int buttons)
{
	set_repeat_timer(NULL);
	
	int selected = 0;
	
	SDL_StopTextInput();
	
	while (1)
	{
		SDL_Event e = { 0 };
		int got_event = 0;
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
				case SDL_WINDOWEVENT:
				{
					switch (e.window.event) 
					{
						case SDL_WINDOWEVENT_RESIZED:
						{
							debug("SDL_WINDOWEVENT_RESIZED %dx%d", e.window.data1, e.window.data2);

							domain->screen_w = my_max(320, e.window.data1 / domain->scale);
							domain->screen_h = my_max(240, e.window.data2 / domain->scale);

							gfx_domain_update(domain, false);
						}
						break;
					}
					break;
				}
				
				case SDL_KEYDOWN:
				{
					switch (e.key.keysym.sym)
					{
						case SDLK_ESCAPE:
						
						if (buttons & MB_CANCEL)
							return MB_CANCEL;
						if (buttons & MB_NO)
							return MB_NO;
						return MB_OK;
						
						break;
						
						case SDLK_SPACE:
						
							
						
						break;
						
						case SDLK_KP_ENTER:
						case SDLK_RETURN:
							if((buttons & MB_NO) && (buttons & MB_YES))
							{
								return selected == 0 ? MB_YES : MB_NO;
							}
							
							else
							{
								return MB_OK;
							}
						break;
						
						case SDLK_LEFT:
						--selected;
						break;
						
						case SDLK_RIGHT:
						++selected;
						break;
						
						default: 
						break;
					}
				}
				break;
			
				case SDL_USEREVENT:
					e.type = SDL_MOUSEBUTTONDOWN;
				break;
				
				case SDL_MOUSEMOTION:
					gfx_convert_mouse_coordinates(domain, &e.motion.x, &e.motion.y);
					gfx_convert_mouse_coordinates(domain, &e.motion.xrel, &e.motion.yrel);
				break;
				
				case SDL_MOUSEBUTTONDOWN:
					gfx_convert_mouse_coordinates(domain, &e.button.x, &e.button.y);
				break;
				
				case SDL_MOUSEBUTTONUP:
				{
					if (e.button.button == SDL_BUTTON_LEFT)
						mouse_released(&e);
				}
				break;
			}
			
			if (e.type != SDL_MOUSEMOTION || (e.motion.state)) ++got_event;
			
			// Process mouse click events immediately, and batch mouse motion events
			// (process the last one) to fix lag with high poll rate mice on Linux.
			//fix from here https://github.com/kometbomb/klystrack/pull/300
			if (should_process_mouse(&e))
				break;
		}
		
		if (got_event || gfx_domain_is_next_frame(domain))
		{
			int r = draw_box(domain, gfx, font, &e, msg, buttons, &selected);
			gfx_domain_flip(domain);
			set_repeat_timer(NULL);
			
			if (r) 
			{
				return r;
			}
		}
		
		else
		{
			SDL_Delay(5);
		}
	}
}
