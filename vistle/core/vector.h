#ifndef VECTOR_H
#define VECTOR_H

#include "scalar.h"
#include <cmath>

#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Geometry>

namespace vistle {

typedef Eigen::Matrix<Scalar, 2, 1> Vector2;
typedef Eigen::Matrix<Scalar, 3, 1> Vector3;

typedef Vector3 Vector;

} // namespace vistle

#endif
