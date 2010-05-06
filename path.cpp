#include <gtk/gtk.h>
#include "path.h"

Path::~Path()
{
    /*
    for (; --segment_count;)
        g_free( segments[segment_count] );
    */
}

void
Path::draw(cairo_t * cr)
{
    for (int i=0; i<segment_count; i++) {
        switch (segments[i].code) {
        case PATH_MOVETO:
            cairo_move_to (cr, segments[i].pt[0], segments[i].pt[1]);
            break;
        case PATH_MOVETO_OPEN:
            break;
        case PATH_CURVETO:
            cairo_curve_to (cr, 
                            segments[i].c1[0], segments[i].c1[1],
                            segments[i].c2[0], segments[i].c2[1],
                            segments[i].pt[0], segments[i].pt[1]);
            break;
        case PATH_LINETO:
            cairo_line_to (cr, segments[i].pt[0], segments[i].pt[1]);
            break;
        case PATH_END:
            break;
        default:
            break;
        }
    }
}

int
Path::addSegment(PathSegment* p) {
    if (segment_count >= MAX_SEGMENTS)
        return 1;

    segments[segment_count++] = *p;
    return 0;
}

int
Path::end() {
    PathSegment *p = new PathSegment(PATH_END, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    return addSegment(p);
}
