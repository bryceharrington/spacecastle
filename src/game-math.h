/* spacecastle - A vector graphics space shooter game
 *
 * Copyright © 2006 Nigel Tao: nigel.tao@myrealbox.com
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

#ifndef __GAME_MATH_H__
#define __GAME_MATH_H__

#include <math.h>

#define TWO_PI (2*M_PI)
#define PI     (M_PI)

// trig computations (and x, y, velocity, etc). are made in fixed point arithmetic
#define FIXED_POINT_SCALE_FACTOR 1024
#define FIXED_POINT_HALF_SCALE_FACTOR 32

// discretization of 360 degrees
#define NUMBER_OF_ROTATION_ANGLES 360
#define RADIANS_PER_ROTATION_ANGLE (TWO_PI / NUMBER_OF_ROTATION_ANGLES)

extern int cos_table[];
extern int sin_table[];

void init_trigonometric_tables ();
int arctan (double y, double x);

#endif
