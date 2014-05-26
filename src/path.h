/* spacecastle - A vector graphics space shooter game
 *
 * Copyright © 2014 Bryce Harrington
 *
 * Spacecastle is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Spacecastle is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Spacecastle.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PATH_H
#define PATH_H

#include "forward.h"
#include "point.h"

typedef enum {
    PATH_MOVETO,        /// Start of closed subpath
    PATH_MOVETO_OPEN,   /// Start of open subpath
    PATH_CURVETO,       /// Bezier curve segment
    PATH_LINETO,        /// Line segment
    PATH_END            /// End record
} Pathcode;

/* Analog to NArtBpath */
class PathSegment {
 public:
    Pathcode      code;    /// Type of path segment

    Point         c1;      /// Control point for curves
    Point         c2;      /// Control point for curves
    Point         pt;      /// Next point

    PathSegment() {}

    PathSegment(Pathcode c,
		Coord x1, Coord y1,
		Coord x2, Coord y2,
		Coord x3, Coord y3)
	: c1(x1, y1), c2(x2, y2), pt(x3, y3)
	{}

    PathSegment(Pathcode c, Coord x, Coord y)
	: c1(0.0, 0.0), c2(0.0, 0.0), pt(x, y)
	{}

};


#define MAX_SEGMENTS (256)

/*
 * Analog to SPCurve in Inkscape
 */
class Path {
public:
    PathSegment   segments[MAX_SEGMENTS]; /// Array of path segments
    int           segment_count;          /// Number of segments

    Path() {}
    ~Path();

    void draw(cairo_t * cr);
    int addSegment(PathSegment* p);
    int end();
};


#endif

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-basic-offset:2
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=2:tabstop=8:softtabstop=2:fileencoding=utf-8:textwidth=99 :
