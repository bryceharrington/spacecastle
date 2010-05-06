#include <gtk/gtk.h>
#include "game.h"

Game *game;

gint
main (gint argc, gchar ** argv)
{
    game = new Game(argc, argv);
    game->init();
    return game->run();
}
