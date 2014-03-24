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

#include "game-math.h"

int cos_table[NUMBER_OF_ROTATION_ANGLES];
int sin_table[NUMBER_OF_ROTATION_ANGLES];

void
init_trigonometric_tables ()
{
    int i;
    int q = (NUMBER_OF_ROTATION_ANGLES / 4);

    for (i = 0; i < NUMBER_OF_ROTATION_ANGLES; i++)
    {
        // our angle system is "true north" - 0 is straight up, whereas                                 
        // cos & sin take 0 as east (and in radians).                                                   
        double angle_in_radians = (q - i) * TWO_PI / NUMBER_OF_ROTATION_ANGLES;
	cos_table[i] =
          +(int) (cos (angle_in_radians) * FIXED_POINT_SCALE_FACTOR);

      // also, our graphics system is "y axis down", although in regular math,                        
      // the y axis is "up", so we have to multiply sin by -1.                                        
      sin_table[i] =
          -(int) (sin (angle_in_radians) * FIXED_POINT_SCALE_FACTOR);
    }
}

int
arctan (double y, double x)
{
    int rot = (atan2(y, x) + PI) * NUMBER_OF_ROTATION_ANGLES / TWO_PI;
    return ( rot + (3 * NUMBER_OF_ROTATION_ANGLES / 4) ) % NUMBER_OF_ROTATION_ANGLES;
}
