#ifndef VISTLE_TOUNSTRUCTURED_CARTESIAN3_H
#define VISTLE_TOUNSTRUCTURED_CARTESIAN3_H

//-------------------------------------------------------------------------
// * container for storing a (cartesian) triple of any type
// *
// * Sever Topan, 2016
//-------------------------------------------------------------------------

template<class T>
struct Cartesian3 {
    T x, y, z;

    // constructor
    Cartesian3(T _x, T _y, T _z): x(_x), y(_y), z(_z) {}

    // index into Cartesian3
    T operator[](unsigned index)
    {
        switch (index) {
        case 0:
            return x;
            break;
        case 1:
            return y;
            break;
        case 2:
            return z;
            break;
        default:
            assert("should never be called");
            return x;
        }
    }
};
#endif
