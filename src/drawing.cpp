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

// SVG Spacewar is copyright 2005 by Nigel Tao: nigel.tao@myrealbox.com
// Licenced under the GNU GPL.
// Developed on cairo version 0.4.0.
//
// 2005-03-31: Version 0.1.

#include "config.h"

#include "drawing.h"

#include "game-math.h"
#include "game-object.h"
#include "score.h"

#include <cairo.h>
#include <stdio.h>

#define ENERGY_BAR_LENGTH (200)

void
draw_text_centered (cairo_t * cr, int font_size, int cx, int cy, int dy, const char *message, double alpha)
{
  double x, y;
  cairo_text_extents_t extents;

  cairo_save (cr);

  cairo_select_font_face (cr, "Serif",
                          CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

  cairo_set_font_size (cr, font_size);
  cairo_text_extents (cr, message, &extents);
  x = cx - (extents.width / 2 + extents.x_bearing);
  y = cy - (extents.height / 2 + extents.y_bearing);

  cairo_set_source_rgba (cr, 1, 1, 0, alpha);
  cairo_move_to (cr, x, y + dy);
  cairo_show_text (cr, message);
  cairo_restore (cr);
}

//------------------------------------------------------------------------------

void
draw_energy_bar (cairo_t * cr, int x, int y, int energy_percent,
                 RGB_t primary_color, RGB_t secondary_color)
{
  cairo_pattern_t *pat;
  double alpha = 0.6;
  int width = int( ENERGY_BAR_LENGTH * (energy_percent / 100.0) );

  cairo_rectangle (cr, x, y, width, 15);

  pat = cairo_pattern_create_linear (0, 0, ENERGY_BAR_LENGTH, 0);
  add_color_stop (pat, 0, secondary_color, alpha);
  add_color_stop (pat, 1, primary_color, alpha);

  cairo_set_source (cr, pat);
  cairo_fill_preserve (cr);
  cairo_pattern_destroy (pat);

  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_stroke (cr);
}

// TODO: Replace str with a Score object
void
draw_score_centered (cairo_t * cr, double cx, double cy, const Score *score)
{
  // TODO: Set text color
  char str[20];
  snprintf(str, 20, "%d", score->amount());

  draw_text_centered(cr, 24, cx, cy, 0, str, 0.75);
}

//------------------------------------------------------------------------------

void
draw_ship_body (cairo_t * cr, GameObject * p)
{
  cairo_pattern_t *pat;

  if (p->is_hit)
  {
    cairo_set_source_rgba (cr, p->primary_color.r, p->primary_color.g,
                           p->primary_color.b, 0.5);
    cairo_arc (cr, 0, 0, SHIP_RADIUS / FIXED_POINT_SCALE_FACTOR, 0, TWO_PI);
    cairo_stroke (cr);
  }

  cairo_save (cr);
  cairo_scale (cr, GLOBAL_SHIP_SCALE_FACTOR, GLOBAL_SHIP_SCALE_FACTOR);

  if (p->is_alive())
  {
    if (p->is_thrusting)
      draw_flare (cr, p->primary_color);

    if (p->p.rotation_accel < 0)
      draw_turning_flare (cr, p->primary_color, -1);

    if (p->p.rotation_accel > 0)
      draw_turning_flare (cr, p->primary_color, 1);
  }

  cairo_move_to (cr, 0, -33);
  cairo_curve_to (cr, 2, -33, 3, -34, 4, -35);
  cairo_curve_to (cr, 8, -10, 6, 15, 15, 15);
  cairo_line_to (cr, 20, 15);
  cairo_line_to (cr, 20, 7);
  cairo_curve_to (cr, 25, 10, 28, 22, 25, 28);
  cairo_curve_to (cr, 20, 26, 8, 24, 0, 24);
  // half way point
  cairo_curve_to (cr, -8, 24, -20, 26, -25, 28);
  cairo_curve_to (cr, -28, 22, -25, 10, -20, 7);
  cairo_line_to (cr, -20, 15);
  cairo_line_to (cr, -15, 15);
  cairo_curve_to (cr, -6, 15, -8, -10, -4, -35);
  cairo_curve_to (cr, -3, -34, -2, -33, 0, -33);

  pat = cairo_pattern_create_linear (-30.0, -30.0, 30.0, 30.0);
  add_color_stop (pat, 0, p->primary_color, 1);
  add_color_stop (pat, 1, p->secondary_color, 1);

  cairo_set_source (cr, pat);
  cairo_fill_preserve (cr);
  cairo_pattern_destroy (pat);

  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_stroke (cr);
  cairo_restore (cr);
}

void
draw_cannon (cairo_t * cr, GameObject * p)
{
  cairo_pattern_t *pat;

  if (p->is_hit)
  {
    cairo_set_source_rgba (cr, p->primary_color.r, p->primary_color.g,
                           p->primary_color.b, 0.5);
    cairo_arc (cr, 0, 0, SHIP_RADIUS / FIXED_POINT_SCALE_FACTOR, 0, TWO_PI);
    cairo_stroke (cr);
  }

  cairo_save (cr);
  cairo_scale (cr, GLOBAL_SHIP_SCALE_FACTOR, GLOBAL_SHIP_SCALE_FACTOR);

  if (p->is_alive())
  {

    if (p->is_thrusting)
      draw_flare (cr, p->primary_color);

    if (p->p.rotation_speed < 0)
      draw_turning_flare (cr, p->primary_color, -1);

    if (p->p.rotation_speed > 0)
      draw_turning_flare (cr, p->primary_color, 1);
  }

  cairo_set_line_width (cr, 2.0);
  cairo_arc (cr, 0, 0, p->p.radius/FIXED_POINT_SCALE_FACTOR, 5.0/180.0, TWO_PI);

  cairo_move_to (cr, 6, -28);
  cairo_line_to (cr, 6, -45);
  cairo_line_to (cr, -6, -45);
  cairo_line_to (cr, -6, -28);

  pat = cairo_pattern_create_linear (-30.0, -30.0, 30.0, 30.0);
  add_color_stop (pat, 0, p->primary_color, 1);
  add_color_stop (pat, 1, p->secondary_color, 1);

  cairo_set_source (cr, pat);
  cairo_fill_preserve (cr);
  cairo_pattern_destroy (pat);

  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_stroke (cr);

  cairo_restore (cr);
}

//------------------------------------------------------------------------------

void
draw_flare (cairo_t * cr, RGB_t color)
{
  cairo_pattern_t *pat;

  RGB_t color_white = {1,1,1};

  cairo_save (cr);

  cairo_translate (cr, 0, 22);
  pat = cairo_pattern_create_radial (0, 0, 2, 0, 5, 12);
  add_color_stop (pat, 0.0, color, 1);
  add_color_stop (pat, 0.3, color_white, 1);
  add_color_stop (pat, 1.0, color, 0);
  cairo_set_source (cr, pat);
  cairo_arc (cr, 0, 0, 20, 0, TWO_PI);

  cairo_fill (cr);
  cairo_pattern_destroy (pat);
  cairo_restore (cr);
}

//------------------------------------------------------------------------------

void
draw_turning_flare (cairo_t * cr, RGB_t color, int right_hand_side)
{
  cairo_pattern_t *pat;

  RGB_t color_white = {1,1,1};

  cairo_save (cr);

  cairo_translate (cr, -23 * right_hand_side, 28);
  pat = cairo_pattern_create_radial (0, 0, 1, 0, 0, 7);
  add_color_stop (pat, 0.0, color_white, 1);
  add_color_stop (pat, 1.0, color, 0);
  cairo_set_source (cr, pat);
  cairo_arc (cr, 0, 0, 7, 0, TWO_PI);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

  cairo_translate (cr, 42 * right_hand_side, -22);
  pat = cairo_pattern_create_radial (0, 0, 1, 0, 0, 7);
  add_color_stop (pat, 0.0, color_white, 1);
  add_color_stop (pat, 1.0, color, 0);
  cairo_set_source (cr, pat);
  cairo_arc (cr, 0, 0, 5, 0, TWO_PI);

  cairo_fill (cr);
  cairo_pattern_destroy (pat);
  cairo_restore (cr);
}

//------------------------------------------------------------------------------

void
draw_ring (cairo_t * cr, GameObject * r) {
  for (int i=0; i<SEGMENTS_PER_RING; i++) {
    if (r->component_energy[i] <= 0)
      continue;

    cairo_save (cr);
    cairo_set_line_width (cr, r->component_energy[i]*4);
    cairo_arc (cr, 0, 0, r->p.radius/FIXED_POINT_SCALE_FACTOR,
               i * TWO_PI/SEGMENTS_PER_RING,
               (i+1) * TWO_PI/SEGMENTS_PER_RING - TWO_PI/180.0);
    cairo_stroke (cr);

    cairo_restore (cr);
  }
}

//------------------------------------------------------------------------------

void
draw_missile (cairo_t * cr, GameObject * m)
{
  cairo_save (cr);
  cairo_scale (cr, GLOBAL_SHIP_SCALE_FACTOR, GLOBAL_SHIP_SCALE_FACTOR);

  if (m->has_exploded)
  {
    draw_exploded_missile (cr, m);
  }
  else
  {
    cairo_pattern_t *pat;

    double alpha = ((double) m->energy) / MISSILE_TICKS_TO_LIVE;
    // non-linear scaling so things don't fade out too fast
    alpha = 1.0 - (1.0 - alpha) * (1.0 - alpha);

    cairo_save (cr);
    cairo_move_to (cr, 0, -4);
    cairo_curve_to (cr, 3, -4, 4, -2, 4, 0);
    cairo_curve_to (cr, 4, 4, 2, 10, 0, 18);
    // half way point
    cairo_curve_to (cr, -2, 10, -4, 4, -4, 0);
    cairo_curve_to (cr, -4, -2, -3, -4, 0, -4);

    pat = cairo_pattern_create_linear (0.0, -5.0, 0.0, 5.0);
    add_color_stop (pat, 0, m->primary_color, alpha);
    add_color_stop (pat, 1, m->secondary_color, alpha);

    cairo_set_source (cr, pat);
    cairo_fill (cr);
    cairo_pattern_destroy (pat);
    cairo_restore (cr);

    cairo_save (cr);
    cairo_arc (cr, 0, 0, 3, 0, TWO_PI);

    pat = cairo_pattern_create_linear (0, 3, 0, -3);
    add_color_stop (pat, 0, m->primary_color, alpha);
    add_color_stop (pat, 1, m->secondary_color, alpha);

    cairo_set_source (cr, pat);
    cairo_fill (cr);
    cairo_pattern_destroy (pat);
    cairo_restore (cr);
  }

  cairo_restore (cr);
}

//------------------------------------------------------------------------------

void
draw_exploded_missile (cairo_t * cr, GameObject * m)
{
  double alpha;
  cairo_pattern_t *pat;

  RGB_t color_black = {0,0,0};

  cairo_save (cr);
  cairo_scale (cr, GLOBAL_SHIP_SCALE_FACTOR, GLOBAL_SHIP_SCALE_FACTOR);

  alpha = ((double) m->energy) / MISSILE_EXPLOSION_TICKS_TO_LIVE;
  alpha = 1.0 - (1.0 - alpha) * (1.0 - alpha);

  cairo_arc (cr, 0, 0, 30, 0, TWO_PI);

  pat = cairo_pattern_create_radial (0, 0, 0, 0, 0, 30);
  add_color_stop (pat, 0,   m->primary_color, alpha);
  add_color_stop (pat, 0.5, m->secondary_color, alpha * 0.75);
  add_color_stop (pat, 1,   color_black, 0);

  cairo_set_source (cr, pat);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);
  cairo_restore (cr);
}

//------------------------------------------------------------------------------

void
draw_star (cairo_t * cr, CanvasItem *)
{
  int a = NUMBER_OF_ROTATION_ANGLES / 10;
  float r1 = 5.0;
  float r2 = 2.0;
  float c;
  int i;

  cairo_save (cr);
  cairo_move_to (cr, r1 * cos_table[0] / FIXED_POINT_SCALE_FACTOR,
                 r1 * sin_table[0] / FIXED_POINT_SCALE_FACTOR);

  for (i = 0; i < 5; i++) {
    cairo_line_to (cr, r1 * cos_table[0] / FIXED_POINT_SCALE_FACTOR,
                   r1 * sin_table[0] / FIXED_POINT_SCALE_FACTOR);
    cairo_line_to (cr, r2 * cos_table[a] / FIXED_POINT_SCALE_FACTOR,
                   r2 * sin_table[a] / FIXED_POINT_SCALE_FACTOR);
    cairo_rotate (cr, 4*a*PI/NUMBER_OF_ROTATION_ANGLES);
  }

  cairo_close_path (cr);
  cairo_restore (cr);

  c = 0.5;
  cairo_set_source_rgb (cr, c, c, c);
  cairo_fill (cr);
}

//------------------------------------------------------------------------------

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
