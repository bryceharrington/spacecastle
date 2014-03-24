/* spacecastle - A vector graphics space shooter game
 *
 * Copyright © 2014 Bryce Harrington
 *
 * Spacecastle is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Spacecastle is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Spacecastle.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>
#include "canvas.h"

CanvasItem::CanvasItem (canvas_item_draw f)
    : x(0), y(0), rotation(0.0), scale(1.0), draw_func(f)
{
}

void
CanvasItem::draw (cairo_t * cr)
{
    if (!draw_func)
        return;

    cairo_save (cr);
    cairo_translate (cr, this->x, this->y);
    cairo_rotate (cr, this->rotation);
    cairo_scale (cr, this->scale, this->scale);
    (*draw_func) (cr, this);
    cairo_restore (cr);
}


void
CanvasItem::set_theme(RGB_t primary, RGB_t secondary)
{
    primary_color = primary;
    secondary_color = secondary;
}

Canvas::Canvas(int w, int h): width(w), height(h), debug_scale_factor(1.0)
{
}

void
Canvas::scale_for_aspect_ratio(cairo_t *cr, int window_width, int window_height)
{
    double scale;
    int playfield_width, playfield_height;
    int tx, ty;
    gboolean is_window_wider;

    cairo_save (cr);

    is_window_wider = (window_width * height) > (width * window_height);

    if (is_window_wider)
    {
        scale = ((double) window_height) / height;
        playfield_width = (width * window_height) / height;
        playfield_height = window_height;
        tx = (window_width - playfield_width) / 2;
        ty = 0;
    }
    else
    {
        scale = ((double) window_width) / width;
        playfield_width = window_width;
        playfield_height = (height * window_width) / width;
        tx = 0;
        ty = (window_height - playfield_height) / 2;
    }

    cairo_translate (cr, tx, ty);
    cairo_rectangle (cr, 0, 0, playfield_width, playfield_height);
    cairo_clip (cr);

    cairo_scale (cr, scale, scale);

    cairo_scale (cr, debug_scale_factor, debug_scale_factor);
}
