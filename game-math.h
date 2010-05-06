#ifndef __GAME_MATH_H__
#define __GAME_MATH_H__

#include <math.h>

#define TWO_PI (2*M_PI)
#define PI     (M_PI)

// trig computations (and x, y, velocity, etc). are made in fixed point arithmetic
#define FIXED_POINT_SCALE_FACTOR 1024
#define FIXED_POINT_HALF_SCALE_FACTOR 32

// discretization of 360 degrees
#define NUMBER_OF_ROTATION_ANGLES 180
#define RADIANS_PER_ROTATION_ANGLE (TWO_PI / NUMBER_OF_ROTATION_ANGLES)

extern int cos_table[];
extern int sin_table[];

void init_trigonometric_tables ();
int arctan (double y, double x);

#endif
