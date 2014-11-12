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

#include "config.h"

#include <err.h>
#include <popt.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/timeb.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "game.h"
#include "game-object.h"
#include "drawing.h"

// TODO:  Need to find best place for these...
void draw_star (cairo_t * cr, CanvasItem * item);
void init_trigonometric_tables (void);

static RGB_t color_red       = {0.9, 0.1, 0.4};
static RGB_t color_darkred   = {0.5, 0.1, 0.4};
static RGB_t color_blue      = {0.3, 0.3, 0.9};
static RGB_t color_darkblue  = {0.1, 0.1, 0.3};
//static RGB_t color_green     = {0.3, 0.9, 0.3};
//static RGB_t color_darkgreen = {0.1, 0.5, 0.3};

Game::Game(gint argc, gchar ** argv)
  : num_objects(0),
    number_of_rings(3),
    next_missile_index(0)
{
  gtk_init (&argc, &argv);
  process_options(argc, argv);
  init_trigonometric_tables ();

  canvas = new Canvas(WIDTH, HEIGHT);

  cannon = new GameObject;
  cannon->set_theme(color_red, color_darkred);

  player = new GameObject;
  player->set_theme(color_blue, color_darkblue);
  num_player_lives = 3;

  init();
}

Game::~Game()
{
  GameObject *o;
  for (; (o = objects[--num_objects]); )
    delete o;
  delete cannon;
  delete player;

  delete canvas;
}

void Game::init() {
  srand ((unsigned int) time (NULL));

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (window), "delete-event",
                    G_CALLBACK (gtk_main_quit), NULL);

  gtk_window_set_default_size (GTK_WINDOW (window), WIDTH, HEIGHT);

  g_signal_connect (G_OBJECT (window), "expose_event",
                    G_CALLBACK (on_expose_event), NULL);
  g_signal_connect (G_OBJECT (window), "key_press_event",
                    G_CALLBACK (on_key_press), NULL);
  g_signal_connect (G_OBJECT (window), "key_release_event",
                    G_CALLBACK (on_key_release), NULL);
  g_timeout_add (MILLIS_PER_FRAME, (GSourceFunc) on_timeout, window);

  level = 0;
  player->p.radius = SHIP_RADIUS;
  player->max_rotation_speed = 3;

  cannon->p.radius = CANNON_RADIUS;
  cannon->max_rotation_speed = 1;

  reset();
}

int Game::run() {
  gtk_widget_show_all (window);
  gtk_main ();

  return 0;
}

void Game::process_options(int argc, gchar ** argv) {
  int rc;
  poptContext pc;
  struct poptOption po[] = {
    /* TODO: Add game options here */
    POPT_AUTOHELP
    {NULL}
  };

  pc = poptGetContext(NULL, argc, (const char **)argv, po, 0);
  poptSetOtherOptionHelp(pc, "[ARG...]");
  poptReadDefaultConfig(pc, 0);

  while ((rc = poptGetNextOpt(pc)) >= 0);
  if (rc != -1) {
    // handle error
    switch(rc) {
      case POPT_ERROR_NOARG:
        errx(1, "Argument missing for an option\n");
      case POPT_ERROR_BADOPT:
        errx(1, "Unknown option or argument\n");
      case POPT_ERROR_BADNUMBER:
      case POPT_ERROR_OVERFLOW:
        errx(1, "Option could not be converted to number\n");
      default:
        errx(1, "Unknown error in option processing\n");
    }
  }
  //const char **remainder = poptGetArgs(pc);
}

void Game::reset() {
  for (int i=0; i<num_objects; i++) {
    objects[i]->init();
  }

  cannon->init();
  cannon->p.pos[0] = WIDTH / 2 * FIXED_POINT_SCALE_FACTOR;
  cannon->p.pos[1] = HEIGHT / 2 * FIXED_POINT_SCALE_FACTOR;
  cannon->p.rotation = random () % NUMBER_OF_ROTATION_ANGLES;
  cannon->p.rotation_speed = 0;
  cannon->p.rotation_accel = 0;
  cannon->ticks_until_can_fire = 0;
  cannon->energy = SHIP_MAX_ENERGY;
  cannon->is_hit = FALSE;
  cannon->draw_func = NULL;

  // Player is placed randomly in one of the four corner areas
  player->init();
  int x_quad = int(2 * random()/RAND_MAX);
  int y_quad = int(2 * random()/RAND_MAX);
  int margin = (HEIGHT + WIDTH)/40;
  player->p.pos[0] = (margin + (2*x_quad + random()/RAND_MAX) * (WIDTH-2*margin)/3.0) * FIXED_POINT_SCALE_FACTOR;
  player->p.pos[1] = (margin + (2*y_quad + random()/RAND_MAX) * (HEIGHT-2*margin)/3.0) * FIXED_POINT_SCALE_FACTOR;
  player->p.rotation = random () % NUMBER_OF_ROTATION_ANGLES;
  player->p.rotation_speed = 0;
  player->p.rotation_accel = 0;
  player->ticks_until_can_fire = 0;
  player->energy = SHIP_MAX_ENERGY;
  player->is_hit = FALSE;
  player->draw_func = NULL;

  message_timeout = 0;
  strncpy(main_message, "", 1);
  strncpy(second_message, "", 1);

  // Increase thickness of rings
  energy_per_segment = 1 + int(sqrt(level));

  // Add rings at higher levels
  number_of_rings = MIN(3 + int(level/4), MAX_NUMBER_OF_RINGS);
  printf("number_of_rings: %d\n", number_of_rings);

  // TODO: Implement speed of ring rotation and verify it varies by level
  // Increase rotation speed of rings
  ring_speed = 1 + (level % 3);

  // TODO: Implement homing mines
  number_of_homing_mines = MIN(number_of_homing_mines + level%2, MAX_NUMBER_OF_MINES);

  // TODO: Implement a forcefield
  // Cannon Forcefield
  if (level % 3 == 2) {
    cannon_forcefield_strength++;
  }

  // TODO: Implement stronger weapon power using cannon_weapon_strength
  if (level % 4 == 3) {
    cannon_weapon_strength++;
  }

  // TODO: Verify cannon rotates at different speeds each level
  // Increase rotational speed of cannon
  cannon->max_rotation_speed = 1 + level % 4;

  // TODO: Implement weapon count
  // Cannon weaponry
  cannon_weapon_count = level % 4;

  // TODO: Implement graphic
  // Add gravitational attraction
  switch (level % 5) {
    case 0:  gravity_x *= -1;                           break;
    case 1:  gravity_y = gravity_x;                     break;
    case 2:  gravity_x *= -1;                           break;
    case 3:  gravity_y = int(level / 5); gravity_x = 0; break;
    case 4:  gravity_x = gravity_y;      gravity_y = 0; break;
  }

  if (level % 6 == 5) {
    cannon_weapon_strength++;
  }

  if (level % 7 == 6) {
    cannon_forcefield_repulsion++;
  }
  if (level > 4) {
    // TODO:  On advanced levels, take into account the speed the player
    // is going, and try to lead him a bit.
  }

  if (level > 8) {
    // Make cannon smarter
    // TODO:  Selectively shoot out one inner ring segment
    // TODO:  If only 2 segments left in outer layer, shoot them
  }

  init_rings_array ();
  init_stars_array ();
  init_missiles_array ();
}

int Game::addObject(GameObject* o) {
  if (num_objects >= MAX_OBJECTS)
    return 1;

  objects[num_objects++] = o;
  return 0;
}

void
Game::checkConditions() {
  if (strlen(main_message) < 1)
  {
    if (!cannon->is_alive())
      advance_level();

    else if (num_player_lives <= 0)
      game_over();

    else if (!player->is_alive())
      try_again();
  }
}

void Game::drawWorld(cairo_t *cr) {
  /* draw background space color */
  cairo_set_source_rgb (cr, 0.1, 0.0, 0.1);
  cairo_paint (cr);

  // draw any stars...
  for (int i = 0; i < NUMBER_OF_STARS; i++) {
    game->stars[i].draw(cr);
  }
}

void Game::drawUI(cairo_t *cr) {
  // ... the energy bars...
  printf("energy: player=%d, cannon=%d, width=%d\n", player->energy, cannon->energy, WIDTH);
  // TODO: Use cannon->max_energy instead of SHIP_MAX_ENERGY
  draw_energy_bar (cr, 10, 10,
                   (100 * cannon->energy) / SHIP_MAX_ENERGY,
                   color_red, color_darkred);
  draw_energy_bar (cr, WIDTH - 210, 10,   // TODO: Use const instead of 200
                   (100 * player->energy) / SHIP_MAX_ENERGY,
                   color_blue, color_darkblue);

  if (strlen(main_message)>0 && message_timeout != 0)
  {
    show_text_message (cr, 80, -30, main_message,
                       MIN(1.0, (message_timeout%200) / 100.0) );
    if (strlen(second_message)>0)
      show_text_message (cr, 30, +40, second_message, 1.0);
    if (message_timeout>0)
      message_timeout--;
  }

}

void Game::drawShip(cairo_t *cr) {
  cairo_save (cr);
  cairo_translate (cr, cannon->p.pos[0] / FIXED_POINT_SCALE_FACTOR,
                   cannon->p.pos[1] / FIXED_POINT_SCALE_FACTOR);
  cairo_rotate (cr, cannon->p.rotation * RADIANS_PER_ROTATION_ANGLE);
  this->drawCannon (cr, cannon);
  cairo_restore (cr);

  cairo_save (cr);
  cairo_translate (cr, player->p.pos[0] / FIXED_POINT_SCALE_FACTOR,
                   player->p.pos[1] / FIXED_POINT_SCALE_FACTOR);
  cairo_rotate (cr, player->p.rotation * RADIANS_PER_ROTATION_ANGLE);
  draw_ship_body (cr, player);
  cairo_restore (cr);
}

void Game::drawCannon(cairo_t *cr, GameObject *player) {
  draw_cannon (cr, player);
}

void Game::drawRings(cairo_t *cr) {
  for (int i = 0; i < number_of_rings; i++) {
    if (rings[i].is_alive())
    {
      cairo_save (cr);
      cairo_translate (cr,
                       rings[i].p.pos[0] / FIXED_POINT_SCALE_FACTOR,
                       rings[i].p.pos[1] / FIXED_POINT_SCALE_FACTOR);
      cairo_rotate (cr,
                    -1 * rings[i].p.rotation * RADIANS_PER_ROTATION_ANGLE
                    - PI/2.0);

      cairo_set_source_rgba (cr, 2-i, i? 1.0/i : 0, 0, 0.6);

      draw_ring (cr, &(rings[i]));
      cairo_restore (cr);
    } else {
      printf("ring %d is dead\n", i);
    }
  }
}

void Game::drawMissiles(cairo_t *cr) {
  for (int i = 0; i < MAX_NUMBER_OF_MISSILES; i++)
  {
    if (missiles[i].is_alive())
    {
      cairo_save (cr);
      cairo_translate (cr, missiles[i].p.pos[0] / FIXED_POINT_SCALE_FACTOR,
                       missiles[i].p.pos[1] / FIXED_POINT_SCALE_FACTOR);
      cairo_rotate (cr,
                    missiles[i].p.rotation * RADIANS_PER_ROTATION_ANGLE);
      draw_missile (cr, &(missiles[i]));
      cairo_restore (cr);
    }
  }
}

void Game::drawMines(cairo_t *cr) {
  // TODO
}

//------------------------------------------------------------------------------
static int number_of_frames = 0;
static long millis_taken_for_frames = 0;

static long
get_time_millis (void)
{
  struct timeb tp;
  ftime (&tp);
  return (long) ((tp.time * 1000) + tp.millitm);
}

gint
on_expose_event (GtkWidget * widget, GdkEventExpose * event)
{
  cairo_t *cr = gdk_cairo_create (widget->window);
  int width = widget->allocation.width;
  int height = widget->allocation.height;
  long start_time = 0;

  if (game->show_fps)
    start_time = get_time_millis ();

  game->canvas->scale_for_aspect_ratio(cr, width, height);

  game->checkConditions();
  game->drawWorld(cr);

  // Draw game elements
  game->drawShip(cr);
  game->drawMissiles(cr);
  game->drawRings(cr);
  game->drawMines(cr);
  game->drawUI(cr);

  cairo_restore (cr);

  if (game->show_fps)
  {
    number_of_frames++;
    millis_taken_for_frames += get_time_millis () - start_time;
    if (number_of_frames >= 100)
    {
      double fps =
        1000.0 * ((double) number_of_frames) /
        ((double) millis_taken_for_frames);
      dbg ("%d frames in %ldms (%.3ffps)\n", number_of_frames,
           millis_taken_for_frames, fps);
      number_of_frames = 0;
      millis_taken_for_frames = 0L;
    }
  }

  cairo_destroy (cr);

  return TRUE;
}

gint
on_key_press (GtkWidget * widget, GdkEventKey * event)
{
  return on_key_event (widget, event, TRUE);
}

gint
on_key_release (GtkWidget * widget, GdkEventKey * event)
{
  return on_key_event (widget, event, FALSE);
}

gint
Game::handle_key_event (GtkWidget * widget, GdkEventKey * event, gboolean key_is_on)
{
  switch (event->keyval)
  {
    case GDK_Tab:
      if (!key_is_on)
        advance_level();
      break;

    case GDK_Escape:
      gtk_main_quit();
      break;

    case GDK_Return:
      if (strlen(main_message)>0)
      {
        level = 0;
        reset();
      }
      break;

    case GDK_bracketleft:
      if (key_is_on)
      {
        canvas->debug_scale_factor /= 1.25f;
        dbg ("Scale: %f\n", canvas->debug_scale_factor);
      }
      break;
    case GDK_bracketright:
      if (key_is_on)
      {
        canvas->debug_scale_factor *= 1.25f;
        dbg ("Scale: %f\n", canvas->debug_scale_factor);
      }
      break;

    case GDK_Left:
    case GDK_KP_Left:
      if (!key_is_on)
        player->p.rotation_accel = 0;
      else if (player->p.rotation_accel > -100)
        player->p.rotation_accel -= 10;
      break;
    case GDK_Right:
    case GDK_KP_Right:
      if (!key_is_on)
        player->p.rotation_accel = 0;
      else if (player->p.rotation_accel < 100)
        player->p.rotation_accel += 10;
      break;
    case GDK_Up:
    case GDK_KP_Up:
      player->is_thrusting = key_is_on;
      break;
    case GDK_Down:
    case GDK_KP_Down:
      //player->is_reversing = key_is_on;
      break;
    case GDK_space:
    case GDK_Control_R:
    case GDK_Control_L:
    case GDK_KP_Insert:
      player->is_firing = key_is_on;
      break;
  }
  return TRUE;
}


void
Game::init_missiles_array ()
{
  for (int i = 0; i < MAX_NUMBER_OF_MISSILES; i++)
  {
    missiles[i].p.radius = MISSILE_RADIUS;
    missiles[i].energy = -1;
  }
}

void
Game::init_rings_array ()
{
  int rot = 1;
  for (int i=0; i < number_of_rings; i++)
  {
    rings[i].pos[0] = WIDTH / 2;
    rings[i].pos[1] = HEIGHT / 2;
    rings[i].p.pos[0] = WIDTH / 2 * FIXED_POINT_SCALE_FACTOR;
    rings[i].p.pos[1] = HEIGHT / 2 * FIXED_POINT_SCALE_FACTOR;
    rings[i].scale = i;
    rings[i].energy = SEGMENTS_PER_RING;
    rings[i].max_rotation_speed = rot;
    rings[i].p.rotation_speed = rot;
    rot *= -1;
    for (int j=0; j<SEGMENTS_PER_RING; j++) {
      rings[i].component_energy[j] = energy_per_segment;
    }

    rings[i].primary_color.r = 0.3;
    rings[i].primary_color.g = 1.0;
    rings[i].primary_color.b = 0.9;
    rings[i].secondary_color.r = 0.1;
    rings[i].secondary_color.g = 1.0;
    rings[i].secondary_color.b = 0.3;
  }
  rings[0].p.radius = SHIELD_OUTER_RADIUS;
  rings[1].p.radius = SHIELD_MIDDLE_RADIUS;
  rings[2].p.radius = SHIELD_INNER_RADIUS;
}

void
Game::init_stars_array ()
{
  int i;

  for (i = 0; i < NUMBER_OF_STARS; i++)
  {
    stars[i].pos[0] = random () % WIDTH;
    stars[i].pos[1] = random () % HEIGHT;
    stars[i].rotation = drand48 () * TWO_PI;
    stars[i].scale = 0.5 + (drand48 ());
    stars[i].draw_func = draw_star;
  }
}

//------------------------------------------------------------------------------
// TODO: Move strings into headers as constants
//       Maybe turn the messages + data into objects
void
Game::game_over()
{
  snprintf(main_message, sizeof(main_message), "Game Over");
  snprintf(second_message, sizeof(main_message), "Press [ENTER] for new game");
  message_timeout = -1;
}

void
Game::try_again()
{
  num_player_lives--;
  snprintf(main_message, sizeof(main_message), "%d lives remainig", level);
  snprintf(second_message, sizeof(main_message), "Press [ENTER] to engage");
  message_timeout = 1000;
}

void
Game::advance_level()
{
  level++;
  reset();
  snprintf(main_message, sizeof(main_message), "Level %d", level);
  snprintf(second_message, sizeof(main_message), "Press [ENTER] to engage");
  message_timeout = 100;
}


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

  operate_cannon (game->cannon, game->player, &(game->rings[game->number_of_rings-1]));
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
    game->player->is_hit = TRUE;
    game->player->p.vel[0] = (p1vx * +5 / 8) + (p2vx * -2 / 8);
    game->player->p.vel[1] = (p1vy * +5 / 8) + (p2vy * -2 / 8);
  }

  for (i = 0; i < MAX_NUMBER_OF_MISSILES; i++)
  {
    if (game->missiles[i].is_alive())
    {
      apply_physics (&(game->missiles[i].p));

      if (!game->missiles[i].has_exploded)
      {
        /* Foreach ring segment, check collision */
        for (j = game->number_of_rings; j >= 0; j--) {
          if (!game->rings[j].is_alive())
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
    }
  }

  for (i = 0; i < game->number_of_rings; i++)
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
  }
  else
  {
    game->cannon->energy = MIN (SHIP_MAX_ENERGY, game->cannon->energy + 3);
  }

  if (game->player->energy <= 0)
  {
    game->player->energy = 0;
  }
  else
  {
    game->player->energy = MIN (SHIP_MAX_ENERGY, game->player->energy + 1);
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
  cannon->p.rotation_speed = -1 * cannon->max_rotation_speed;
}

static void
turn_cannon_right (GameObject *cannon)
{
  cannon->p.rotation_speed = 1 * cannon->max_rotation_speed;
}

static void
turn_cannon_stop (GameObject *cannon)
{
  cannon->p.rotation_speed = 0;
}


static void
operate_cannon (GameObject * cannon, GameObject * player, GameObject *ring)
{
  int direction;

  physics_t *c = &(cannon->p);
  physics_t *p = &(player->p);

  if (! player->is_alive()) {
    /* TODO:  Reset */
    cannon->is_firing = FALSE;
    return;
  }

  direction = arctan ( p->pos[1] - c->pos[1], p->pos[0] - c->pos[0] );

  if (direction == c->rotation) {
    // What segment would we hit if we fired?
    int seg_no = ring_segment_by_rotation(ring, c->rotation);

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

}

static void
apply_physics_to_player (GameObject * player)
{
  int v2, m2;
  physics_t *p = &(player->p);

  if (player->is_alive())
  {
    // Apply any accelerational impulses
    if (p->rotation_accel != 0.0) {
      p->rotation_speed += p->rotation_accel / 10.0;
      p->rotation_accel /= 4.0;
    }

    // Apply any rotations
    p->rotation += player->p.rotation_speed;
    while (p->rotation < 0)
      p->rotation += NUMBER_OF_ROTATION_ANGLES;

    while (p->rotation >= NUMBER_OF_ROTATION_ANGLES)
      p->rotation -= NUMBER_OF_ROTATION_ANGLES;

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
