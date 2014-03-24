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

void draw_energy_bar (cairo_t *, GameObject *player);
void draw_flare (cairo_t *, RGB_t);
void draw_ring (cairo_t *, GameObject *ring);
void draw_missile (cairo_t *, GameObject *missile);
void draw_exploded_missile (cairo_t *, GameObject *missile);
void draw_ship_body (cairo_t *, GameObject *player);
void draw_cannon (cairo_t *, GameObject *player);
void draw_star (cairo_t * cr, CanvasItem * item);
void draw_turning_flare (cairo_t *, RGB_t, int);
void show_text_message (cairo_t *, int, int, const char *);

#endif
