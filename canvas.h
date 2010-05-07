#ifndef __CANVAS_H__
#define __CANVAS_H__

class CanvasItem;

typedef void   (* canvas_item_draw) (cairo_t * cr, CanvasItem * item);

// TODO: Merge this into this class
extern void scale_for_aspect_ratio (cairo_t * cr, int widget_width, int widget_height);


typedef struct
{
    gdouble r, g, b;
}
RGB_t;

inline void
add_color_stop (cairo_pattern_t* pat, double offset, RGB_t color, double alpha)
{
    cairo_pattern_add_color_stop_rgba (pat, offset, color.r, color.g, color.b, alpha);
}

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
    void set_theme(RGB_t primary, RGB_t secondary);
};

class Canvas {
 private:
    int      width;
    int      height;

 public:
    double   debug_scale_factor;

    Canvas(int w, int h);
    ~Canvas() { }
    
    void   scale_for_aspect_ratio(cairo_t *cr, int window_width, int window_height);
};

#endif 

