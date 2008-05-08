#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "game.h"
#include "game-object.h"

// TODO:  Need to find best place for this...
void draw_star (cairo_t * cr, CanvasItem * item);

Game::Game(gint argc, gchar ** argv)
    : num_objects(0), debug_scale_factor(1.0), next_missile_index(0)
{
    gtk_init (&argc, &argv);

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

    canon->p.x = WIDTH / 2 * FIXED_POINT_SCALE_FACTOR;
    canon->p.y = HEIGHT / 2 * FIXED_POINT_SCALE_FACTOR;
    canon->p.vx = 0;
    canon->p.vy = 0;
    canon->p.rotation = random () % NUMBER_OF_ROTATION_ANGLES;
    canon->p.radius = CANON_RADIUS;
    canon->is_thrusting = FALSE;
    canon->is_turning_left = FALSE;
    canon->is_turning_right = FALSE;
    canon->is_firing = FALSE;
    canon->primary_color.r = 0.3;
    canon->primary_color.g = 0.5;
    canon->primary_color.b = 0.9;
    canon->secondary_color.r = 0.1;
    canon->secondary_color.g = 0.3;
    canon->secondary_color.b = 0.3;
    canon->ticks_until_can_fire = 0;
    canon->energy = SHIP_MAX_ENERGY;
    canon->is_hit = FALSE;
    canon->is_alive = TRUE;

    player->p.x = WIDTH / 3 * FIXED_POINT_SCALE_FACTOR;
    player->p.y = (HEIGHT - 150) * FIXED_POINT_SCALE_FACTOR;
    player->p.vx = 0;
    player->p.vy = 0;
    player->p.rotation = random () % NUMBER_OF_ROTATION_ANGLES;
    player->p.radius = SHIP_RADIUS;
    player->is_thrusting = FALSE;
    player->is_turning_left = FALSE;
    player->is_turning_right = FALSE;
    player->is_firing = FALSE;
    player->primary_color.r = 0.9;
    player->primary_color.g = 0.2;
    player->primary_color.b = 0.3;
    player->secondary_color.r = 0.5;
    player->secondary_color.g = 0.2;
    player->secondary_color.b = 0.3;
    player->ticks_until_can_fire = 0;
    player->energy = SHIP_MAX_ENERGY;
    player->is_hit = FALSE;
    player->is_alive = TRUE;

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
            debug_scale_factor /= 1.25f;
            printf ("Scale: %f\n", debug_scale_factor);
        }
        break;
    case GDK_bracketright:
        if (key_is_on)
        {
            debug_scale_factor *= 1.25f;
            printf ("Scale: %f\n", debug_scale_factor);
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
    for (int i=0; i < MAX_NUMBER_OF_RINGS; i++)
    {
        rings[i].x = WIDTH / 2;
        rings[i].y = HEIGHT / 2;
        rings[i].p.x = WIDTH / 2 * FIXED_POINT_SCALE_FACTOR;
        rings[i].p.y = HEIGHT / 2 * FIXED_POINT_SCALE_FACTOR;
        rings[i].is_alive = TRUE;
        rings[i].scale = i;
        rings[i].energy = (1 << 2*SEGMENTS_PER_RING) - 1;

        rings[i].primary_color.r = 0.3;
        rings[i].primary_color.g = 1.0;
        rings[i].primary_color.b = 0.9;
        rings[i].secondary_color.r = 0.1;
        rings[i].secondary_color.g = 1.0;
        rings[i].secondary_color.b = 0.3;
    }
    rings[0].p.radius = SHIELD_INNER_RADIUS;
    rings[1].p.radius = SHIELD_MIDDLE_RADIUS;
    rings[2].p.radius = SHIELD_OUTER_RADIUS;
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
