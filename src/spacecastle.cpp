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

// 2010-05-05: 1000 lines
// 2010-05-06:  939
// 2010-05-30:  609

#include <stdio.h>
#include <stdlib.h>
#include <math.h>  // For sqrt()
#include <gtk/gtk.h>

#include "game-config.h"
#include "game-object.h"
#include "game.h"

//------------------------------------------------------------------------------
// Forward definitions of functions

static void operate_cannon (GameObject *cannon, GameObject *player, GameObject *ring);
static void apply_physics (physics_t *);
static void apply_physics_to_player (GameObject *player);
static gboolean check_for_collision (physics_t *, physics_t *);
static gboolean check_for_ring_collision (physics_t *, physics_t *);
static int ring_segment_by_rotation (GameObject *ring, int rot);
static void enforce_minimum_distance (physics_t *, physics_t *);
static void on_collision (GameObject *player, GameObject *missile);
static int ring_segment_hit (GameObject *ring, GameObject *missile);
static void on_ring_segment_collision (GameObject * ring, GameObject * m, int segment);
gint on_key_event (GtkWidget *, GdkEventKey *, gboolean);
gint on_timeout (gpointer);

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

  operate_cannon (game->cannon, game->player, &(game->rings[MAX_NUMBER_OF_RINGS-1]));
  apply_physics_to_player (game->cannon);
  apply_physics_to_player (game->player);

  if (check_for_collision (&(game->rings[0].p), &(game->player->p)))
    {
      int p1vx, p1vy, p2vx, p2vy;
      int dvx, dvy, dv2;
      int damage;

      enforce_minimum_distance (&(game->rings[0].p), &(game->player->p));

      p1vx = game->rings[0].p.vel[0];
      p1vy = game->rings[0].p.vel[1];
      p2vx = game->player->p.vel[0];
      p2vy = game->player->p.vel[1];

      dvx = (p1vx - p2vx) / FIXED_POINT_HALF_SCALE_FACTOR;
      dvy = (p1vy - p2vy) / FIXED_POINT_HALF_SCALE_FACTOR;
      dv2 = (dvx * dvx) + (dvy * dvy);
      damage = ((int)(sqrt (dv2))) / DAMAGE_PER_SHIP_BOUNCE_DIVISOR;

      game->player->energy -= damage;
      game->player_status->energy = game->player->energy;
      game->player->is_hit = TRUE;
      game->player->p.vel[0] = (p1vx * +5 / 8) + (p2vx * -2 / 8);
      game->player->p.vel[1] = (p1vy * +5 / 8) + (p2vy * -2 / 8);
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
          on_collision (game->cannon, &(game->missiles[i]));


          if (check_for_collision (&(game->missiles[i].p), &(game->player->p)))
          on_collision (game->player, &(game->missiles[i]));

        }

      game->missiles[i].energy--;
      if (game->missiles[i].energy <= 0)
          game->missiles[i].is_alive = FALSE;
    }
    }

  for (i = 0; i < MAX_NUMBER_OF_RINGS; i++)
  {
      game->rings[i].p.rotation =
          (game->rings[i].p.rotation + game->rings[i].p.rotation_speed)
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

// TODO: These three routines are temporary until I've refactored away all
//  need for is_turning_*
static void
turn_cannon_left (GameObject *cannon)
{
  cannon->is_turning_left = TRUE;
  cannon->is_turning_right = FALSE;
  cannon->p.rotation_speed = -1;
}

static void
turn_cannon_right (GameObject *cannon)
{
  cannon->is_turning_left = FALSE;
  cannon->is_turning_right = TRUE;
  cannon->p.rotation_speed = 1;
}

static void
turn_cannon_stop (GameObject *cannon)
{
  cannon->is_turning_left = FALSE;
  cannon->is_turning_right = FALSE;
  cannon->p.rotation_speed = 0;
}


static void
operate_cannon (GameObject * cannon, GameObject * player, GameObject *ring)
{
    int direction;

    physics_t *c = &(cannon->p);
    physics_t *p = &(player->p);

    if (! player->is_alive) {
        /* TODO:  Reset */
        cannon->is_firing = FALSE;
        return;
    }

    direction = arctan ( p->pos[1] - c->pos[1], p->pos[0] - c->pos[0] );

    if (direction == c->rotation) {
        // What segment would we hit if we fired?
        int seg_no = ring_segment_by_rotation(ring, c->rotation);
        dbg("I would hit segment %d\n", seg_no);

        // TODO: If rotation angle is such that our missile
        //   would likely hit a ring segment, don't shoot.

        // Don't shoot if it'd just hurt our ring shield
        if (ring->component_energy[seg_no] > 0) {
            gboolean ring_is_undamaged = true;
            for (int seg=0; seg<SEGMENTS_PER_RING; seg++) {
                if (ring->component_energy[seg] <= 0) {
                    ring_is_undamaged = false;
                }
            }

            // However, if no segments destroyed yet, shoot one if level > 1
            if (ring_is_undamaged) {
                cannon->is_firing = TRUE;
                turn_cannon_stop (cannon);
            }

        } else {
            cannon->is_firing = TRUE;
            turn_cannon_stop (cannon);
        }
    } else if (c->rotation - direction == NUMBER_OF_ROTATION_ANGLES/2) {
        cannon->is_firing = FALSE;
        // Stay going in same direction
    } else if (c->rotation - direction < NUMBER_OF_ROTATION_ANGLES/2
               && c->rotation - direction > 0) {
        turn_cannon_left (cannon);
        cannon->is_firing = FALSE;

    } else if (direction - c->rotation > NUMBER_OF_ROTATION_ANGLES/2
               && c->rotation - direction < 0) {
        turn_cannon_left (cannon);
        cannon->is_firing = FALSE;

    } else {
        cannon->is_firing = FALSE;
        turn_cannon_right (cannon);
    }
    if (cannon->is_turning_right)
      cannon->p.rotation_speed = 1;
    else if (cannon->is_turning_left)
      cannon->p.rotation_speed = -1;

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
      p->rotation += player->p.rotation_speed;
      while (p->rotation < 0)
                p->rotation += NUMBER_OF_ROTATION_ANGLES;
    }

      // ... or right.
      if (player->is_turning_right)
    {
      p->rotation += player->p.rotation_speed;
      while (p->rotation >= NUMBER_OF_ROTATION_ANGLES)
                p->rotation -= NUMBER_OF_ROTATION_ANGLES;
    }

      // check if accelerating
      if (player->is_thrusting)
    {
      p->vel[0] += SHIP_ACCELERATION_FACTOR * cos_table[p->rotation];
      p->vel[1] += SHIP_ACCELERATION_FACTOR * sin_table[p->rotation];
    }

    // check if reversing
      if (player->is_reversing)
    {
      p->vel[0] -= SHIP_ACCELERATION_FACTOR * cos_table[p->rotation];
      p->vel[1] -= SHIP_ACCELERATION_FACTOR * sin_table[p->rotation];
    }

      // apply velocity upper bound
      v2 = ((p->vel[0]) * (p->vel[0])) + ((p->vel[1]) * (p->vel[1]));
      m2 = SHIP_MAX_VELOCITY * SHIP_MAX_VELOCITY;
      if (v2 > m2)
    {
      p->vel[0] = (int) (((double) (p->vel[0]) * m2) / v2);
      p->vel[1] = (int) (((double) (p->vel[1]) * m2) / v2);
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
          game->next_missile_index = 0;

          m->p.pos[0] =
        p->pos[0] +
        (((SHIP_RADIUS +
           MISSILE_RADIUS) / FIXED_POINT_SCALE_FACTOR) * xx);
          m->p.pos[1] =
        p->pos[1] +
        (((SHIP_RADIUS +
           MISSILE_RADIUS) / FIXED_POINT_SCALE_FACTOR) * yy);
          m->p.vel[0] = p->vel[0] + (MISSILE_SPEED * xx);
          m->p.vel[1] = p->vel[1] + (MISSILE_SPEED * yy);
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
  p->pos[0] += p->vel[0];
  while (p->pos[0] > (WIDTH * FIXED_POINT_SCALE_FACTOR))
    p->pos[0] -= (WIDTH * FIXED_POINT_SCALE_FACTOR);
  while (p->pos[0] < 0)
    p->pos[0] += (WIDTH * FIXED_POINT_SCALE_FACTOR);

  p->pos[1] += p->vel[1];
  while (p->pos[1] > (HEIGHT * FIXED_POINT_SCALE_FACTOR))
      p->pos[1] -= (HEIGHT * FIXED_POINT_SCALE_FACTOR);
  while (p->pos[1] < 0)
      p->pos[1] += (HEIGHT * FIXED_POINT_SCALE_FACTOR);
}

//------------------------------------------------------------------------------

static gboolean
check_for_collision (physics_t * p1, physics_t * p2)
{
  int dx = (p1->pos[0] - p2->pos[0]) / FIXED_POINT_HALF_SCALE_FACTOR;
  int dy = (p1->pos[1] - p2->pos[1]) / FIXED_POINT_HALF_SCALE_FACTOR;
  int r = (p1->radius + p2->radius) / FIXED_POINT_HALF_SCALE_FACTOR;
  int d2 = (dx * dx) + (dy * dy);
  return (d2 < (r * r)) ? TRUE : FALSE;
}


static gboolean
check_for_ring_collision (physics_t * ring, physics_t * p1)
{
  int dx = (p1->pos[0] - ring->pos[0]) / FIXED_POINT_HALF_SCALE_FACTOR;
  int dy = (p1->pos[1] - ring->pos[1]) / FIXED_POINT_HALF_SCALE_FACTOR;
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


// TODO: Work on converting Point from here...
static int
ring_segment_hit (GameObject *ring, GameObject *m)
{
    /* Calculate angle of missile compared with ring center */
    int dx = (m->p.pos[0] - ring->p.pos[0]);
    int dy = (m->p.pos[1] - ring->p.pos[1]);
    int rot = arctan(dy, dx);

    return ring_segment_by_rotation(ring, rot);
}

//------------------------------------------------------------------------------

static void
enforce_minimum_distance (physics_t * ring, physics_t * p)
{
  int dx = ring->pos[0] - p->pos[0];
  int dy = ring->pos[1] - p->pos[1];
  double d2 = (((double) dx) * dx) + (((double) dy) * dy);
  int d = (int) sqrt (d2);
  int r = ring->radius + p->radius;

  // normalize dx and dy to length = ((r - d) / 2) + fudge_factor
  int desired_vector_length = ((r - d) * 5) / 8.0;

  dx *= desired_vector_length;
  dy *= desired_vector_length;
  dx /= d;
  dy /= d;

  p->pos[0] -= 2*dx;
  p->pos[1] -= 2*dy;
}

//------------------------------------------------------------------------------

static void
on_collision (GameObject * p, GameObject * m)
{
  p->energy -= DAMAGE_PER_MISSILE;
  p->is_hit = TRUE;
  m->has_exploded = TRUE;
  m->energy = MISSILE_EXPLOSION_TICKS_TO_LIVE;
  m->p.vel[0] = 0;
  m->p.vel[1] = 0;
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
    m->p.vel[0] = 0;
    m->p.vel[1] = 0;

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
