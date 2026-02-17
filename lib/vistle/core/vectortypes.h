#ifndef VISTLE_CORE_VECTORTYPES_H
#define VISTLE_CORE_VECTORTYPES_H

#include "scalar.h"
#include <array>
#include <string>
#include <complex>
#ifndef NDEBUG
#define EIGEN_INITIALIZE_MATRICES_BY_NAN
#endif
#include <Eigen/Core>

namespace vistle {

template<class T, int d>
using VistleVector = Eigen::Matrix<T, d, 1>;
template<int d>
using ScalarVector = VistleVector<Scalar, d>;
template<int d>
using DoubleVector = VistleVector<double, d>;
template<int d>
using FloatVector = VistleVector<float, d>;

typedef Eigen::Matrix<Scalar, 1, 1> Vector1;
typedef Eigen::Matrix<Scalar, 2, 1> Vector2;
typedef Eigen::Matrix<Scalar, 3, 1> Vector3;
typedef Eigen::Matrix<Scalar, 4, 1> Vector4;

typedef Eigen::Matrix<Scalar, 2, 3> Matrix2x3;
typedef Eigen::Matrix<Scalar, 3, 2> Matrix3x2;

typedef Eigen::Matrix<Scalar, 1, 1> Matrix1;
typedef Eigen::Matrix<Scalar, 2, 2> Matrix2;
typedef Eigen::Matrix<Scalar, 3, 3> Matrix3;
typedef Eigen::Matrix<Scalar, 4, 4> Matrix4;


typedef Eigen::Matrix<double, 1, 1> DoubleVector1;
typedef Eigen::Matrix<double, 2, 1> DoubleVector2;
typedef Eigen::Matrix<double, 3, 1> DoubleVector3;
typedef Eigen::Matrix<double, 4, 1> DoubleVector4;

typedef Eigen::Matrix<double, 2, 3> DoubleMatrix2x3;
typedef Eigen::Matrix<double, 3, 2> DoubleMatrix3x2;

typedef Eigen::Matrix<double, 1, 1> DoubleMatrix1;
typedef Eigen::Matrix<double, 2, 2> DoubleMatrix2;
typedef Eigen::Matrix<double, 3, 3> DoubleMatrix3;
typedef Eigen::Matrix<double, 4, 4> DoubleMatrix4;

typedef Eigen::Matrix<float, 1, 1> Vector1f;
typedef Eigen::Matrix<float, 2, 1> Vector2f;
typedef Eigen::Matrix<float, 3, 1> Vector3f;
typedef Eigen::Matrix<float, 4, 1> Vector4f;

typedef Eigen::Matrix<float, 2, 3> Matrix2x3f;
typedef Eigen::Matrix<float, 3, 2> Matrix3x2f;

typedef Eigen::Matrix<float, 1, 1> Matrix1f;
typedef Eigen::Matrix<float, 2, 2> Matrix2f;
typedef Eigen::Matrix<float, 3, 3> Matrix3f;
typedef Eigen::Matrix<float, 4, 4> Matrix4f;

typedef Eigen::Matrix<double, 1, 1> Vector1d;
typedef Eigen::Matrix<double, 2, 1> Vector2d;
typedef Eigen::Matrix<double, 3, 1> Vector3d;
typedef Eigen::Matrix<double, 4, 1> Vector4d;

typedef Eigen::Matrix<double, 2, 3> Matrix2x3d;
typedef Eigen::Matrix<double, 3, 2> Matrix3x2d;

typedef Eigen::Matrix<double, 1, 1> Matrix1d;
typedef Eigen::Matrix<double, 2, 2> Matrix2d;
typedef Eigen::Matrix<double, 3, 3> Matrix3d;
typedef Eigen::Matrix<double, 4, 4> Matrix4d;

} // namespace vistle
#endif
