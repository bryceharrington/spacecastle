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

#ifndef __GAME_OBJECT_H__
#define __GAME_OBJECT_H__

#include <glib.h>

#include "forward.h"
#include "canvas.h"

typedef struct
{
    int x, y;
    int vx, vy;

    // 0 is straight up, (NUMBER_OF_ROTATION_ANGLES / 4) is pointing right
    int rotation;

    // used for collision detection - we presume that an object is equivalent
    // to its bounding circle, rather than trying to do something fancy.
    int radius;
}
physics_t;

class GameObject : public CanvasItem {
 public:
    physics_t p;

    // TODO:  Move all this stuff into game object properties or subclasses or something
    gboolean is_thrusting;
    gboolean is_turning_left;
    gboolean is_turning_right;
    gboolean is_firing;
    gboolean is_reversing;

    int ticks_until_can_fire;  // only for spaceships
    int energy;                // for missiles and spaceships
    int component_energy[MAX_COMPONENTS]; // for rings
    int rotation_speed;
    int animation_tick;

    gboolean is_hit;           // only for spaceships
    gboolean is_alive;
    gboolean has_exploded;     // only for missiles

    GameObject() {}
    ~GameObject() {}

    void init();
};

inline void GameObject::init() {
    is_thrusting = FALSE;
    is_reversing = FALSE;
    is_turning_left = FALSE;
    is_turning_right = FALSE;
    is_firing = FALSE;

    p.vx = 0;
    p.vy = 0;

    animation_tick = 0;
}

#endif
