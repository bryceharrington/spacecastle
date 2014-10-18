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

#ifndef __GAME_H__
#define __GAME_H__

#include <glib.h>

#include "forward.h"
#include "game-debug.h"
#include "game-config.h"
#include "game-object.h"

// Forward definitions of handler functions
gint on_expose_event (GtkWidget *, GdkEventExpose *);
gint on_key_event (GtkWidget *, GdkEventKey *, gboolean);
gint on_key_press (GtkWidget *, GdkEventKey *);
gint on_key_release (GtkWidget *, GdkEventKey *);
gint on_timeout (gpointer);

class Game {
private:
  GameObject* objects[MAX_OBJECTS];
  int         num_objects;

public:
  GtkWidget   *window;
  const char  *main_message;
  const char  *second_message;
  int          message_timeout;
  double       debug_scale_factor;

  // TODO:  Move these into objects[]
  GameObject  *cannon;
  GameObject  *player;
  int          num_player_lives;
  GameObject   missiles[MAX_NUMBER_OF_MISSILES];
  int          next_missile_index;
  GameObject   rings[MAX_NUMBER_OF_RINGS];
  int          next_ring_index;
  int          level;

  // TODO: Implement all the following.  Set all to zero in init()
  int          number_of_homing_mines;
  int          cannon_forcefield_strength;
  int          cannon_forcefield_repulsion;
  int          cannon_weapon_count;
  int          cannon_weapon_strength;
  int          energy_per_segment;
  int          number_of_rings;
  int          ring_speed;
  int          gravity_x;
  int          gravity_y;

  // These also need incremented by level
  int          cannon_max_energy;

  gboolean     show_fps;

  // TODO:  Move these into a background object structure
  Canvas      *canvas;
  CanvasItem   stars[NUMBER_OF_STARS];

  Game(gint argc, gchar ** argv);
  ~Game();

  void init();
  void init_missiles_array ();
  void init_stars_array ();
  void init_rings_array ();

  int addObject();
  void checkConditions();
  void drawWorld(cairo_t *cr);
  void drawUI(cairo_t *cr);
  void drawTextMessage(cairo_t *cr, int x, int y, const char*msg);
  void drawShip(cairo_t *cr);
  void drawCannon(cairo_t *, GameObject *player);
  void drawMissiles(cairo_t *cr);
  void drawRings(cairo_t *cr);
  void drawMines(cairo_t *cr);

  void reset();
  void game_over();
  void try_again();
  void advance_level();
  int run();

  int addObject(GameObject* o);
  void process_options(int argc, gchar ** argv);
  gint handle_key_event (GtkWidget * widget, GdkEventKey * event, gboolean key_is_on);
};

extern Game* game;


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
