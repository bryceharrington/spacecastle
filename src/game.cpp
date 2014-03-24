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

#include <popt.h>
#include <err.h>
#include <sys/timeb.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "game.h"
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
    : num_objects(0), num_player_lives(3), next_missile_index(0)
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
    cannon->p.x = WIDTH / 2 * FIXED_POINT_SCALE_FACTOR;
    cannon->p.y = HEIGHT / 2 * FIXED_POINT_SCALE_FACTOR;
    cannon->p.rotation = random () % NUMBER_OF_ROTATION_ANGLES;
    cannon->p.radius = CANNON_RADIUS;
    cannon->ticks_until_can_fire = 0;
    cannon->energy = SHIP_MAX_ENERGY;
    cannon->is_hit = FALSE;
    cannon->is_alive = TRUE;
    cannon->draw_func = NULL;
    cannon->rotation_speed = 1;

    cannon_status->init();
    cannon_status->x = 30;
    cannon_status->y = 30;
    cannon_status->rotation = 0;
    cannon_status->energy = CANNON_MAX_ENERGY;
    cannon_status->draw_func = (canvas_item_draw) draw_energy_bar;

    player->init();
    player->p.x = WIDTH / 2 * FIXED_POINT_SCALE_FACTOR;
    player->p.y = 150 * FIXED_POINT_SCALE_FACTOR;
    player->p.rotation = random () % NUMBER_OF_ROTATION_ANGLES;
    player->p.radius = SHIP_RADIUS;
    player->ticks_until_can_fire = 0;
    player->energy = SHIP_MAX_ENERGY;
    player->is_hit = FALSE;
    player->is_alive = TRUE;
    player->draw_func = NULL;
    player->rotation_speed = 3;

    player_status->init();
    player_status->x = WIDTH - 30;
    player_status->y = 30;
    player_status->rotation = PI;
    player_status->energy = SHIP_MAX_ENERGY;
    player_status->draw_func = (canvas_item_draw) draw_energy_bar;

    init_rings_array ();
    init_stars_array ();
    init_missiles_array ();

    game_over_message = NULL;
}

int Game::addObject(GameObject* o) {
    if (num_objects >= MAX_OBJECTS)
	return 1;

    objects[num_objects++] = o;
    return 0;
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
  cannon_status->draw(cr);
  player_status->draw(cr);

  if (game_over_message == NULL)
    {
      if (!cannon->is_alive) {
	/* Bonus life */
	num_player_lives++;
	game_over_message = "Next Level!";
	advance_level();
      }
      if (!player->is_alive)
	{
	  num_player_lives--;
	  game_over_message = "Try again!!!";
	}

      if (num_player_lives <= 0)
	{
	  game_over_message = "Game Over";
	}
    }
  if (game_over_message != NULL)
    {
      show_text_message (cr, 80, -30, game_over_message);
      show_text_message (cr, 30, +40, "Press [ENTER] to restart");
    }

}

void Game::drawShip(cairo_t *cr) {
  cairo_save (cr);
  cairo_translate (cr, cannon->p.x / FIXED_POINT_SCALE_FACTOR,
		   cannon->p.y / FIXED_POINT_SCALE_FACTOR);
  cairo_rotate (cr, cannon->p.rotation * RADIANS_PER_ROTATION_ANGLE);
  this->drawCannon (cr, cannon);
  cairo_restore (cr);

  cairo_save (cr);
  cairo_translate (cr, player->p.x / FIXED_POINT_SCALE_FACTOR,
		   player->p.y / FIXED_POINT_SCALE_FACTOR);
  cairo_rotate (cr, player->p.rotation * RADIANS_PER_ROTATION_ANGLE);
  draw_ship_body (cr, player);
  cairo_restore (cr);
}

void Game::drawCannon(cairo_t *cr, GameObject *player) {
  draw_cannon (cr, player);
}

void Game::drawRings(cairo_t *cr) {
  for (int i = 0; i <MAX_NUMBER_OF_RINGS; i++) {
    if (rings[i].is_alive)
      {
	cairo_save (cr);
	cairo_translate (cr,
			 rings[i].p.x / FIXED_POINT_SCALE_FACTOR,
			 rings[i].p.y / FIXED_POINT_SCALE_FACTOR);
	cairo_rotate (cr,
		      -1 * rings[i].p.rotation * RADIANS_PER_ROTATION_ANGLE
		      - PI/2.0);

	cairo_set_source_rgba (cr, 2-i, i? 1.0/i : 0, 0, 0.6);

	draw_ring (cr, &(rings[i]));
	cairo_restore (cr);
      }
  }
}

void Game::drawMissiles(cairo_t *cr) {
  for (int i = 0; i < MAX_NUMBER_OF_MISSILES; i++)
    {
      if (missiles[i].is_alive)
	{
	  cairo_save (cr);
	  cairo_translate (cr, missiles[i].p.x / FIXED_POINT_SCALE_FACTOR,
			   missiles[i].p.y / FIXED_POINT_SCALE_FACTOR);
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
    case GDK_Escape:
	gtk_main_quit ();
	break;

    case GDK_Return:
	if (game_over_message != NULL)
	{
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
	player->is_turning_left = key_is_on;
	break;
    case GDK_Right:
    case GDK_KP_Right:
	player->is_turning_right = key_is_on;
	break;
    case GDK_Up:
    case GDK_KP_Up:
	player->is_thrusting = key_is_on;
	break;
    case GDK_Down:
    case GDK_KP_Down:
	player->is_reversing = key_is_on;
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
	missiles[i].is_alive = FALSE;
    }
}

void
Game::init_rings_array ()
{
    int rot = 1;
    for (int i=0; i < MAX_NUMBER_OF_RINGS; i++)
    {
	rings[i].x = WIDTH / 2;
	rings[i].y = HEIGHT / 2;
	rings[i].p.x = WIDTH / 2 * FIXED_POINT_SCALE_FACTOR;
	rings[i].p.y = HEIGHT / 2 * FIXED_POINT_SCALE_FACTOR;
	rings[i].is_alive = TRUE;
	rings[i].scale = i;
	rings[i].energy = SEGMENTS_PER_RING;
	rings[i].rotation_speed = rot;
	rot *= -1;
	for (int j=0; j<SEGMENTS_PER_RING; j++) {
	    rings[i].component_energy[j] = 1 + level;
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
	stars[i].x = random () % WIDTH;
	stars[i].y = random () % HEIGHT;
	stars[i].rotation = drand48 () * TWO_PI;
	stars[i].scale = 0.5 + (drand48 ());
	stars[i].draw_func = draw_star;
    }
}

//------------------------------------------------------------------------------

void
Game::advance_level()
{
    level++;

    // TODO:  On advanced levels, take into account the speed the player
    // is going, and try to lead him a bit.

    // Increase thickness of rings

    // Add another ring

    // Increase rotation speed of rings

    // Give cannon better or additional weaponry

    // Add gravitational attraction
    // Add forcefield repulsion

    // Make cannon smarter
    // + Selectively shoot out one inner ring segment
    // + If only 2 segments left in outer layer, shoot them
    // + Lead the player's ship when firing

}

//------------------------------------------------------------------------------
