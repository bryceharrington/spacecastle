#include <stdlib.h>
#include <cairo.h>

#include "canvas.h"
#include "world.h"

void draw_star (cairo_t * cr, CanvasItem * item);


World::World() {
  for (int i = 0; i < NUMBER_OF_STARS; i++)
  {
    stars[i].pos[0] = random() % WIDTH;
    stars[i].pos[1] = random() % HEIGHT;
    stars[i].rotation = drand48 () * TWO_PI;
    stars[i].scale = 0.5 + (drand48 ());
    stars[i].draw_func = draw_star;
  }
}

World::~World() {
}

void World::draw(cairo_t *cr) {
    // background
    cairo_set_source_rgb(cr, 0.1, 0.0, 0.1);
    cairo_paint(cr);

    // stars
    for (int i = 0; i < NUMBER_OF_STARS; i++) {
	stars[i].draw(cr);
    }
}
