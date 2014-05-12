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

#ifndef __GAME_CONFIG_H__
#define __GAME_CONFIG_H__

#include "game-math.h"

#define WIDTH  800
#define HEIGHT 600

// equivalent to 25 fps
#define MILLIS_PER_FRAME 40

// maximum number of game objects allowed
#define MAX_OBJECTS (1024)

#define MAX_COMPONENTS (8)

#define MAX_NUMBER_OF_MISSILES 60

#define NUMBER_OF_STARS 30

// a shot every 9/25 seconds = 8 ticks between shots
#define TICKS_BETWEEN_FIRE 8

// fudge this for bigger or smaller ships
#define GLOBAL_SHIP_SCALE_FACTOR 0.8

#define SHIP_ACCELERATION_FACTOR 1
#define SHIP_MAX_VELOCITY (10 * FIXED_POINT_SCALE_FACTOR)
#define SHIP_RADIUS ((int) (38 * FIXED_POINT_SCALE_FACTOR * GLOBAL_SHIP_SCALE_FACTOR))

#define CANNON_RADIUS ((int) (40 * FIXED_POINT_SCALE_FACTOR * GLOBAL_SHIP_SCALE_FACTOR))
#define SHIELD_INNER_RADIUS ((int) (60 * FIXED_POINT_SCALE_FACTOR))
#define SHIELD_MIDDLE_RADIUS ((int) (70 * FIXED_POINT_SCALE_FACTOR))
#define SHIELD_OUTER_RADIUS ((int) (80 * FIXED_POINT_SCALE_FACTOR))

#define SHIP_MAX_ENERGY    1000
#define CANNON_MAX_ENERGY  2000
#define DAMAGE_PER_MISSILE 100
#define ENERGY_PER_MISSILE 10

// bounce damage depends on how fast you're going
#define DAMAGE_PER_SHIP_BOUNCE_DIVISOR 3

#define MISSILE_RADIUS (4 * FIXED_POINT_SCALE_FACTOR)
#define MISSILE_SPEED 8
#define MISSILE_TICKS_TO_LIVE 60
#define MISSILE_EXPLOSION_TICKS_TO_LIVE 6

#define SEGMENTS_PER_RING (8)
#define MAX_NUMBER_OF_RINGS 3

#endif
