#ifndef CUTGEOMETRY_H
#define CUTGEOMETRY_H

#include "module.h"

class Vec3 {
public:
   Vec3(const float _x, const float _y, const float _z)
      : x(_x), y(_y), z(_z) {

   }

   Vec3 operator - () const {

      return Vec3(-x, -y, -z);
   }

   Vec3 operator * (float const & rhs) const {

      return Vec3(rhs * x, rhs * y, rhs * z);
   }


   Vec3 operator + (Vec3 const & rhs) const {

      return Vec3(x + rhs.x, y + rhs.y, z + rhs.z);
   }

   Vec3 operator - (Vec3 const & rhs) const {

      return Vec3(x - rhs.x, y - rhs.y, z - rhs.z);
   }

   float operator * (Vec3 const & rhs) const {

      return x * rhs.x + y * rhs.y + z * rhs.z;
   }

   float x, y, z;
};

class CutGeometry: public vistle::Module {

 public:
   CutGeometry(int rank, int size, int moduleID);
   ~CutGeometry();

   vistle::Object * cutGeometry(const vistle::Object * object,
                                const Vec3 & point,
                                const Vec3 & normal) const;

 private:
   virtual bool compute();
};

#endif
