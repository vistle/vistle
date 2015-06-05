#ifndef VECTOR_H
#define VECTOR_H

#include "scalar.h"
#include <cmath>

#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Geometry>

namespace vistle {

typedef Eigen::Quaternion<vistle::Scalar> Quaternion;
typedef Eigen::AngleAxis<vistle::Scalar> AngleAxis;

#if __cplusplus >= 201103L
template<int d>
using ScalarVector = Eigen::Matrix<Scalar, d, 1>;
#endif

template<int d>
struct VistleScalarVector {
   typedef Eigen::Matrix<Scalar, d, 1> type;
};

typedef Eigen::Matrix<Scalar, 1, 1> Vector1;
typedef Eigen::Matrix<Scalar, 2, 1> Vector2;
typedef Eigen::Matrix<Scalar, 3, 1> Vector3;
typedef Eigen::Matrix<Scalar, 4, 1> Vector4;

typedef Vector3 Vector;

typedef Eigen::Matrix<Scalar, 2, 3> Matrix2x3;
typedef Eigen::Matrix<Scalar, 3, 2> Matrix3x2;

typedef Eigen::Matrix<Scalar, 1, 1> Matrix1;
typedef Eigen::Matrix<Scalar, 2, 2> Matrix2;
typedef Eigen::Matrix<Scalar, 3, 3> Matrix3;
typedef Eigen::Matrix<Scalar, 4, 4> Matrix4;

template<class Archive, class M>
void serializeMatrix(Archive & ar, M &m, const unsigned int version) {

   for (int j=0; j<m.rows(); ++j) {
      for (int i=0; i<m.cols(); ++i) {
         ar & m(j, i);
      }
   }
}

} // namespace vistle

namespace boost {
namespace serialization {

template<class Archive>
void serialize(Archive & ar, vistle::Vector1 &v, const unsigned int version) {

   vistle::serializeMatrix(ar, v, version);
}

template<class Archive>
void serialize(Archive & ar, vistle::Vector2 &v, const unsigned int version) {

   vistle::serializeMatrix(ar, v, version);
}

template<class Archive>
void serialize(Archive & ar, vistle::Vector3 &v, const unsigned int version) {

   vistle::serializeMatrix(ar, v, version);
}

template<class Archive>
void serialize(Archive & ar, vistle::Vector4 &v, const unsigned int version) {

   vistle::serializeMatrix(ar, v, version);
}

template<class Archive>
void serialize(Archive & ar, vistle::Matrix4 &m, const unsigned int version) {

   vistle::serializeMatrix(ar, m, version);
}

} // namespace serialization
} // namespace boost

#endif
