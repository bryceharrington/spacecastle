#ifndef __DRAWING_H__
#define __DRAWING_H__


void draw_energy_bar (cairo_t *, GameObject *player);
void draw_flare (cairo_t *, RGB_t);
void draw_ring (cairo_t *, GameObject *ring);
void draw_missile (cairo_t *, GameObject *missile);
void draw_exploded_missile (cairo_t *, GameObject *missile);
void draw_ship_body (cairo_t *, GameObject *player);
void draw_cannon (cairo_t *, GameObject *player);
void draw_star (cairo_t * cr, CanvasItem * item);
void draw_turning_flare (cairo_t *, RGB_t, int);
void show_text_message (cairo_t *, int, int, const char *);

#endif
