// SVG Spacewar is copyright 2005 by Nigel Tao: nigel.tao@myrealbox.com
// Licenced under the GNU GPL.
// Developed on cairo version 0.4.0.
//
// 2005-03-31: Version 0.1.

// 2010-05-05: 1000 lines
// 2010-05-06:  939
// 2010-05-30:  609

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>

#include "canvas.h"
#include "game-config.h"
#include "game-object.h"
#include "game-math.h"
#include "drawing.h"
#include "game.h"

//------------------------------------------------------------------------------
// Forward definitions of functions

static void operate_cannon (GameObject *cannon, GameObject *player);
static void apply_physics (physics_t *);
static void apply_physics_to_player (GameObject *player);
static gboolean check_for_collision (physics_t *, physics_t *);
static gboolean check_for_ring_collision (physics_t *, physics_t *);
static void enforce_minimum_distance (physics_t *, physics_t *);
static long get_time_millis (void);
static void on_collision (GameObject *player, GameObject *missile);
static int ring_segment_hit (GameObject *ring, GameObject *missile);
static void on_ring_segment_collision (GameObject * ring, GameObject * m, int segment);
gint on_expose_event (GtkWidget *, GdkEventExpose *);
gint on_key_event (GtkWidget *, GdkEventKey *, gboolean);
gint on_timeout (gpointer);

//------------------------------------------------------------------------------

static gboolean show_fps = TRUE;
static int number_of_frames = 0;
static long millis_taken_for_frames = 0;

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
  int width = widget->allocation.width;
  int height = widget->allocation.height;
  int i;
  long start_time = 0;
  if (show_fps)
    {
      start_time = get_time_millis ();
    }

  game->canvas->scale_for_aspect_ratio(cr, width, height);

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
  draw_cannon (cr, game->cannon);
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
	  cairo_translate (cr,
                           game->rings[i].p.x / FIXED_POINT_SCALE_FACTOR,
			   game->rings[i].p.y / FIXED_POINT_SCALE_FACTOR);
	  cairo_rotate (cr,
                        -1 * game->rings[i].p.rotation * RADIANS_PER_ROTATION_ANGLE
                        - PI/2.0);

          cairo_set_source_rgba (cr, 2-i, i? 1.0/i : 0, 0, 0.6);

	  draw_ring (cr, &(game->rings[i]));
	  cairo_restore (cr);
	}
    }

  // TODO:  Finally, the space mines

  if (game->game_over_message == NULL)
  {
      if (!game->cannon->is_alive) {
          /* Bonus life */
          game->num_player_lives++;
          game->game_over_message = "Next Level!";
          game->advance_level();
      }
      if (!game->player->is_alive)
      {
          game->num_player_lives--;
          game->game_over_message = "Try again!!!";
      }

      if (game->num_player_lives <= 0)
      {
          game->game_over_message = "Game Over";
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
              /* Foreach ring segment, check collision */
              for (j = MAX_NUMBER_OF_RINGS; j >= 0; j--) {
                  if (!game->rings[j].is_alive)
                      continue;

                  if (check_for_ring_collision (&(game->rings[j].p),
                                                &(game->missiles[i].p)))
                  {
                      int segment = ring_segment_hit(&(game->rings[j]),
                                                     &(game->missiles[i]));

                      on_ring_segment_collision (&(game->rings[j]),
                                                 &(game->missiles[i]),
                                                 segment);
                  }
              }
	      if (check_for_collision (&(game->missiles[i].p), &(game->cannon->p)))
		{
		  on_collision (game->cannon, &(game->missiles[i]));
		}

	      if (check_for_collision (&(game->missiles[i].p), &(game->player->p)))
		{
		  on_collision (game->player, &(game->missiles[i]));
		}
	    }

	  game->missiles[i].energy--;
	  if (game->missiles[i].energy <= 0)
	    {
	      game->missiles[i].is_alive = FALSE;
	    }
	}
    }

  for (i = 0; i < MAX_NUMBER_OF_RINGS; i++)
  {
      game->rings[i].p.rotation = 
          (game->rings[i].p.rotation + game->rings[i].rotation_speed)
          % NUMBER_OF_ROTATION_ANGLES;

      if (game->rings[i].p.rotation < 0)
          game->rings[i].p.rotation += NUMBER_OF_ROTATION_ANGLES;
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
        // What segment would we hit if we fired?
        // What energy level is the segment?
        // If no segments destroyed yet, shoot one if level > 1
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
	  p->rotation -= player->rotation_speed;
	  while (p->rotation < 0)
	    {
                p->rotation += NUMBER_OF_ROTATION_ANGLES;
	    }
	}

      // ... or right.
      if (player->is_turning_right)
	{
	  p->rotation += player->rotation_speed;
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
check_for_ring_collision (physics_t * ring, physics_t * p1)
{
  int dx = (p1->x - ring->x) / FIXED_POINT_HALF_SCALE_FACTOR;
  int dy = (p1->y - ring->y) / FIXED_POINT_HALF_SCALE_FACTOR;
  int r  = (ring->radius + p1->radius) / FIXED_POINT_HALF_SCALE_FACTOR;
  int rr = (ring->radius * 0.8) / FIXED_POINT_HALF_SCALE_FACTOR;
  int d2 = (dx * dx) + (dy * dy);

  return (d2 < (r * r) && (d2 > (rr * rr)))? TRUE : FALSE;
}

static int
ring_segment_by_rotation (GameObject *ring, int rot)
{
    /* Account for the current rotation of the ring */
    int angle_ring_hit = (rot + ring->p.rotation)
        % NUMBER_OF_ROTATION_ANGLES;

    /* Divide angle by arc length of a segment */
    return angle_ring_hit / (NUMBER_OF_ROTATION_ANGLES / SEGMENTS_PER_RING);
}

static int
ring_segment_hit (GameObject *ring, GameObject *m)
{
    /* Calculate angle of missile compared with ring center */
    int dx = (m->p.x - ring->p.x);
    int dy = (m->p.y - ring->p.y);
    int rot = arctan(dy, dx);

    return ring_segment_by_rotation(ring, rot);
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
    if (ring->component_energy[segment] <= 0)
        return;

    ring->component_energy[segment]--;

    ring->is_hit = TRUE;
    m->has_exploded = TRUE;
    m->energy = MISSILE_EXPLOSION_TICKS_TO_LIVE;
    m->p.vx = 0;
    m->p.vy = 0;

    if (ring->component_energy[segment] <= 0)
        ring->energy--;

    if (ring->energy <= 0) {
        ring->is_alive = FALSE;
        // TODO:  If ring dead, create new one and regenerate mines

        // Mark rings are in transition
        // Each tick, increase radius by 1
        // until it equals the next higher level
        // Then move ring[N] to ring[N-1]
    }
}



//------------------------------------------------------------------------------

gint
on_key_event (GtkWidget * widget, GdkEventKey * event, gboolean key_is_on)
{
    return game->handle_key_event(widget, event, key_is_on);
}
