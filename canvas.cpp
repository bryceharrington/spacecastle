#include <gtk/gtk.h>
#include "canvas.h"

CanvasItem::CanvasItem (canvas_item_draw f)
    : x(0), y(0), rotation(0.0), scale(1.0), draw_func(f)
{
}

void
CanvasItem::draw (cairo_t * cr)
{
    if (!draw_func)
        return;

    cairo_save (cr);
    cairo_translate (cr, this->x, this->y);
    cairo_rotate (cr, this->rotation);
    cairo_scale (cr, this->scale, this->scale);
    (*draw_func) (cr, this);
    cairo_restore (cr);
}


