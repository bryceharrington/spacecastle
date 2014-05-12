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

#ifndef POINT_H
#define POINT_H

/*
 * Cartesian point class.  Modelled after NR::Point.
 */

typedef double Coord;

class Point {

 public:
    inline Point()
        { _pt[0] = _pt[1] = 0; }

    inline Point(Coord x, Coord y) {
        _pt[0] = x;
        _pt[1] = y;
    }

    inline Point(Point const &p) {
        for (unsigned i = 0; i < 2; ++i) {
            _pt[i] = p._pt[i];
        }
    }

    inline Point &operator=(Point const &p) {
        for (unsigned i = 0; i < 2; ++i) {
            _pt[i] = p._pt[i];
        }
        return *this;
    }

    inline Coord operator[](unsigned i) const {
        return _pt[i];
    }

    inline Coord &operator[](unsigned i) {
        return _pt[i];
    }

 private:
    Coord _pt[2];
};

#endif
