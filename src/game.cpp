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

#include <err.h>
#include <popt.h>
#include <math.h>
#include <stdlib.h>
#include <sys/timeb.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "game.h"
#include "game-config.h"
#include "drawing.h"

// TODO:  Need to find best place for these...
void draw_star (cairo_t * cr, CanvasItem * item);
void draw_energy_bar (cairo_t * cr, GameObject * item);
void init_trigonometric_tables (void);

static RGB_t color_red      = {0.9, 0.2, 0.3};
static RGB_t color_darkred  = {0.5, 0.2, 0.3};
static RGB_t color_blue     = {0.3, 0.5, 0.9};
static RGB_t color_darkblue = {0.1, 0.3, 0.3};

Game::Game(gint argc, gchar ** argv)
  : num_objects(0), num_player_lives(3), next_missile_index(0),
    number_of_rings(3)
{
  gtk_init (&argc, &argv);
  process_options(argc, argv);

  init_trigonometric_tables ();

  canvas = new Canvas(WIDTH, HEIGHT);

  cannon = new GameObject;
  cannon->set_theme(color_blue, color_darkblue);

  cannon_status = new GameObject;
  cannon_status->set_theme(color_blue, color_darkblue);

  player = new GameObject;
  player->set_theme(color_red, color_darkred);

  player_status = new GameObject;
  player_status->set_theme(color_red, color_darkred);
  init();
}

Game::~Game()
{
  GameObject *o;
  for (; (o = objects[--num_objects]); )
    delete o;
  delete cannon;
  delete cannon_status;
  delete player;
  delete player_status;

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

  cannon_status->init();
  cannon_status->pos = Point(0, 0);
  cannon_status->rotation = 0;
  cannon_status->energy = CANNON_MAX_ENERGY;
  cannon_status->draw_func = (canvas_item_draw) draw_energy_bar;

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

  player_status->init();
  player_status->pos = Point( WIDTH - SHIP_MAX_ENERGY/5, 0);
  player_status->rotation = PI;
  player_status->energy = SHIP_MAX_ENERGY;
  player_status->draw_func = (canvas_item_draw) draw_energy_bar;

  main_message = NULL;
  second_message = NULL;
  message_timeout = 0;

  // Increase thickness of rings
  energy_per_segment = 1 + int(sqrt(level));

  // Add rings at higher levels
  number_of_rings = MIN(3 + int(level/4), MAX_NUMBER_OF_RINGS);
  printf("number_of_rings: %d\n", number_of_rings);

  // Increase rotation speed of rings
  ring_speed = 1 + (level % 3);

  number_of_homing_mines = MIN(number_of_homing_mines + level%2, MAX_NUMBER_OF_MINES);

  // Cannon Forcefield
  if (level % 3 == 1) {
    cannon_forcefield_strength++;
  }

  // Increase rotational speed of cannon
  cannon->max_rotation_speed = 1 + level % 4;

  // Cannon weaponry
  cannon_weapon_count = level % 4;

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
    // + Lead the player's ship when firing
    // TODO:  On advanced levels, take into account the speed the player
    // is going, and try to lead him a bit.
  }

  if (level > 8) {
    // Make cannon smarter
    // + Selectively shoot out one inner ring segment
    // + If only 2 segments left in outer layer, shoot them
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
  if (main_message == NULL)
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
  cannon_status->draw_func(cr, cannon_status);
  player_status->draw_func(cr, player_status);

  if (main_message != NULL && message_timeout != 0)
  {
    show_text_message (cr, 80, -30, main_message,
                       MIN(1.0, (message_timeout%200) / 100.0) );
    if (second_message != NULL)
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
  printf("%d rings\n", number_of_rings);
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
      advance_level();
      break;

    case GDK_Escape:
      gtk_main_quit();
      break;

    case GDK_Return:
      if (main_message != NULL)
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
  main_message = "Game Over";
  second_message = "Press [ENTER] for new game";
  message_timeout = -1;
}

void
Game::try_again()
{
  num_player_lives--;
  main_message = "Try again!!!";
  second_message = NULL;
  message_timeout = 1000;
}

void
Game::advance_level()
{
  level++;
  this->reset();
  main_message = "Next Level!";
  second_message = "Press [ENTER] to start";
  message_timeout = 100;
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
