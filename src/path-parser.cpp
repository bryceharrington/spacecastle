#define __PATH_PARSER_CPP__
/*
  Parses SVG path element data into a bezier path.
  Derived from svg-path.c from Sodipodi, but is adapted to work with
  Path objects

  Copyright (C) 2000 Eazel, Inc.
  Copyright (C) 2000 Lauris Kaplinski
  Copyright (C) 2001 Ximian, Inc.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public
  License along with this program; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA 02111-1307, USA.

  Authors:
  Raph Levien <raph@artofcode.com>
  Lauris Kaplinski <lauris@ximian.com>
*/

#include "path.h"

#include <glib.h>

#include <stdlib.h>
#include <math.h>
#include <cassert>


/*
  At present, there is no support for <marker> or any other contextual
  information from the SVG file. The API will need to change rather
  significantly to support these.

  Reference: SVG working draft 3 March 2000, section 8.
*/

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif  /*  M_PI  */

struct ParsePathContext {
    Path  *bpath;
    double cpx, cpy;  /* current point */
    double rpx, rpy;  /* reflection point (for 's' and 't' commands) */
    double spx, spy;  /* beginning of current subpath point */
    char   cmd;       /* current command (lowercase) */
    int    param;     /* parameter number */
    bool   rel;       /* true if relative coords */
    double params[7]; /* parameters that have been parsed */
};

static void rsvg_path_arc_segment(ParsePathContext *ctx,
                                  double xc, double yc,
                                  double th0, double th1,
                                  double rx, double ry, double x_axis_rotation)
{
    double sin_th, cos_th;
    double a00, a01, a10, a11;
    double x1, y1, x2, y2, x3, y3;
    double t;
    double th_half;

    sin_th = sin (x_axis_rotation * (M_PI / 180.0));
    cos_th = cos (x_axis_rotation * (M_PI / 180.0));
    /* inverse transform compared with rsvg_path_arc */
    a00 = cos_th * rx;
    a01 = -sin_th * ry;
    a10 = sin_th * rx;
    a11 = cos_th * ry;

    th_half = 0.5 * (th1 - th0);
    t = (8.0 / 3.0) * sin(th_half * 0.5) * sin(th_half * 0.5) / sin(th_half);
    x1 = xc + cos (th0) - t * sin (th0);
    y1 = yc + sin (th0) + t * cos (th0);
    x3 = xc + cos (th1);
    y3 = yc + sin (th1);
    x2 = x3 + t * sin (th1);
    y2 = y3 - t * cos (th1);

    PathSegment *p = new PathSegment(PATH_CURVETO,
                                     a00 * x1 + a01 * y1, a10 * x1 + a11 * y1,
                                     a00 * x2 + a01 * y2, a10 * x2 + a11 * y2,
                                     a00 * x3 + a01 * y3, a10 * x3 + a11 * y3);
    ctx->bpath->addSegment( p );
}

/**
 * rsvg_path_arc: Add an RSVG arc to the path context.
 * @ctx: Path context.
 * @rx: Radius in x direction (before rotation).
 * @ry: Radius in y direction (before rotation).
 * @x_axis_rotation: Rotation angle for axes.
 * @large_arc_flag: 0 for arc length <= 180, 1 for arc >= 180.
 * @sweep: 0 for "negative angle", 1 for "positive angle".
 * @x: New x coordinate.
 * @y: New y coordinate.
 *
 **/
static void rsvg_path_arc (ParsePathContext *ctx,
                           double rx, double ry, double x_axis_rotation,
                           int large_arc_flag, int sweep_flag,
                           double x, double y)
{
    double sin_th, cos_th;
    double a00, a01, a10, a11;
    double x0, y0, x1, y1, xc, yc;
    double d, sfactor, sfactor_sq;
    double th0, th1, th_arc;
    double px, py, pl;
    int i, n_segs;

    sin_th = sin (x_axis_rotation * (M_PI / 180.0));
    cos_th = cos (x_axis_rotation * (M_PI / 180.0));

    /*
      Correction of out-of-range radii as described in Appendix F.6.6:

      1. Ensure radii are non-zero (Done?).
      2. Ensure that radii are positive.
      3. Ensure that radii are large enough.
    */

    if(rx < 0.0) rx = -rx;
    if(ry < 0.0) ry = -ry;

    px = cos_th * (ctx->cpx - x) * 0.5 + sin_th * (ctx->cpy - y) * 0.5;
    py = cos_th * (ctx->cpy - y) * 0.5 - sin_th * (ctx->cpx - x) * 0.5;
    pl = (px * px) / (rx * rx) + (py * py) / (ry * ry);

    if(pl > 1.0)
    {
        pl  = sqrt(pl);
        rx *= pl;
        ry *= pl;
    }

    /* Proceed with computations as described in Appendix F.6.5 */

    a00 = cos_th / rx;
    a01 = sin_th / rx;
    a10 = -sin_th / ry;
    a11 = cos_th / ry;
    x0 = a00 * ctx->cpx + a01 * ctx->cpy;
    y0 = a10 * ctx->cpx + a11 * ctx->cpy;
    x1 = a00 * x + a01 * y;
    y1 = a10 * x + a11 * y;
    /* (x0, y0) is current point in transformed coordinate space.
       (x1, y1) is new point in transformed coordinate space.

       The arc fits a unit-radius circle in this space.
    */
    d = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0);
    sfactor_sq = 1.0 / d - 0.25;
    if (sfactor_sq < 0) sfactor_sq = 0;
    sfactor = sqrt (sfactor_sq);
    if (sweep_flag == large_arc_flag) sfactor = -sfactor;
    xc = 0.5 * (x0 + x1) - sfactor * (y1 - y0);
    yc = 0.5 * (y0 + y1) + sfactor * (x1 - x0);
    /* (xc, yc) is center of the circle. */

    th0 = atan2 (y0 - yc, x0 - xc);
    th1 = atan2 (y1 - yc, x1 - xc);

    th_arc = th1 - th0;
    if (th_arc < 0 && sweep_flag)
        th_arc += 2 * M_PI;
    else if (th_arc > 0 && !sweep_flag)
        th_arc -= 2 * M_PI;

    n_segs = (int) ceil (fabs (th_arc / (M_PI * 0.5 + 0.001)));

    for (i = 0; i < n_segs; i++) {
        rsvg_path_arc_segment(ctx, xc, yc,
                              th0 + i * th_arc / n_segs,
                              th0 + (i + 1) * th_arc / n_segs,
                              rx, ry, x_axis_rotation);
    }

    ctx->cpx = x;
    ctx->cpy = y;
}


/* supply defaults for missing parameters, assuming relative coordinates
   are to be interpreted as x,y */
static void rsvg_parse_path_default_xy(ParsePathContext *ctx, int n_params)
{
    int i;

    if (ctx->rel) {
        for (i = ctx->param; i < n_params; i++) {
            if (i > 2)
                ctx->params[i] = ctx->params[i - 2];
            else if (i == 1)
                ctx->params[i] = ctx->cpy;
            else if (i == 0)
                /* we shouldn't get here (usually ctx->param > 0 as
                   precondition) */
                ctx->params[i] = ctx->cpx;
        }
    } else {
        for (i = ctx->param; i < n_params; i++) {
            ctx->params[i] = 0.0;
        }
    }
}

static void rsvg_parse_path_do_cmd(ParsePathContext *ctx, bool final)
{
    double x1, y1, x2, y2, x3, y3;

#ifdef VERBOSE
    int i;

    g_print ("parse_path %c:", ctx->cmd);
    for (i = 0; i < ctx->param; i++) {
        g_print(" %f", ctx->params[i]);
    }
    g_print (final ? ".\n" : "\n");
#endif

    switch (ctx->cmd) {
        case 'm':
            /* moveto */
            if (ctx->param == 2 || final)
            {
                rsvg_parse_path_default_xy (ctx, 2);
#ifdef VERBOSE
                g_print ("'m' moveto %g,%g\n",
                         ctx->params[0], ctx->params[1]);
#endif
                PathSegment *p = new PathSegment(PATH_MOVETO,
                                                 ctx->params[0], ctx->params[1]);
                ctx->bpath->addSegment(p);
                ctx->cpx = ctx->rpx = ctx->spx = ctx->params[0];
                ctx->cpy = ctx->rpy = ctx->spy = ctx->params[1];
                ctx->param = 0;
                ctx->cmd = 'l';
            }
            break;
        case 'l':
            /* lineto */
            if (ctx->param == 2 || final)
            {
                rsvg_parse_path_default_xy (ctx, 2);
#ifdef VERBOSE
                g_print ("'l' lineto %g,%g\n",
                         ctx->params[0], ctx->params[1]);
#endif
                PathSegment *p = new PathSegment(PATH_LINETO,
                                                 ctx->params[0], ctx->params[1]);
                ctx->bpath->addSegment(p);
                ctx->cpx = ctx->rpx = ctx->params[0];
                ctx->cpy = ctx->rpy = ctx->params[1];
                ctx->param = 0;
            }
            break;
        case 'c':
            /* curveto */
            if (ctx->param == 6 || final )
            {
                rsvg_parse_path_default_xy (ctx, 6);
                x1 = ctx->params[0];
                y1 = ctx->params[1];
                x2 = ctx->params[2];
                y2 = ctx->params[3];
                x3 = ctx->params[4];
                y3 = ctx->params[5];
#ifdef VERBOSE
                g_print ("'c' curveto %g,%g %g,%g, %g,%g\n",
                         x1, y1, x2, y2, x3, y3);
#endif
                PathSegment *p = new PathSegment(PATH_CURVETO,
                                                 x1, y1, x2, y2, x3, y3);
                ctx->bpath->addSegment(p);
                ctx->rpx = x2;
                ctx->rpy = y2;
                ctx->cpx = x3;
                ctx->cpy = y3;
                ctx->param = 0;
            }
            break;
        case 's':
            /* smooth curveto */
            if (ctx->param == 4 || final)
            {
                rsvg_parse_path_default_xy (ctx, 4);
                x1 = 2 * ctx->cpx - ctx->rpx;
                y1 = 2 * ctx->cpy - ctx->rpy;
                x2 = ctx->params[0];
                y2 = ctx->params[1];
                x3 = ctx->params[2];
                y3 = ctx->params[3];
#ifdef VERBOSE
                g_print ("'s' curveto %g,%g %g,%g, %g,%g\n",
                         x1, y1, x2, y2, x3, y3);
#endif
                PathSegment *p = new PathSegment(PATH_CURVETO,
                                                 x1, y1, x2, y2, x3, y3);
                ctx->bpath->addSegment(p);
                ctx->rpx = x2;
                ctx->rpy = y2;
                ctx->cpx = x3;
                ctx->cpy = y3;
                ctx->param = 0;
            }
            break;
        case 'h':
            /* horizontal lineto */
            if (ctx->param == 1) {
#ifdef VERBOSE
                g_print ("'h' lineto %g,%g\n",
                         ctx->params[0], ctx->cpy);
#endif
                PathSegment *p = new PathSegment(PATH_LINETO,
                                                 ctx->params[0], ctx->cpy);
                ctx->bpath->addSegment(p);
                ctx->cpx = ctx->rpx = ctx->params[0];
                ctx->param = 0;
            }
            break;
        case 'v':
            /* vertical lineto */
            if (ctx->param == 1) {
#ifdef VERBOSE
                g_print ("'v' lineto %g,%g\n",
                         ctx->cpx, ctx->params[0]);
#endif
                PathSegment *p = new PathSegment(PATH_LINETO,
                                                 ctx->cpx, ctx->params[0]);
                ctx->bpath->addSegment(p);
                ctx->cpy = ctx->rpy = ctx->params[0];
                ctx->param = 0;
            }
            break;
        case 'q':
            /* quadratic bezier curveto */

            /* non-normative reference:
               http://www.icce.rug.nl/erikjan/bluefuzz/beziers/beziers/beziers.html
            */
            if (ctx->param == 4 || final)
            {
                rsvg_parse_path_default_xy (ctx, 4);
                /* raise quadratic bezier to cubic */
                x1 = (ctx->cpx + 2 * ctx->params[0]) * (1.0 / 3.0);
                y1 = (ctx->cpy + 2 * ctx->params[1]) * (1.0 / 3.0);
                x3 = ctx->params[2];
                y3 = ctx->params[3];
                x2 = (x3 + 2 * ctx->params[0]) * (1.0 / 3.0);
                y2 = (y3 + 2 * ctx->params[1]) * (1.0 / 3.0);
#ifdef VERBOSE
                g_print("'q' curveto %g,%g %g,%g, %g,%g\n",
                        x1, y1, x2, y2, x3, y3);
#endif
                PathSegment *p = new PathSegment(PATH_CURVETO,
                                                 x1, y1, x2, y2, x3, y3);
                ctx->bpath->addSegment(p);
                ctx->rpx = ctx->params[0];
                ctx->rpy = ctx->params[1];
                ctx->cpx = x3;
                ctx->cpy = y3;
                ctx->param = 0;
            }
            break;
        case 't':
            /* Truetype quadratic bezier curveto */
            if (ctx->param == 2 || final) {
                double xc, yc; /* quadratic control point */

                xc = 2 * ctx->cpx - ctx->rpx;
                yc = 2 * ctx->cpy - ctx->rpy;
                /* generate a quadratic bezier with control point = xc, yc */
                x1 = (ctx->cpx + 2 * xc) * (1.0 / 3.0);
                y1 = (ctx->cpy + 2 * yc) * (1.0 / 3.0);
                x3 = ctx->params[0];
                y3 = ctx->params[1];
                x2 = (x3 + 2 * xc) * (1.0 / 3.0);
                y2 = (y3 + 2 * yc) * (1.0 / 3.0);
#ifdef VERBOSE
                g_print ("'t' curveto %g,%g %g,%g, %g,%g\n",
                         x1, y1, x2, y2, x3, y3);
#endif
                PathSegment *p = new PathSegment(PATH_CURVETO,
                                                 x1, y1, x2, y2, x3, y3);
                ctx->bpath->addSegment(p);
                ctx->rpx = xc;
                ctx->rpy = yc;
                ctx->cpx = x3;
                ctx->cpy = y3;
                ctx->param = 0;
            } else if (final) {
                if (ctx->param > 2) {
                    rsvg_parse_path_default_xy(ctx, 4);
                    /* raise quadratic bezier to cubic */
                    x1 = (ctx->cpx + 2 * ctx->params[0]) * (1.0 / 3.0);
                    y1 = (ctx->cpy + 2 * ctx->params[1]) * (1.0 / 3.0);
                    x3 = ctx->params[2];
                    y3 = ctx->params[3];
                    x2 = (x3 + 2 * ctx->params[0]) * (1.0 / 3.0);
                    y2 = (y3 + 2 * ctx->params[1]) * (1.0 / 3.0);
#ifdef VERBOSE
                    g_print ("'t' curveto %g,%g %g,%g, %g,%g\n",
                             x1, y1, x2, y2, x3, y3);
#endif
                    PathSegment *p = new PathSegment(PATH_CURVETO,
                                                     x1, y1, x2, y2, x3, y3);
                    ctx->bpath->addSegment(p);
                    ctx->rpx = x2;
                    ctx->rpy = y2;
                    ctx->cpx = x3;
                    ctx->cpy = y3;
                } else {
                    rsvg_parse_path_default_xy(ctx, 2);
#ifdef VERBOSE
                    g_print ("'t' lineto %g,%g\n",
                             ctx->params[0], ctx->params[1]);
#endif
                    PathSegment *p = new PathSegment(PATH_LINETO,
                                                     ctx->params[0], ctx->params[1]);
                    ctx->bpath->addSegment(p);

                    ctx->cpx = ctx->rpx = ctx->params[0];
                    ctx->cpy = ctx->rpy = ctx->params[1];
                }
                ctx->param = 0;
            }
            break;
        case 'a':
            if (ctx->param == 7 || final)
            {
                rsvg_path_arc(ctx,
                              ctx->params[0], ctx->params[1], ctx->params[2],
                              (int) ctx->params[3], (int) ctx->params[4],
                              ctx->params[5], ctx->params[6]);
                ctx->param = 0;
            }
            break;
        default:
            ctx->param = 0;
    }
}

static void rsvg_parse_path_data(ParsePathContext *ctx, const char *data)
{
    int i = 0;
    double val = 0;
    char c = 0;
    bool in_num = false;
    bool in_frac = false;
    bool in_exp = false;
    bool exp_wait_sign = false;
    int sign = 0;
    int exp = 0;
    int exp_sign = 0;
    double frac = 0.0;

    /* fixme: Do better error processing: e.g. at least stop parsing as soon as we find an error.
     * At some point we'll need to do all of
     * http://www.w3.org/TR/SVG11/implnote.html#ErrorProcessing.
     */
    for (i = 0; ; i++)
    {
        c = data[i];
        if (c >= '0' && c <= '9')
        {
            /* digit */
            if (in_num)
            {
                if (in_exp)
                {
                    exp = (exp * 10) + c - '0';
                    exp_wait_sign = false;
                }
                else if (in_frac)
                    val += (frac *= 0.1) * (c - '0');
                else
                    val = (val * 10) + c - '0';
            }
            else
            {
                in_num = true;
                assert(!in_frac && !in_exp);
                exp = 0;
                exp_sign = 1;
                exp_wait_sign = false;
                val = c - '0';
                sign = 1;
            }
        }
        else if (c == '.' && !(in_frac || in_exp))
        {
            if (!in_num)
            {
                in_num = true;
                assert(!in_exp);
                exp = 0;
                exp_sign = 1;
                exp_wait_sign = false;
                val = 0;
                sign = 1;
            }
            in_frac = true;
            frac = 1;
        }
        else if ((c == 'E' || c == 'e') && in_num)
        {
            /* fixme: Should we add `&& !in_exp' to the above condition?
             * It looks like the current code will parse `1e3e4' (as 1e4). */
            in_exp = true;
            exp_wait_sign = true;
            exp = 0;
            exp_sign = 1;
        }
        else if ((c == '+' || c == '-') && in_exp)
        {
            exp_sign = c == '+' ? 1 : -1;
        }
        else if (in_num)
        {
            /* end of number */

            val *= sign * pow (10, exp_sign * exp);
            if (ctx->rel)
            {
                /* Handle relative coordinates. This switch statement attempts
                   to determine _what_ the coords are relative to. This is
                   underspecified in the 12 Apr working draft. */
                switch (ctx->cmd)
                {
                    case 'l':
                    case 'm':
                    case 'c':
                    case 's':
                    case 'q':
                    case 't':
                        if ( ctx->param & 1 ) {
                            val += ctx->cpy; /* odd param, y */
                        } else {
                            val += ctx->cpx; /* even param, x */
                        }
                        break;
                    case 'a':
                        /* rule: sixth and seventh are x and y, rest are not
                           relative */
                        if (ctx->param == 5)
                            val += ctx->cpx;
                        else if (ctx->param == 6)
                            val += ctx->cpy;
                        break;
                    case 'h':
                        /* rule: x-relative */
                        val += ctx->cpx;
                        break;
                    case 'v':
                        /* rule: y-relative */
                        val += ctx->cpy;
                        break;
                }
            }
            ctx->params[ctx->param++] = val;
            rsvg_parse_path_do_cmd (ctx, false);
            if (c=='.') {
                in_num = true;
                val = 0;
                in_frac = true;
                in_exp = false;
                frac = 1;
            }
            else {
                in_num = false;
                in_frac = false;
                in_exp = false;
            }
        }

        if (c == '\0')
            break;
        else if ((c == '+' || c == '-') && !exp_wait_sign)
        {
            sign = c == '+' ? 1 : -1;;
            val = 0;
            in_num = true;
            in_frac = false;
            in_exp = false;
            exp = 0;
            exp_sign = 1;
            exp_wait_sign = false;
        }
        else if (c == 'z' || c == 'Z')
        {
            if (ctx->param)
                rsvg_parse_path_do_cmd (ctx, true);
            ctx->bpath->end();

            ctx->cmd = 'm';
            ctx->params[0] = ctx->cpx = ctx->rpx = ctx->spx;
            ctx->params[1] = ctx->cpy = ctx->rpy = ctx->spy;
            ctx->param = 2;
        }
        else if (c >= 'A' && c <= 'Z' && c != 'E')
        {
            if (ctx->param)
                rsvg_parse_path_do_cmd (ctx, true);
            ctx->cmd = c + 'a' - 'A';
            ctx->rel = false;
        }
        else if (c >= 'a' && c <= 'z' && c != 'e')
        {
            if (ctx->param)
                rsvg_parse_path_do_cmd (ctx, true);
            ctx->cmd = c;
            ctx->rel = true;
        }
        /* else c _should_ be whitespace or , */
    }
}


Path *sp_svg_read_path(gchar const *str)
{
    ParsePathContext ctx;
    Path *bpath = new Path;

    ctx.bpath = bpath;
    ctx.cpx = 0.0;
    ctx.cpy = 0.0;
    ctx.cmd = 0;
    ctx.param = 0;

    rsvg_parse_path_data (&ctx, str);

    if (ctx.param && ctx.cmd != 'm') {
        rsvg_parse_path_do_cmd (&ctx, true);
    }

    ctx.bpath = NULL;

    return bpath;
}


/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
