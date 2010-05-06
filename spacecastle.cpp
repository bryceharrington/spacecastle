// SVG Spacewar is copyright 2005 by Nigel Tao: nigel.tao@myrealbox.com
// Licenced under the GNU GPL.
// Developed on cairo version 0.4.0.
//
// 2005-03-31: Version 0.1.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <gtk/gtk.h>
#include <sys/timeb.h>

#include "canvas.h"
#include "game-object.h"
#include "game-math.h"
#include "game.h"

//------------------------------------------------------------------------------
// Forward definitions of functions

static void operate_cannon (GameObject *cannon, GameObject *player);
static void apply_physics (physics_t *);
static void apply_physics_to_player (GameObject *player);
static gboolean check_for_collision (physics_t *, physics_t *);
static gboolean check_for_ring_segment_collision (physics_t *, physics_t *);
void draw_energy_bar (cairo_t *, GameObject *player);
static void draw_flare (cairo_t *, RGB_t);
static void draw_ring (cairo_t *, GameObject *ring);
static void draw_missile (cairo_t *, GameObject *missile);
static void draw_exploded_missile (cairo_t *, GameObject *missile);
static void draw_ship_body (cairo_t *, GameObject *player);
void draw_star (cairo_t * cr, CanvasItem * item);
static void draw_turning_flare (cairo_t *, RGB_t, int);
static void enforce_minimum_distance (physics_t *, physics_t *);
static long get_time_millis (void);
void init_trigonometric_tables (void);
static void on_collision (GameObject *player, GameObject *missile);
static int ring_segment_hit (GameObject *ring, GameObject *missile);
static void on_ring_segment_collision (GameObject * ring, GameObject * m, int segment);
gint on_expose_event (GtkWidget *, GdkEventExpose *);
gint on_key_event (GtkWidget *, GdkEventKey *, gboolean);
gint on_timeout (gpointer);
static void scale_for_aspect_ratio (cairo_t *, int, int);
static void show_text_message (cairo_t *, int, int, const char *);

//------------------------------------------------------------------------------

static gboolean show_fps = TRUE;
static int number_of_frames = 0;
static long millis_taken_for_frames = 0;

//------------------------------------------------------------------------------

static int cos_table[NUMBER_OF_ROTATION_ANGLES];
static int sin_table[NUMBER_OF_ROTATION_ANGLES];

void
init_trigonometric_tables ()
{
  int i;
  int q = (NUMBER_OF_ROTATION_ANGLES / 4);

  for (i = 0; i < NUMBER_OF_ROTATION_ANGLES; i++)
    {
      // our angle system is "true north" - 0 is straight up, whereas
      // cos & sin take 0 as east (and in radians).
      double angle_in_radians = (q - i) * TWO_PI / NUMBER_OF_ROTATION_ANGLES;
      cos_table[i] =
	+(int) (cos (angle_in_radians) * FIXED_POINT_SCALE_FACTOR);

      // also, our graphics system is "y axis down", although in regular math,
      // the y axis is "up", so we have to multiply sin by -1.
      sin_table[i] =
	-(int) (sin (angle_in_radians) * FIXED_POINT_SCALE_FACTOR);
    }
}

static int
arctan (double y, double x)
{
    int rot = (atan2(y, x) + PI) * NUMBER_OF_ROTATION_ANGLES / TWO_PI;
    return ( rot + (3 * NUMBER_OF_ROTATION_ANGLES / 4) ) % NUMBER_OF_ROTATION_ANGLES;
}

//------------------------------------------------------------------------------

static long
get_time_millis (void)
{
  struct timeb tp;
  ftime (&tp);
  return (long) ((tp.time * 1000) + tp.millitm);
}

//------------------------------------------------------------------------------

gint
on_expose_event (GtkWidget * widget, GdkEventExpose * event)
{
  cairo_t *cr = gdk_cairo_create (widget->window);
  int i;
  long start_time = 0;
  if (show_fps)
    {
      start_time = get_time_millis ();
    }

  cairo_save (cr);

  scale_for_aspect_ratio (cr, widget->allocation.width,
			  widget->allocation.height);

  cairo_scale (cr, game->debug_scale_factor, game->debug_scale_factor);

  /* draw background space color */
  cairo_set_source_rgb (cr, 0.1, 0.0, 0.1);
  cairo_paint (cr);

  // draw any stars...
  for (i = 0; i < NUMBER_OF_STARS; i++)
    {
      game->stars[i].draw(cr);
    }

  // ... the energy bars...
  game->cannon_status->draw(cr);
  game->player_status->draw(cr);

  // ... the two ships...
  cairo_save (cr);
  cairo_translate (cr, game->cannon->p.x / FIXED_POINT_SCALE_FACTOR,
		   game->cannon->p.y / FIXED_POINT_SCALE_FACTOR);
  cairo_rotate (cr, game->cannon->p.rotation * RADIANS_PER_ROTATION_ANGLE);
  draw_ship_body (cr, game->cannon);
  cairo_restore (cr);

  cairo_save (cr);
  cairo_translate (cr, game->player->p.x / FIXED_POINT_SCALE_FACTOR,
		   game->player->p.y / FIXED_POINT_SCALE_FACTOR);
  cairo_rotate (cr, game->player->p.rotation * RADIANS_PER_ROTATION_ANGLE);
  draw_ship_body (cr, game->player);
  cairo_restore (cr);

  // ... and any missiles.
  for (i = 0; i < MAX_NUMBER_OF_MISSILES; i++)
    {
      if (game->missiles[i].is_alive)
	{
	  cairo_save (cr);
	  cairo_translate (cr, game->missiles[i].p.x / FIXED_POINT_SCALE_FACTOR,
			   game->missiles[i].p.y / FIXED_POINT_SCALE_FACTOR);
	  cairo_rotate (cr,
			game->missiles[i].p.rotation * RADIANS_PER_ROTATION_ANGLE);
	  draw_missile (cr, &(game->missiles[i]));
	  cairo_restore (cr);
	}
    }

  // and now the rings too
  for (i = 0; i <MAX_NUMBER_OF_RINGS; i++)
    {
      if (game->rings[i].is_alive)
	{
	  cairo_save (cr);
	  cairo_translate (cr, game->rings[i].p.x / FIXED_POINT_SCALE_FACTOR,
			   game->rings[i].p.y / FIXED_POINT_SCALE_FACTOR);
	  cairo_rotate (cr,
			game->rings[i].p.rotation * RADIANS_PER_ROTATION_ANGLE);

          cairo_set_source_rgba (cr, 2-i, i? 1.0/i : 0, 0, 0.6);

	  draw_ring (cr, &(game->rings[i]));
	  cairo_restore (cr);
	}
    }

  // TODO:  Finally, the space mines

  if (game->game_over_message == NULL)
    {
      if (!game->cannon->is_alive)
	{
	  game->game_over_message = (!game->player->is_alive) ? "DRAW" : "RED wins";
	}
      else
	{
	  game->game_over_message = (!game->player->is_alive) ? "BLUE wins" : NULL;
	}
    }
  if (game->game_over_message != NULL)
    {
      show_text_message (cr, 80, -30, game->game_over_message);
      show_text_message (cr, 30, +40, "Press [ENTER] to restart");
    }

  cairo_restore (cr);

  if (show_fps)
    {
      number_of_frames++;
      millis_taken_for_frames += get_time_millis () - start_time;
      if (number_of_frames >= 100)
	{
	  double fps =
	    1000.0 * ((double) number_of_frames) /
	    ((double) millis_taken_for_frames);
	  printf ("%d frames in %ldms (%.3ffps)\n", number_of_frames,
		  millis_taken_for_frames, fps);
	  number_of_frames = 0;
	  millis_taken_for_frames = 0L;
	}
    }

  cairo_destroy (cr);
  return TRUE;
}

//------------------------------------------------------------------------------

static void
scale_for_aspect_ratio (cairo_t * cr, int widget_width, int widget_height)
{
  double scale;
  int playfield_width, playfield_height;
  int tx, ty;
  gboolean is_widget_wider;

  is_widget_wider = (widget_width * HEIGHT) > (WIDTH * widget_height);

  if (is_widget_wider)
    {
      scale = ((double) widget_height) / HEIGHT;
      playfield_width = (WIDTH * widget_height) / HEIGHT;
      playfield_height = widget_height;
      tx = (widget_width - playfield_width) / 2;
      ty = 0;
    }
  else
    {
      scale = ((double) widget_width) / WIDTH;
      playfield_width = widget_width;
      playfield_height = (HEIGHT * widget_width) / WIDTH;
      tx = 0;
      ty = (widget_height - playfield_height) / 2;
    }

  cairo_translate (cr, tx, ty);
  cairo_rectangle (cr, 0, 0, playfield_width, playfield_height);
  cairo_clip (cr);

  cairo_scale (cr, scale, scale);
}

//------------------------------------------------------------------------------

void
draw_energy_bar (cairo_t * cr, GameObject * p)
{
  cairo_pattern_t *pat;
  double alpha = 0.6;

  cairo_rectangle (cr, 0, -5, p->energy / 5, 10);

  pat = cairo_pattern_create_linear (0, 0, SHIP_MAX_ENERGY / 5, 0);
  cairo_pattern_add_color_stop_rgba (pat, 0,
				     p->secondary_color.r,
				     p->secondary_color.g,
				     p->secondary_color.b, alpha);
  cairo_pattern_add_color_stop_rgba (pat, 1, p->primary_color.r,
				     p->primary_color.g, p->primary_color.b,
				     alpha);

  cairo_set_source (cr, pat);
  cairo_fill_preserve (cr);
  cairo_pattern_destroy (pat);

  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_stroke (cr);
}

//------------------------------------------------------------------------------

static void
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

  if (p->is_alive)
    {

      if (p->is_thrusting)
	{
	  draw_flare (cr, p->primary_color);
	}

      if (p->is_turning_left && !p->is_turning_right)
	{
	  draw_turning_flare (cr, p->primary_color, -1);
	}

      if (!p->is_turning_left && p->is_turning_right)
	{
	  draw_turning_flare (cr, p->primary_color, 1);
	}
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
  cairo_pattern_add_color_stop_rgba (pat, 0,
				     p->primary_color.r, p->primary_color.g,
				     p->primary_color.b, 1);
  cairo_pattern_add_color_stop_rgba (pat, 1, p->secondary_color.r,
				     p->secondary_color.g,
				     p->secondary_color.b, 1);

  cairo_set_source (cr, pat);
  cairo_fill_preserve (cr);
  cairo_pattern_destroy (pat);

  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_stroke (cr);
  cairo_restore (cr);
}

//------------------------------------------------------------------------------

static void
draw_flare (cairo_t * cr, RGB_t color)
{
  cairo_pattern_t *pat;

  cairo_save (cr);
  cairo_translate (cr, 0, 22);
  pat = cairo_pattern_create_radial (0, 0, 2, 0, 5, 12);

  cairo_pattern_add_color_stop_rgba (pat, 0.0, color.r, color.g, color.b, 1);
  cairo_pattern_add_color_stop_rgba (pat, 0.3, 1, 1, 1, 1);
  cairo_pattern_add_color_stop_rgba (pat, 1.0, color.r, color.g, color.b, 0);
  cairo_set_source (cr, pat);
  cairo_arc (cr, 0, 0, 20, 0, TWO_PI);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);
  cairo_restore (cr);
}

//------------------------------------------------------------------------------

static void
draw_turning_flare (cairo_t * cr, RGB_t color, int right_hand_side)
{
  cairo_pattern_t *pat;
  cairo_save (cr);

  cairo_translate (cr, -23 * right_hand_side, 28);
  pat = cairo_pattern_create_radial (0, 0, 1, 0, 0, 7);
  cairo_pattern_add_color_stop_rgba (pat, 0.0, 1, 1, 1, 1);
  cairo_pattern_add_color_stop_rgba (pat, 1.0, color.r, color.g, color.b, 0);
  cairo_set_source (cr, pat);
  cairo_arc (cr, 0, 0, 7, 0, TWO_PI);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

  cairo_translate (cr, 42 * right_hand_side, -22);
  pat = cairo_pattern_create_radial (0, 0, 1, 0, 0, 7);
  cairo_pattern_add_color_stop_rgba (pat, 0.0, 1, 1, 1, 1);
  cairo_pattern_add_color_stop_rgba (pat, 1.0, color.r, color.g, color.b, 0);
  cairo_set_source (cr, pat);
  cairo_arc (cr, 0, 0, 5, 0, TWO_PI);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

  cairo_restore (cr);
}

//------------------------------------------------------------------------------

static void
draw_ring (cairo_t * cr, GameObject * r) {
  for (int i=0; i<SEGMENTS_PER_RING; i++) {
      cairo_rotate (cr, TWO_PI * i/SEGMENTS_PER_RING);
      if (!(r->energy & (1 << (2*i)))
          && !(r->energy & (1 << (2*i+1))))
          continue;

      cairo_save (cr);
      cairo_rotate (cr, -5.0/8.0*PI);
      cairo_scale (cr, GLOBAL_SHIP_SCALE_FACTOR, GLOBAL_SHIP_SCALE_FACTOR);

      cairo_set_line_width (cr, 5.0);
      cairo_arc (cr, 0, 0, r->p.radius/FIXED_POINT_SCALE_FACTOR, 5.0/180.0, TWO_PI/SEGMENTS_PER_RING);
      cairo_stroke (cr);

      cairo_restore (cr);
  }
}

//------------------------------------------------------------------------------

static void
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
      cairo_pattern_add_color_stop_rgba (pat, 0,
					 m->primary_color.r,
					 m->primary_color.g,
					 m->primary_color.b, alpha);
      cairo_pattern_add_color_stop_rgba (pat, 1, m->secondary_color.r,
					 m->secondary_color.g,
					 m->secondary_color.b, alpha);

      cairo_set_source (cr, pat);
      cairo_fill (cr);
      cairo_pattern_destroy (pat);
      cairo_restore (cr);

      cairo_save (cr);
      cairo_arc (cr, 0, 0, 3, 0, TWO_PI);

      pat = cairo_pattern_create_linear (0, 3, 0, -3);
      cairo_pattern_add_color_stop_rgba (pat, 0,
					 m->primary_color.r,
					 m->primary_color.g,
					 m->primary_color.b, alpha);
      cairo_pattern_add_color_stop_rgba (pat, 1, m->secondary_color.r,
					 m->secondary_color.g,
					 m->secondary_color.b, alpha);

      cairo_set_source (cr, pat);
      cairo_fill (cr);
      cairo_pattern_destroy (pat);
      cairo_restore (cr);
    }

  cairo_restore (cr);
}

//------------------------------------------------------------------------------

static void
draw_exploded_missile (cairo_t * cr, GameObject * m)
{
  double alpha;
  cairo_pattern_t *pat;

  cairo_save (cr);
  cairo_scale (cr, GLOBAL_SHIP_SCALE_FACTOR, GLOBAL_SHIP_SCALE_FACTOR);

  alpha = ((double) m->energy) / MISSILE_EXPLOSION_TICKS_TO_LIVE;
  alpha = 1.0 - (1.0 - alpha) * (1.0 - alpha);

  cairo_arc (cr, 0, 0, 30, 0, TWO_PI);

  pat = cairo_pattern_create_radial (0, 0, 0, 0, 0, 30);
  cairo_pattern_add_color_stop_rgba (pat, 0,
				     m->primary_color.r, m->primary_color.g,
				     m->primary_color.b, alpha);
  cairo_pattern_add_color_stop_rgba (pat, 0.5, m->secondary_color.r,
				     m->secondary_color.g,
				     m->secondary_color.b, alpha * 0.75);
  cairo_pattern_add_color_stop_rgba (pat, 1, 0, 0, 0, 0);

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

gint
on_timeout (gpointer data)
{
  int i, j;

  game->cannon->is_hit = FALSE;
  game->player->is_hit = FALSE;
  for (j=0; j< MAX_NUMBER_OF_RINGS; j++) {
      game->rings[j].is_hit = FALSE;
  }

  operate_cannon (game->cannon, game->player);
  apply_physics_to_player (game->cannon);
  apply_physics_to_player (game->player);

  if (check_for_collision (&(game->rings[0].p), &(game->player->p)))
    {
      int p1vx, p1vy, p2vx, p2vy;
      int dvx, dvy, dv2;
      int damage;

      enforce_minimum_distance (&(game->rings[0].p), &(game->player->p));

      p1vx = game->rings[0].p.vx;
      p1vy = game->rings[0].p.vy;
      p2vx = game->player->p.vx;
      p2vy = game->player->p.vy;

      dvx = (p1vx - p2vx) / FIXED_POINT_HALF_SCALE_FACTOR;
      dvy = (p1vy - p2vy) / FIXED_POINT_HALF_SCALE_FACTOR;
      dv2 = (dvx * dvx) + (dvy * dvy);
      damage = ((int)(sqrt (dv2))) / DAMAGE_PER_SHIP_BOUNCE_DIVISOR;

      game->player->energy -= damage;
      game->player_status->energy = game->player->energy;
      game->player->is_hit = TRUE;
      game->player->p.vx = (p1vx * +5 / 8) + (p2vx * -2 / 8);
      game->player->p.vy = (p1vy * +5 / 8) + (p2vy * -2 / 8);
    }

  for (i = 0; i < MAX_NUMBER_OF_MISSILES; i++)
    {
      if (game->missiles[i].is_alive)
	{
	  apply_physics (&(game->missiles[i].p));

	  if (!game->missiles[i].has_exploded)
	    {
	      if (check_for_collision (&(game->missiles[i].p), &(game->cannon->p)))
		{
		  on_collision (game->cannon, &(game->missiles[i]));
		}

	      if (check_for_collision (&(game->missiles[i].p), &(game->player->p)))
		{
		  on_collision (game->player, &(game->missiles[i]));
		}

              /* TODO: Foreach ring segment, check collision */
              for (j = 0; j < MAX_NUMBER_OF_RINGS; j++) {
                  if (!game->rings[j].is_alive)
                      continue;

                  if (check_for_ring_segment_collision (&(game->rings[j].p), &(game->missiles[i].p)))
                  {
                      int segment = ring_segment_hit(&(game->rings[j]),
                                                     &(game->missiles[i]));

                      printf("Collision of missile %d with ring %d segment %d\n",
                             i, j, segment);

                      on_ring_segment_collision (&(game->rings[j]),
                                                 &(game->missiles[i]),
                                                 segment);
                  }
              }
	    }

	  game->missiles[i].energy--;
	  if (game->missiles[i].energy <= 0)
	    {
	      game->missiles[i].is_alive = FALSE;
	    }
	}
    }

  int rot = 1;
  for (i = 0; i < MAX_NUMBER_OF_RINGS; i++)
  {
      rot *= -1;
      game->rings[i].p.rotation = (rot + game->rings[i].p.rotation)
          % NUMBER_OF_ROTATION_ANGLES;
  }

  if (game->cannon->energy <= 0)
    {
      game->cannon->energy = 0;
      game->cannon_status->energy = 0;
      game->cannon->is_alive = FALSE;
    }
  else
    {
      game->cannon->energy = MIN (SHIP_MAX_ENERGY, game->cannon->energy + 3);
      game->cannon_status->energy = MIN (SHIP_MAX_ENERGY, game->cannon->energy + 3);
    }

  if (game->player->energy <= 0)
    {
      game->player->energy = 0;
      game->player_status->energy = 0;
      game->player->is_alive = FALSE;
    }
  else
    {
      game->player->energy = MIN (SHIP_MAX_ENERGY, game->player->energy + 1);
      game->player_status->energy = MIN (SHIP_MAX_ENERGY, game->player->energy + 1);
    }

  gtk_widget_queue_draw ((GtkWidget *) data);
  return TRUE;
}

//------------------------------------------------------------------------------

static void
operate_cannon (GameObject * cannon, GameObject * player)
{
    int direction;

    physics_t *c = &(cannon->p);
    physics_t *p = &(player->p);

    if (! player->is_alive) {
        /* TODO:  Reset */
        cannon->is_firing = FALSE;
        return;
    }

    direction = arctan ( (p->y - c->y), (p->x - c->x) );

    if (direction == c->rotation) {
        cannon->is_firing = TRUE;
        cannon->is_turning_left = FALSE;
        cannon->is_turning_right = FALSE;
    } else if (c->rotation - direction == NUMBER_OF_ROTATION_ANGLES/2) {
        cannon->is_firing = FALSE;
        // Stay going in same direction
    } else if (c->rotation - direction < NUMBER_OF_ROTATION_ANGLES/2
               && c->rotation - direction > 0) {
        cannon->is_firing = FALSE;
        cannon->is_turning_right = FALSE;
        cannon->is_turning_left = TRUE;

    } else if (direction - c->rotation > NUMBER_OF_ROTATION_ANGLES/2
               && c->rotation - direction < 0) {
        cannon->is_firing = FALSE;
        cannon->is_turning_right = FALSE;
        cannon->is_turning_left = TRUE;

    } else {
        cannon->is_firing = FALSE;
        cannon->is_turning_right = TRUE;
        cannon->is_turning_left = FALSE;
    }

    // TODO:  On advanced levels, take into account the speed the player
    // is going, and try to lead him a bit.
}

static void
apply_physics_to_player (GameObject * player)
{
  int v2, m2;
  physics_t *p = &(player->p);

  if (player->is_alive)
    {
      // check if player is turning left, ...
      if (player->is_turning_left)
	{
	  p->rotation--;
	  while (p->rotation < 0)
	    {
	      p->rotation += NUMBER_OF_ROTATION_ANGLES;
	    }
	}

      // ... or right.
      if (player->is_turning_right)
	{
	  p->rotation++;
	  while (p->rotation >= NUMBER_OF_ROTATION_ANGLES)
	    {
	      p->rotation -= NUMBER_OF_ROTATION_ANGLES;
	    }
	}

      // check if accelerating
      if (player->is_thrusting)
	{
	  p->vx += SHIP_ACCELERATION_FACTOR * cos_table[p->rotation];
	  p->vy += SHIP_ACCELERATION_FACTOR * sin_table[p->rotation];
	}

	// check if reversing
      if (player->is_reversing)
	{
	  p->vx -= SHIP_ACCELERATION_FACTOR * cos_table[p->rotation];
	  p->vy -= SHIP_ACCELERATION_FACTOR * sin_table[p->rotation];
	}

      // apply velocity upper bound
      v2 = ((p->vx) * (p->vx)) + ((p->vy) * (p->vy));
      m2 = SHIP_MAX_VELOCITY * SHIP_MAX_VELOCITY;
      if (v2 > m2)
	{
	  p->vx = (int) (((double) (p->vx) * m2) / v2);
	  p->vy = (int) (((double) (p->vy) * m2) / v2);
	}

      // check if player is shooting
      if (player->ticks_until_can_fire == 0)
	{
	  if ((player->is_firing) && (player->energy > ENERGY_PER_MISSILE))
	    {
	      int xx = cos_table[p->rotation];
	      int yy = sin_table[p->rotation];

	      GameObject *m = &(game->missiles[game->next_missile_index++]);

	      player->energy -= ENERGY_PER_MISSILE;

	      if (game->next_missile_index == MAX_NUMBER_OF_MISSILES)
		{
		  game->next_missile_index = 0;
		}

	      m->p.x =
		p->x +
		(((SHIP_RADIUS +
		   MISSILE_RADIUS) / FIXED_POINT_SCALE_FACTOR) * xx);
	      m->p.y =
		p->y +
		(((SHIP_RADIUS +
		   MISSILE_RADIUS) / FIXED_POINT_SCALE_FACTOR) * yy);
	      m->p.vx = p->vx + (MISSILE_SPEED * xx);
	      m->p.vy = p->vy + (MISSILE_SPEED * yy);
	      m->p.rotation = p->rotation;
	      m->energy = MISSILE_TICKS_TO_LIVE;
	      m->primary_color = player->primary_color;
	      m->secondary_color = player->secondary_color;
	      m->is_alive = TRUE;
	      m->has_exploded = FALSE;

	      player->ticks_until_can_fire += TICKS_BETWEEN_FIRE;
	    }
	}
      else
	{
	  player->ticks_until_can_fire--;
	}
    }

  // apply velocity deltas to displacement
  apply_physics (p);
}

//------------------------------------------------------------------------------

static void
apply_physics (physics_t * p)
{
  p->x += p->vx;
  while (p->x > (WIDTH * FIXED_POINT_SCALE_FACTOR))
    {
      p->x -= (WIDTH * FIXED_POINT_SCALE_FACTOR);
    }
  while (p->x < 0)
    {
      p->x += (WIDTH * FIXED_POINT_SCALE_FACTOR);
    }

  p->y += p->vy;
  while (p->y > (HEIGHT * FIXED_POINT_SCALE_FACTOR))
    {
      p->y -= (HEIGHT * FIXED_POINT_SCALE_FACTOR);
    }
  while (p->y < 0)
    {
      p->y += (HEIGHT * FIXED_POINT_SCALE_FACTOR);
    }
}

//------------------------------------------------------------------------------

static gboolean
check_for_collision (physics_t * p1, physics_t * p2)
{
  int dx = (p1->x - p2->x) / FIXED_POINT_HALF_SCALE_FACTOR;
  int dy = (p1->y - p2->y) / FIXED_POINT_HALF_SCALE_FACTOR;
  int r = (p1->radius + p2->radius) / FIXED_POINT_HALF_SCALE_FACTOR;
  int d2 = (dx * dx) + (dy * dy);
  return (d2 < (r * r)) ? TRUE : FALSE;
}


static gboolean
check_for_ring_segment_collision (physics_t * ring, physics_t * p1)
{
  int dx = (p1->x - ring->x) / FIXED_POINT_HALF_SCALE_FACTOR;
  int dy = (p1->y - ring->y) / FIXED_POINT_HALF_SCALE_FACTOR;
  int r  = (ring->radius - 2*p1->radius) / FIXED_POINT_HALF_SCALE_FACTOR;
  int rr = (ring->radius * 0.75) / FIXED_POINT_HALF_SCALE_FACTOR;
  int d2 = (dx * dx) + (dy * dy);

  /*
  printf ("((%d - %d)^2 + (%d - %d)^2 = %d) < ((%d + %d)^2 = %d): %s\n",
          p1->x, ring->x, p1->y, ring->y, d2, p1->radius, ring->radius, r*r, (d2 < (r * r)) ? "TRUE" : "FALSE");
  printf ("%d: %d > %d: %s\n",
          rr, d2, rr*rr, (d2 > (rr * rr)) ? "TRUE" : "FALSE");
  */
  return (d2 < (r * r) && (d2 > (rr * rr)))? TRUE : FALSE;
}

static int
ring_segment_hit (GameObject *ring, GameObject *m)
{
    /* TODO: Calculate angle of m relative to ring */
    int dx = (m->p.x - ring->p.x);
    int dy = (m->p.y - ring->p.y);
    int rot = ring->p.rotation - arctan(dy, dx);
    //int rot = ring->p.rotation;

    /* TODO: Consider the current rotation of the ring */
    return abs(SEGMENTS_PER_RING * rot / NUMBER_OF_ROTATION_ANGLES);
}

//------------------------------------------------------------------------------

static void
enforce_minimum_distance (physics_t * ring, physics_t * p)
{
  int dx = ring->x - p->x;
  int dy = ring->y - p->y;
  double d2 = (((double) dx) * dx) + (((double) dy) * dy);
  int d = (int) sqrt (d2);
  int r = ring->radius + p->radius;

  // normalize dx and dy to length = ((r - d) / 2) + fudge_factor
  int desired_vector_length = ((r - d) * 5) / 8.0;

  dx *= desired_vector_length;
  dy *= desired_vector_length;
  dx /= d;
  dy /= d;

  p->x -= 2*dx;
  p->y -= 2*dy;
}

//------------------------------------------------------------------------------

static void
on_collision (GameObject * p, GameObject * m)
{
  p->energy -= DAMAGE_PER_MISSILE;
  p->is_hit = TRUE;
  m->has_exploded = TRUE;
  m->energy = MISSILE_EXPLOSION_TICKS_TO_LIVE;
  m->p.vx = 0;
  m->p.vy = 0;
}

static void
on_ring_segment_collision (GameObject * ring, GameObject * m, int segment)
{
    if (ring->energy & (1<<(2*segment))) {
        /*
        printf("Ring segment %d energy: 0x%.8o modified by 0x%.8o\n",
               segment, ring->energy, (1<<(2*segment)));
        */
        ring->energy &= ~(1<<(2*segment));
    } else if (ring->energy & (1<<(2*segment+1))) {
        /*
        printf("Secondary ring segment %d energy: 0x%.8o modified by 0x%.8o\n",
               segment, ring->energy, (1<<(2*segment+1)));
        */
        ring->energy &= ~(1<<(2*segment+1));
    } else {
        return;
    }
    /*
    printf("Ring energy: 0x%.8o\n", ring->energy);
    */
    printf("Ring segment %d hit\n", segment);
    ring->is_hit = TRUE;
    m->has_exploded = TRUE;
    m->energy = MISSILE_EXPLOSION_TICKS_TO_LIVE;
    m->p.vx = 0;
    m->p.vy = 0;

    if (ring->energy <= 0) {
        ring->is_alive = FALSE;
        // TODO:  If ring dead, create new one and regenerate mines
    }
}


//------------------------------------------------------------------------------

static void
show_text_message (cairo_t * cr, int font_size, int dy, const char *message)
{
  double x, y;
  cairo_text_extents_t extents;

  cairo_save (cr);

  cairo_select_font_face (cr, "Serif",
			  CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

  cairo_set_font_size (cr, font_size);
  cairo_text_extents (cr, message, &extents);
  x = (WIDTH / 2) - (extents.width / 2 + extents.x_bearing);
  y = (HEIGHT / 2) - (extents.height / 2 + extents.y_bearing);

  cairo_set_source_rgba (cr, 1, 1, 1, 1);
  cairo_move_to (cr, x, y + dy);
  cairo_show_text (cr, message);
  cairo_restore (cr);
}

//------------------------------------------------------------------------------

gint
on_key_event (GtkWidget * widget, GdkEventKey * event, gboolean key_is_on)
{
    return game->handle_key_event(widget, event, key_is_on);
}
