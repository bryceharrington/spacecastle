#ifndef __GAME_H__
#define __GAME_H__

#include <stdlib.h>
#include <time.h>
#include <gtk/gtk.h>

#include "game-debug.h"
#include "game-config.h"
#include "game-object.h"  // TODO:  Remove this
#include "game-math.h"
#
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
    const char  *game_over_message;
    double       debug_scale_factor;

    // TODO:  Move these into objects[]
    GameObject  *cannon;
    GameObject  *cannon_status;
    GameObject  *player;
    GameObject  *player_status;
    int          num_player_lives;
    GameObject   missiles[MAX_NUMBER_OF_MISSILES];
    int          next_missile_index;
    GameObject   rings[MAX_NUMBER_OF_RINGS];
    int          next_ring_index;
    int          level;

    // TODO:  Move these into a background object structure
    Canvas      *canvas;
    CanvasItem   stars[NUMBER_OF_STARS];

    Game(gint argc, gchar ** argv);
    ~Game();

    void init();
    void init_missiles_array ();
    void init_stars_array ();
    void init_rings_array ();

    void reset();
    void advance_level();
    int run();

    int addObject(GameObject* o);

    gint handle_key_event (GtkWidget * widget, GdkEventKey * event, gboolean key_is_on);
};

extern Game* game;

inline void Game::init() {
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

    reset();
}

inline int Game::run() {
    gtk_widget_show_all (window);
    gtk_main ();

    return 0;
}


#endif
