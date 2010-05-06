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
