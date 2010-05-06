#ifndef __GAME_H__
#define __GAME_H__

#include <stdlib.h>
#include <time.h>
#include <gtk/gtk.h>
#include <map>

//class GameObject;
#include "game-object.h"  // TODO:  Remove this
#include "game-math.h"

#define WIDTH  800
#define HEIGHT 600

// equivalent to 25 fps
#define MILLIS_PER_FRAME 40

// maximum number of game objects allowed
#define MAX_OBJECTS (1024)

#define MAX_NUMBER_OF_MISSILES 60

#define NUMBER_OF_STARS 30

// trig computations (and x, y, velocity, etc). are made in fixed point arithmetic
#define FIXED_POINT_SCALE_FACTOR 1024
#define FIXED_POINT_HALF_SCALE_FACTOR 32

// discretization of 360 degrees
#define NUMBER_OF_ROTATION_ANGLES 180
#define RADIANS_PER_ROTATION_ANGLE (TWO_PI / NUMBER_OF_ROTATION_ANGLES)

// a shot every 9/25 seconds = 8 ticks between shots
#define TICKS_BETWEEN_FIRE 8

// fudge this for bigger or smaller ships
#define GLOBAL_SHIP_SCALE_FACTOR 0.8

#define SHIP_ACCELERATION_FACTOR 1
#define SHIP_MAX_VELOCITY (10 * FIXED_POINT_SCALE_FACTOR)
#define SHIP_RADIUS ((int) (38 * FIXED_POINT_SCALE_FACTOR * GLOBAL_SHIP_SCALE_FACTOR))

#define CANNON_RADIUS ((int) (40 * FIXED_POINT_SCALE_FACTOR * GLOBAL_SHIP_SCALE_FACTOR))
#define SHIELD_OUTER_RADIUS ((int) (60 * FIXED_POINT_SCALE_FACTOR))
#define SHIELD_MIDDLE_RADIUS ((int) (80 * FIXED_POINT_SCALE_FACTOR))
#define SHIELD_INNER_RADIUS ((int) (100 * FIXED_POINT_SCALE_FACTOR))

#define SHIP_MAX_ENERGY 1000
#define DAMAGE_PER_MISSILE 200
#define ENERGY_PER_MISSILE 10

// bounce damage depends on how fast you're going
#define DAMAGE_PER_SHIP_BOUNCE_DIVISOR 3

#define MISSILE_RADIUS (4 * FIXED_POINT_SCALE_FACTOR)
#define MISSILE_SPEED 8
#define MISSILE_TICKS_TO_LIVE 60
#define MISSILE_EXPLOSION_TICKS_TO_LIVE 6

#define SEGMENTS_PER_RING 8
#define MAX_NUMBER_OF_RINGS 3

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

    // TODO:  Could this all be done with a single data structure?
    std::map<GQuark, int>     iparams;
    std::map<GQuark, double>  dparams;
    std::map<GQuark, char*>   param_desc;

 public:
    GtkWidget   *window;
    const char  *game_over_message;
    double       debug_scale_factor;

    // TODO:  Move these into objects[]
    GameObject  *cannon;
    GameObject  *player;
    GameObject   missiles[MAX_NUMBER_OF_MISSILES];
    int          next_missile_index;
    GameObject   rings[MAX_NUMBER_OF_RINGS];
    int          next_ring_index;

    // TODO:  Move these into a background object structure
    CanvasItem   stars[NUMBER_OF_STARS];

    double dparam(GQuark name) const;
    double dparam(const char* name) const;
    int  iparam(GQuark name) const;
    int  iparam(const char* name) const;
    void define(const char* name, int value, const char* desc = NULL);
    void define(const char* name, double value, const char* desc = NULL);

    Game(gint argc, gchar ** argv);
    ~Game();

    void setup_parameters();

    void init();
    void init_missiles_array ();
    void init_stars_array ();
    void init_rings_array ();

    void reset();
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

inline double Game::dparam(GQuark name) const {
    return 0.0;
}

inline double Game::dparam(const char* name) const {
    return dparam(g_quark_from_string(name));
}

inline int  Game::iparam(GQuark name) const {
    return 0;
}

inline int  Game::iparam(const char* name) const {
    return iparam(g_quark_from_string(name));
}

inline void Game::define(const char* name, int value, const char* desc) {
}

inline void Game::define(const char* name, double value, const char* desc) {
}

// Set up the global parameters
inline void Game::setup_parameters() {
    define("FIXED_POINT_SCALE_FACTOR", 1024,
           "trig computations (and x, y, velocity, etc). are made in fixed point arithmetic");
    define("FIXED_POINT_HALF_SCALE_FACTOR", 32,
           "trig computations (and x, y, velocity, etc). are made in fixed point arithmetic");

    define("NUMBER_OF_ROTATION_ANGLES", 60,
           "discretization of 360 degrees");
    define("RADIANS_PER_ROTATION_ANGLE", (TWO_PI / NUMBER_OF_ROTATION_ANGLES),
           "discretization of 360 degrees");

    define("TICKS_BETWEEN_FIRE", 8,
           "a shot every 9/25 seconds = 8 ticks between shots");

    define("GLOBAL_SHIP_SCALE_FACTOR", 0.8,
           "fudge this for bigger or smaller ships");

    define("SHIP_ACCELERATION_FACTOR", 1);
    define("SHIP_MAX_VELOCITY", (10 * FIXED_POINT_SCALE_FACTOR));
    define("SHIP_RADIUS", ((int) (38 * FIXED_POINT_SCALE_FACTOR * GLOBAL_SHIP_SCALE_FACTOR)));

    define("SHIP_MAX_ENERGY", 1000);
    define("DAMAGE_PER_MISSILE", 200);
    define("ENERGY_PER_MISSILE", 10);

    define("DAMAGE_PER_SHIP_BOUNCE_DIVISOR", 3,
           "bounce damage depends on how fast you're going");

    define("NUMBER_OF_STARS", 20);

    define("MAX_NUMBER_OF_MISSILES", 60);
    define("MISSILE_RADIUS", (4 * FIXED_POINT_SCALE_FACTOR));
    define("MISSILE_SPEED", 8);
    define("MISSILE_TICKS_TO_LIVE", 60);
    define("MISSILE_EXPLOSION_TICKS_TO_LIVE", 6);
}


#endif
