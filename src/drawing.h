/* spacecastle - A vector graphics space shooter game
 *
 * Copyright © 2006 Nigel Tao: nigel.tao@myrealbox.com
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

#ifndef __DRAWING_H__
#define __DRAWING_H__

#include "forward.h"

void draw_energy_bar (cairo_t *, int x, int y, int energy_percent,
                      RGB_t primary_color, RGB_t secondary_color);
void draw_score_centered (cairo_t * cr, double x, double y, const char *str);
void draw_flare (cairo_t *, RGB_t);
void draw_ring (cairo_t *, GameObject *ring);
void draw_missile (cairo_t *, GameObject *missile);
void draw_exploded_missile (cairo_t *, GameObject *missile);
void draw_ship_body (cairo_t *, GameObject *player);
void draw_cannon (cairo_t *, GameObject *player);
void draw_star (cairo_t * cr, CanvasItem * item);
void draw_turning_flare (cairo_t *, RGB_t, int);
void draw_text_centered (cairo_t *, int font_size, int cx, int cy, int dy, const char *message, double alpha);

#endif

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-basic-offset:2
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=2:tabstop=8:softtabstop=2:fileencoding=utf-8:textwidth=99 :
