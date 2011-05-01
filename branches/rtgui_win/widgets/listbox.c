/*
 * File      : listbox.c
 * This file is part of RTGUI in RT-Thread RTOS
 * COPYRIGHT (C) 2010, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2010-01-06     Bernard      first version
 */
#include <panel.h>
#include <rtgui/rtgui_theme.h>
#include <rtgui/widgets/listbox.h>

#define RTGUI_WIDGET_DEFAULT_MARGIN		3

static rt_bool_t rtgui_listbox_unfocus(PVOID wdt, rtgui_event_t* event);
static rt_bool_t rtgui_listbox_sbar_handle(PVOID wdt, rtgui_event_t* event);
static rt_uint32_t rtgui_listbox_get_item_count(rtgui_listbox_t* box);
static void rtgui_listbox_add_item(rtgui_listbox_t* box,rtgui_listbox_item_t* item);

static void _rtgui_listbox_constructor(rtgui_listbox_t *box)
{
	/* set default widget rect and set event handler */
	rtgui_widget_set_event_handler(box,rtgui_listbox_event_handler);
	rtgui_widget_set_onunfocus(box, rtgui_listbox_unfocus);
	RTGUI_WIDGET_FLAG(box) |= RTGUI_WIDGET_FLAG_FOCUSABLE;
	box->frist_aloc = 0;	
	box->now_aloc = 0;
	box->old_aloc = 0;		
	box->item_count = 0;
	box->item_size	= SELECTED_HEIGHT;
	box->item_per_page = 0;	
	box->ispopup = RT_FALSE;
	box->widgetlnk = RT_NULL;		
	
	RTGUI_WIDGET_BACKGROUND(box) = white;
	RTGUI_WIDGET_TEXTALIGN(box) = RTGUI_ALIGN_LEFT|RTGUI_ALIGN_CENTER_VERTICAL;

	box->items = RT_NULL;
	box->sbar = RT_NULL;
#ifdef LB_USING_MULTI_SEL
	box->multi_sel = 0;
#endif
	box->on_item = RT_NULL;
	box->updown = RT_NULL;

	box->get_count = rtgui_listbox_get_item_count;
	box->add_item = rtgui_listbox_add_item;
}
static void _rtgui_listbox_destructor(rtgui_listbox_t *box)
{

}

rtgui_type_t *rtgui_listbox_type_get(void)
{
	static rtgui_type_t *listbox_type = RT_NULL;

	if(!listbox_type)
	{
		listbox_type = rtgui_type_create("listbox", RTGUI_CONTAINER_TYPE,
			sizeof(rtgui_listbox_t),
			RTGUI_CONSTRUCTOR(_rtgui_listbox_constructor), 
			RTGUI_DESTRUCTOR(_rtgui_listbox_destructor));
	}

	return listbox_type;
}

rtgui_listbox_t* rtgui_listbox_create(PVOID parent, int left,int top,int w,int h,rt_uint32_t style)
{
	rtgui_listbox_t* box = RT_NULL;

	RT_ASSERT(parent != RT_NULL);

	box = rtgui_widget_create(RTGUI_LISTBOX_TYPE);
	if(box != RT_NULL)
	{
		rtgui_rect_t rect;    
		rtgui_widget_get_rect(parent,&rect);
		rtgui_widget_rect_to_device(parent, &rect);
		rect.x1 += left;
		rect.y1 += top;
		rect.x2 = rect.x1+w;
		rect.y2 = rect.y1+h;
		rtgui_widget_set_rect(box,&rect);
		rtgui_container_add_child(parent, box);

		rtgui_widget_set_style(box,style);

		if(box->sbar == RT_NULL)
		{/* create scrollbar */
			rt_uint32_t sLeft,sTop,sWidth=RTGUI_DEFAULT_SB_WIDTH,sLen;
			sLeft = rtgui_rect_width(rect)-RTGUI_WIDGET_BORDER(box)-sWidth;
			sTop = RTGUI_WIDGET_BORDER(box);
			sLen = rtgui_rect_height(rect)-RTGUI_WIDGET_BORDER(box)*2;
	
			box->sbar = rtgui_scrollbar_create(box,sLeft,sTop,sWidth,sLen,RTGUI_VERTICAL);
			
			if(box->sbar != RT_NULL)
			{
				box->sbar->widgetlnk = (PVOID)box;
				box->sbar->on_scroll = rtgui_listbox_sbar_handle;
				RTGUI_WIDGET_HIDE(box->sbar);/* default hide scrollbar */
			}
		}
	}

	return box;
}

void rtgui_listbox_set_items(rtgui_listbox_t* box, rtgui_listbox_item_t* items, rt_uint32_t count)
{
	rtgui_rect_t rect;
	rt_uint32_t i,h;;

	RT_ASSERT(box != RT_NULL);

	if(box->items != RT_NULL)
	{
		rt_free(box->items);
		box->items = RT_NULL;
	}
	/* support add/del/append, dynamic memory */
	box->items = (rtgui_listbox_item_t*) rt_malloc(sizeof(rtgui_listbox_item_t)*count);
	if(box->items == RT_NULL) return;

	for(i=0;i<count;i++)
	{
		box->items[i].name = rt_strdup(items[i].name);
		box->items[i].image = items[i].image;
#ifdef LB_USING_MULTI_SEL
		box->items[i].ext_flag = 0;
#endif	
	}

	box->item_count = count;
	box->now_aloc = 0;
	box->old_aloc = 0;

	rtgui_widget_get_rect(box, &rect);

	box->item_per_page = rtgui_rect_height(rect) / (box->item_size+2);
	
	if(box->ispopup)/* created by popup */
	{
		if(box->item_count < 5)
			box->item_per_page = count;	
		else
			box->item_per_page = 5;

		h = 2+(box->item_size+2)*box->item_per_page;
		rect.y2 = rect.y1+h;
		rtgui_widget_rect_to_device(box,&rect);
		rtgui_widget_set_rect(box,&rect);/* update listbox extent */

		if(box->sbar != RT_NULL) /* update scrollbar extent */
		{	
			rtgui_widget_get_rect(box->sbar,&rect);
			rect.y2 = rect.y1+h-RTGUI_WIDGET_BORDER(box)*2;
			rtgui_widget_rect_to_device(box->sbar,&rect);
			rtgui_widget_set_rect(box->sbar,&rect);		
		}	
	}
	
	if(box->sbar != RT_NULL)/* update scrollbar value */
	{	
		if(box->item_count > box->item_per_page)
		{
			RTGUI_WIDGET_UNHIDE(box->sbar);
			rtgui_scrollbar_set_line_step(box->sbar, 1);
			rtgui_scrollbar_set_page_step(box->sbar, box->item_per_page);
			rtgui_scrollbar_set_range(box->sbar, box->item_count);
		}
		else
		{
			RTGUI_WIDGET_HIDE(box->sbar);
		}
		rtgui_widget_update_clip(box);		
	}
	
}

void rtgui_listbox_destroy(rtgui_listbox_t* box)
{
    /* destroy box */
	rtgui_widget_destroy(box);
}

static const unsigned char lb_ext_flag[]=
{0x00,0x10,0x00,0x30,0x00,0x70,0x80,0xE0,0xC1,0xC0,0xE3,0x80,0x77,0x00,0x3E,0x00,0x1C,0x00,0x08,0x00};

/* draw listbox all item */
void rtgui_listbox_ondraw(rtgui_listbox_t* box)
{
	rtgui_rect_t rect, item_rect, image_rect;
	rt_uint16_t frist, i;
	const rtgui_listbox_item_t* item;
	rtgui_dc_t* dc;
	
	RT_ASSERT(box != RT_NULL);

	/* begin drawing */
	dc = rtgui_dc_begin_drawing(box);
	if(dc == RT_NULL)return;

	rtgui_widget_get_rect(box, &rect);

	/* draw listbox border */
	rtgui_dc_draw_border(dc, &rect,RTGUI_WIDGET_BORDER_STYLE(box));
	rtgui_rect_inflate(&rect,-RTGUI_WIDGET_BORDER(box));
	RTGUI_DC_BC(dc) = white;
	rtgui_dc_fill_rect(dc, &rect); 
	rtgui_rect_inflate(&rect,RTGUI_WIDGET_BORDER(box));

	if(box->items==RT_NULL)return;/* not exist items. */

	if(box->sbar && !RTGUI_WIDGET_IS_HIDE(box->sbar))
	{	
		rect.x2 -= rtgui_rect_width(box->sbar->parent.extent);
	}

	/* get item base rect */
	item_rect = rect;
	item_rect.x1 += RTGUI_WIDGET_BORDER(box); 
	item_rect.x2 -= RTGUI_WIDGET_BORDER(box);
	item_rect.y1 += RTGUI_WIDGET_BORDER(box);
	item_rect.y2 = item_rect.y1 + (2+box->item_size);

	/* get frist aloc */
	frist = box->frist_aloc;
	for(i = 0; i < box->item_per_page; i ++)
	{
		if(frist + i >= box->item_count) break;

		item = &(box->items[frist + i]);

		if(frist + i == box->now_aloc)
		{//draw current item
			if(RTGUI_WIDGET_IS_FOCUSED(box))
			{	
				RTGUI_DC_BC(dc) = selected_color;
				RTGUI_DC_FC(dc) = white;
				rtgui_dc_fill_rect(dc, &item_rect);
				rtgui_dc_draw_focus_rect(dc, &item_rect);
			}
			else
			{
				RTGUI_DC_BC(dc) = dark_grey;
				RTGUI_DC_FC(dc) = black;
				rtgui_dc_fill_rect(dc, &item_rect);
			}
		}
		item_rect.x1 += RTGUI_WIDGET_DEFAULT_MARGIN;

		if(item->image != RT_NULL)
		{
			/* get image base rect */
			image_rect.x1 = 0; 
			image_rect.y1 = 0;
			image_rect.x2 = item->image->w; 
			image_rect.y2 = item->image->h;
			rtgui_rect_moveto_align(&item_rect, &image_rect, RTGUI_ALIGN_CENTER_VERTICAL);
			rtgui_image_paste(item->image, dc, &image_rect, white);
			item_rect.x1 += item->image->w + 2;
		}
        /* draw text */
		if(frist + i == box->now_aloc && RTGUI_WIDGET_IS_FOCUSED(box))
		{
			RTGUI_DC_FC(dc) = white;
			rtgui_dc_draw_text(dc, item->name, &item_rect);
		}
		else
		{
			RTGUI_DC_FC(dc) = black;
			rtgui_dc_draw_text(dc, item->name, &item_rect);
		}
#ifdef LB_USING_MULTI_SEL
		if(box->multi_sel && item->ext_flag)
		{
			rtgui_rect_t tmprect = rect;
			tmprect.x2 -= RTGUI_WIDGET_DEFAULT_MARGIN;
			tmprect.x2 -= RTGUI_WIDGET_BORDER(box);
			tmprect.x1 = tmprect.x2 - 12;
			tmprect.y1 = item_rect.y1;
			tmprect.y2 = item_rect.y2;
			rtgui_dc_draw_word(dc, tmprect.x1, tmprect.y1+6, 10, lb_ext_flag);
		}
#endif
        if(item->image != RT_NULL)
            item_rect.x1 -= (item->image->w + 2);
		item_rect.x1 -= RTGUI_WIDGET_DEFAULT_MARGIN;

        /* move to next item position */
		item_rect.y1 += (box->item_size + 2);
		item_rect.y2 += (box->item_size + 2);
	}

	if(box->sbar && !RTGUI_WIDGET_IS_HIDE(box->sbar))
	{
		rtgui_theme_draw_scrollbar(box->sbar);
	}

	rtgui_dc_end_drawing(dc);
}

/* update listbox new/old focus item */
void rtgui_listbox_update(rtgui_listbox_t* box)
{
	const rtgui_listbox_item_t* item;
	rtgui_rect_t rect, item_rect, image_rect;
	rtgui_dc_t* dc;
	
	RT_ASSERT(box != RT_NULL);
	
	if(RTGUI_WIDGET_IS_HIDE(box))return;
	if(box->items==RT_NULL)return;

	/* begin drawing */
	dc = rtgui_dc_begin_drawing(box);
	if(dc == RT_NULL)return;

	rtgui_widget_get_rect(box, &rect);
	if(box->sbar && !RTGUI_WIDGET_IS_HIDE(box->sbar))
	{
		rect.x2 -= rtgui_rect_width(box->sbar->parent.extent);
	}

	if((box->old_aloc >= box->frist_aloc) && /* int front some page */
	   (box->old_aloc < box->frist_aloc+box->item_per_page) && /* int later some page */
	   (box->old_aloc != box->now_aloc)) /* change location */
	{/* these condition dispell blinked when drawed */
		item_rect = rect;
		/* get old item's rect */
		item_rect.x1 += RTGUI_WIDGET_BORDER(box); 
		item_rect.x2 -= RTGUI_WIDGET_BORDER(box);
		item_rect.y1 += RTGUI_WIDGET_BORDER(box);
		item_rect.y1 += ((box->old_aloc-box->frist_aloc) % box->item_per_page) * (2 + box->item_size);
		item_rect.y2 = item_rect.y1 + (2+box->item_size);
	
		/* draw old item */
		RTGUI_DC_BC(dc) = white;
		RTGUI_DC_FC(dc) = black;
		rtgui_dc_fill_rect(dc,&item_rect);
	
		item_rect.x1 += RTGUI_WIDGET_DEFAULT_MARGIN;
	
		item = &(box->items[box->old_aloc]);
		if(item->image != RT_NULL)
		{
			image_rect.x1 = 0; 
			image_rect.y1 = 0;
			image_rect.x2 = item->image->w; 
			image_rect.y2 = item->image->h;
			rtgui_rect_moveto_align(&item_rect, &image_rect, RTGUI_ALIGN_CENTER_VERTICAL);
			rtgui_image_paste(item->image, dc, &image_rect, white);
			item_rect.x1 += item->image->w + 2;
		}
		rtgui_dc_draw_text(dc, item->name, &item_rect);
#ifdef LB_USING_MULTI_SEL
		if(box->multi_sel && item->ext_flag)
		{
			rtgui_rect_t tmprect = rect;
			tmprect.x2 -= RTGUI_WIDGET_BORDER(box);
			tmprect.x2 -= RTGUI_WIDGET_DEFAULT_MARGIN;
			tmprect.x1 = tmprect.x2 - 12;
			tmprect.y1 = item_rect.y1;
			tmprect.y2 = item_rect.y2;
			rtgui_dc_draw_word(dc, tmprect.x1, tmprect.y1+6, 10, lb_ext_flag);
		}
#endif
	}

	/* draw now item */
	item_rect = rect;
	/* get now item's rect */
	item_rect.x1 += RTGUI_WIDGET_BORDER(box); 
	item_rect.x2 -= RTGUI_WIDGET_BORDER(box);
	item_rect.y1 += RTGUI_WIDGET_BORDER(box);
	item_rect.y1 += ((box->now_aloc-box->frist_aloc) % box->item_per_page) * (2 + box->item_size);
	item_rect.y2 = item_rect.y1 + (2 + box->item_size);

	/* draw current item */
	if(RTGUI_WIDGET_IS_FOCUSED(box))
	{	
		RTGUI_DC_BC(dc) = selected_color;
		RTGUI_DC_FC(dc) = white;
		rtgui_dc_fill_rect(dc, &item_rect);
		rtgui_dc_draw_focus_rect(dc, &item_rect); /* draw focus rect */
	}
	else
	{
		RTGUI_DC_BC(dc) = dark_grey;
		RTGUI_DC_FC(dc) = black;
		rtgui_dc_fill_rect(dc, &item_rect);
	}

	item_rect.x1 += RTGUI_WIDGET_DEFAULT_MARGIN;

	item = &(box->items[box->now_aloc]);
	if(item->image != RT_NULL)
	{
		image_rect.x1 = 0; 
		image_rect.y1 = 0;
		image_rect.x2 = item->image->w; 
		image_rect.y2 = item->image->h;
		rtgui_rect_moveto_align(&item_rect, &image_rect, RTGUI_ALIGN_CENTER_VERTICAL);
		rtgui_image_paste(item->image, dc, &image_rect, white);
        item_rect.x1 += (item->image->w + 2);
	}
	if(RTGUI_WIDGET_IS_FOCUSED(box))
	{
		RTGUI_DC_FC(dc) = white;
		rtgui_dc_draw_text(dc, item->name, &item_rect);
	}
	else
	{
		RTGUI_DC_FC(dc) = black;
		rtgui_dc_draw_text(dc, item->name, &item_rect);
	}
#ifdef LB_USING_MULTI_SEL
	if(box->multi_sel && item->ext_flag)
	{
		rtgui_rect_t tmprect = rect;
		tmprect.x2 -= RTGUI_WIDGET_BORDER(box);
		tmprect.x2 -= RTGUI_WIDGET_DEFAULT_MARGIN;
		tmprect.x1 = tmprect.x2 - 12;
		tmprect.y1 = item_rect.y1;
		tmprect.y2 = item_rect.y2;
		rtgui_dc_draw_word(dc, tmprect.x1, tmprect.y1+6, 10, lb_ext_flag);
	}
#endif

	rtgui_dc_end_drawing(dc);
}

static void rtgui_listbox_onmouse(rtgui_listbox_t* box, rtgui_event_mouse_t* emouse)
{
	rtgui_rect_t rect;

	/* get physical extent information */
	rtgui_widget_get_rect(box, &rect);
	if(box->sbar && !RTGUI_WIDGET_IS_HIDE(box->sbar))
		rect.x2 -= rtgui_rect_width(box->sbar->parent.extent);

	if((rtgui_region_contains_point(&RTGUI_WIDGET_CLIP(box), emouse->x, emouse->y,&rect) == RT_EOK) 
		&& (box->item_count > 0))
	{
		rt_uint16_t i;
		i = (emouse->y - rect.y1) / (2 + box->item_size);
		
		/* set focus */
		rtgui_widget_focus(box);
		
		if((i < box->item_count) && (i < box->item_per_page))
		{
			if(emouse->button & RTGUI_MOUSE_BUTTON_DOWN)
			{
				box->old_aloc = box->now_aloc;
				/* set selected item */
				box->now_aloc = box->frist_aloc + i;
				
				if(box->on_item != RT_NULL)
				{	
					box->on_item(box, RT_NULL);
				}
				
				/* down event */
				rtgui_listbox_update(box);
			}
			else if(emouse->button & RTGUI_MOUSE_BUTTON_UP)
			{
				rtgui_listbox_update(box);

				if(box->ispopup && !RTGUI_WIDGET_IS_HIDE(box))
				{
					RTGUI_WIDGET_HIDE(box);
					box->frist_aloc=0;
					box->now_aloc = 0;
					
					rtgui_widget_update_clip(RTGUI_WIDGET_PARENT(box));
					rtgui_widget_update(RTGUI_WIDGET_PARENT(box));
					return;	
				}
			}
			if(box->sbar && !RTGUI_WIDGET_IS_HIDE(box))
			{
				if(!RTGUI_WIDGET_IS_HIDE(box->sbar))
					rtgui_scrollbar_set_value(box->sbar,box->frist_aloc);
			}
		}
	}
}

rt_bool_t rtgui_listbox_event_handler(PVOID wdt, rtgui_event_t* event)
{
	rtgui_widget_t *widget = wdt;
	rtgui_listbox_t* box = (rtgui_listbox_t*)wdt;
	
	RT_ASSERT(box != RT_NULL);
	
	switch (event->type)
	{
		case RTGUI_EVENT_PAINT:
		{
			if(widget->on_draw)
				widget->on_draw(widget, RT_NULL);
			else
				rtgui_listbox_ondraw(box);
			return RT_FALSE;
		}
	    case RTGUI_EVENT_RESIZE:
	        {
				rtgui_event_resize_t* resize;
	
				resize = (rtgui_event_resize_t*)event;
	
	            /* recalculate page items */
				box->item_per_page = resize->h  / (2 + box->item_size);
	        }
	        break;
	
		case RTGUI_EVENT_MOUSE_BUTTON: 
			if(widget->on_mouseclick != RT_NULL) 
			{
				widget->on_mouseclick(widget, event);
			}
			else
			{
				rtgui_listbox_onmouse(box, (rtgui_event_mouse_t*)event);
			}
			break;

	    case RTGUI_EVENT_KBD:
        {
            rtgui_event_kbd_t* ekbd = (rtgui_event_kbd_t*)event;
            if((RTGUI_KBD_IS_DOWN(ekbd)) && (box->item_count > 0))
            {	
				switch (ekbd->key)
                {
	                case RTGUIK_UP:
						if(box->now_aloc > 0)
						{
							box->old_aloc = box->now_aloc;
							box->now_aloc --;
							if(box->now_aloc < box->frist_aloc)
							{
								if(box->frist_aloc)box->frist_aloc--;
								rtgui_listbox_ondraw(box);
							}
							else
							{
								rtgui_listbox_update(box);
							}

							if(box->sbar && !RTGUI_WIDGET_IS_HIDE(box))
							{
								if(!RTGUI_WIDGET_IS_HIDE(box->sbar))
									rtgui_scrollbar_set_value(box->sbar,box->frist_aloc);
							}

							if(box->updown != RT_NULL)
							{
								box->updown(box, RT_NULL);
							}
						}
						break;

	                case RTGUIK_DOWN:
						if(box->now_aloc < box->item_count - 1)
						{
							box->old_aloc = box->now_aloc;
							box->now_aloc ++;	
							if(box->now_aloc >= box->frist_aloc+box->item_per_page)
							{
								box->frist_aloc++;
								rtgui_listbox_ondraw(box);
							}
							else
							{
								rtgui_listbox_update(box);
							}
							if(box->sbar && !RTGUI_WIDGET_IS_HIDE(box))
							{
								if(!RTGUI_WIDGET_IS_HIDE(box->sbar))
									rtgui_scrollbar_set_value(box->sbar,box->frist_aloc);
							}
							
							if(box->updown != RT_NULL)
							{
								box->updown(box, RT_NULL);
							}
						}
						break;
	
					case RTGUIK_RETURN:
						if(box->on_item != RT_NULL)
						{
							box->on_item(box, RT_NULL);
						}
#ifdef LB_USING_MULTI_SEL						
						if(box->multi_sel)
						{
							rtgui_listbox_item_t *item;
							item = &(box->items[box->now_aloc]);
							item->ext_flag = 1;
							rtgui_widget_update(RTGUI_WIDGET_PARENT(box));
						}
#endif
						if(box->ispopup && !RTGUI_WIDGET_IS_HIDE(box))
						{
							RTGUI_WIDGET_HIDE(box);
							box->frist_aloc=0;
							box->now_aloc = 0;
							rtgui_widget_update_clip(RTGUI_WIDGET_PARENT(box));
							rtgui_widget_update(RTGUI_WIDGET_PARENT(box));
						}
						break;
					case RTGUIK_BACKSPACE:
#ifdef LB_USING_MULTI_SEL						
						if(box->multi_sel)
						{
							rtgui_listbox_item_t *item;
							item = &(box->items[box->now_aloc]);
							item->ext_flag = 0;
							rtgui_widget_update(RTGUI_WIDGET_PARENT(box));
						}
#endif
						break;
	
	                default:
	                    break;
                }
            }
			return RT_FALSE;
        }
		default:  
			return rtgui_container_event_handler(box, event);
	}

    /* use box event handler */
    return rtgui_container_event_handler(box, event);
}

void rtgui_listbox_set_onitem(rtgui_listbox_t* box, rtgui_event_handler_ptr func)
{
	if(box == RT_NULL) return;

	box->on_item = func;
}

void rtgui_listbox_set_updown(rtgui_listbox_t* box, rtgui_event_handler_ptr func)
{
	if(box == RT_NULL) return;

	box->updown = func;
}

void rtgui_listbox_delete_item(rtgui_listbox_t* box, rt_uint32_t item_num)
{
	rtgui_listbox_item_t* _items;
	rt_base_t i;

	if(box == RT_NULL) return;

	for(i=item_num;i<box->item_count-1;i++)
	{
		if(box->items[i].name != RT_NULL)	
		{
			rt_free(box->items[i].name);
			box->items[i].name = RT_NULL;
		}
		box->items[i].name = box->items[i+1].name;
		box->items[i].image = box->items[i+1].image;
	}

	box->item_count -= 1;
	_items = rt_realloc(box->items,sizeof(rtgui_listbox_item_t)*(box->item_count));
	if(_items != RT_NULL)	
	{
		box->items = _items;
	}

	rtgui_listbox_ondraw(box);
}

static void rtgui_listbox_add_item(rtgui_listbox_t* box,rtgui_listbox_item_t* item)
{
	rtgui_listbox_item_t* _items;
	RT_ASSERT(box != RT_NULL);

	if(box->item_count==0)
	{
		rtgui_listbox_set_items(box, item, 1);
		if(!RTGUI_WIDGET_IS_HIDE(box))
		{
			rtgui_listbox_ondraw(box);
		}
		return;
	}
	
	_items = rt_realloc(box->items,sizeof(rtgui_listbox_item_t)*(box->item_count+1));

	if(_items != RT_NULL)	
	{
		box->items = _items;
		box->items[box->item_count].name = rt_strdup(item->name);
		box->items[box->item_count].image= item->image;
		box->item_count += 1;
		
		if(box->sbar != RT_NULL)		
		{
			rtgui_panel_t *panel = rtgui_panel_get();
			if(RTGUI_WIDGET_IS_HIDE(box->sbar))
			{
				if(box->item_count > box->item_per_page)
				{
					RTGUI_WIDGET_UNHIDE(box->sbar);
					rtgui_scrollbar_set_line_step(box->sbar, 1);
					rtgui_scrollbar_set_page_step(box->sbar, box->item_per_page);
					rtgui_scrollbar_set_range(box->sbar, box->item_count);
				}
				else
				{
					RTGUI_WIDGET_HIDE(box->sbar);
				}
				rtgui_widget_update_clip(box);
				if(external_clip_size > 0)
				{
					rt_int32_t i;
					rtgui_rect_t *rect = external_clip_rect;
					for(i=0;i<external_clip_size;i++)
					{
						if(rtgui_rect_is_intersect(rect, &RTGUI_WIDGET_EXTENT(box)) == RT_EOK)
						{
							rtgui_panel_update_clip(panel);
							rtgui_panel_redraw(&RTGUI_WIDGET_EXTENT(box));
							break;
						}
						rect++;
					}	
				}
			}
			else
			{
				rtgui_scrollbar_set_range(box->sbar, box->item_count);	
			}
		}
		
		if(!RTGUI_WIDGET_IS_HIDE(box))
		{
			rtgui_listbox_ondraw(box);
		}
	}
}

static rt_uint32_t rtgui_listbox_get_item_count(rtgui_listbox_t* box)
{
	return box->item_count;
}

/* update listbox with assign row */
void rtgui_listbox_update_aloc(rtgui_listbox_t* box, rt_uint16_t aloc)
{
	if(box != RT_NULL)
	{
		if(aloc > (box->item_count-1)) return;
		if(box->item_count > box->item_per_page)
		{
			if((aloc+box->item_per_page) > (box->item_count-1))
			{	
				box->now_aloc = aloc;
				box->old_aloc = aloc;
				box->frist_aloc = box->item_count-box->item_per_page;
			} 
			else
			{	
				box->now_aloc = aloc;
				box->old_aloc = aloc;
				box->frist_aloc = aloc;	
			}
		}
		else
		{
			box->now_aloc = aloc;
		}
		
		rtgui_listbox_ondraw(box);
		
		if(!RTGUI_WIDGET_IS_HIDE(box->sbar))
		{
			rtgui_scrollbar_set_value(box->sbar, box->frist_aloc);
		}
	}
}

static rt_bool_t rtgui_listbox_unfocus(PVOID wdt, rtgui_event_t* event)
{
	rtgui_listbox_t *box = (rtgui_listbox_t*)wdt;
	if(box == RT_NULL)return RT_FALSE;

	if(!RTGUI_WIDGET_IS_FOCUSED(box))
	{/* clear focus rectangle */
		rtgui_listbox_update(RTGUI_LISTBOX(wdt));
	}

	if(box->ispopup)
	{/* this is a popup listbox ,so it hang on the parent widget */
		rtgui_win_t *win;

		RTGUI_WIDGET_HIDE(box);
		box->frist_aloc=0;
		box->now_aloc = 0;
		
		win = rtgui_win_get_win_by_widget(box);
		if(win != RT_NULL)
		{/* it in a window box */ 
			if(rtgui_rect_is_intersect(&(RTGUI_WIDGET_EXTENT(win)), 
				&(RTGUI_WIDGET_EXTENT(box))) == RT_EOK)
			{
				rtgui_topwin_move(win,RTGUI_WIDGET_EXTENT(win).x1,
					RTGUI_WIDGET_EXTENT(win).y1);
			}
			rtgui_widget_focus(win);
			rtgui_widget_update_clip(win);
		}
		rtgui_panel_redraw(&(RTGUI_WIDGET_EXTENT(box)));	
	}

	return RT_TRUE;
}

static rt_bool_t rtgui_listbox_sbar_handle(PVOID wdt, rtgui_event_t* event)
{
	rtgui_listbox_t *box = (rtgui_listbox_t*)wdt;

	/* adjust frist display row when dragging */
	box->frist_aloc = box->sbar->value;	

	rtgui_listbox_ondraw(box);	

	return RT_TRUE;
}

