#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "game.h"
#include "game-object.h"

// TODO:  Need to find best place for these...
void draw_star (cairo_t * cr, CanvasItem * item);
void draw_energy_bar (cairo_t * cr, GameObject * item);
void init_trigonometric_tables (void);

static RGB_t color_red      = {0.9, 0.2, 0.3};
static RGB_t color_darkred  = {0.5, 0.2, 0.3};
static RGB_t color_blue     = {0.3, 0.5, 0.9};
static RGB_t color_darkblue = {0.1, 0.3, 0.3};

Game::Game(gint argc, gchar ** argv)
    : num_objects(0), next_missile_index(0), num_player_lives(3)
{
    gtk_init (&argc, &argv);

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


//------------------------------------------------------------------------------

gint
on_key_press (GtkWidget * widget, GdkEventKey * event)
{
    return on_key_event (widget, event, TRUE);
}

//------------------------------------------------------------------------------

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
            printf ("Scale: %f\n", canvas->debug_scale_factor);
        }
        break;
    case GDK_bracketright:
        if (key_is_on)
        {
            canvas->debug_scale_factor *= 1.25f;
            printf ("Scale: %f\n", canvas->debug_scale_factor);
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
            rings[i].component_energy[j] = 6;
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
