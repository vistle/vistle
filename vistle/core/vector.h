#ifndef VECTOR_H
#define VECTOR_H

#include "scalar.h"
#include <cmath>

#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Geometry>

namespace vistle {

#if !(defined(__GNUC__) && __GNUC__==4 && __GNUC_MINOR__<=6)
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

typedef Eigen::Matrix<Scalar, 4, 4> Matrix4;

} // namespace vistle

#endif
