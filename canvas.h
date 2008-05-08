#ifndef __CANVAS_H__
#define __CANVAS_H__

class CanvasItem;

typedef void   (* canvas_item_draw) (cairo_t * cr, CanvasItem * item);

typedef struct
{
    gdouble r, g, b;
}
RGB_t;

class CanvasItem {
 private:

 public:
    double x, y;
    float  rotation;
    float  scale;
    canvas_item_draw  draw_func;

    RGB_t primary_color;
    RGB_t secondary_color;

    void draw(cairo_t * cr);
    CanvasItem(canvas_item_draw f=NULL);
};


#endif 

