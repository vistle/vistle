#ifndef VISTLE_VECTORTYPES_H
#define VISTLE_VECTORTYPES_H

#include "scalar.h"
#include <array>
#include <string>
#include <complex>
#ifndef NDEBUG
#define EIGEN_INITIALIZE_MATRICES_BY_NAN
#endif
#include <eigen3/Eigen/src/Core/util/Macros.h>
#include <eigen3/Eigen/src/Core/util/ConfigureVectorization.h>
#include <eigen3/Eigen/src/Core/util/Constants.h>
#include <eigen3/Eigen/src/Core/util/Meta.h>
#include <eigen3/Eigen/src/Core/util/ForwardDeclarations.h>

namespace vistle {

template<class T, int d>
using VistleVector = Eigen::Matrix<T, d, 1>;
template<int d>
using ScalarVector = VistleVector<Scalar, d>;
template<int d>
using DoubleVector = VistleVector<double, d>;
template<int d>
using FloatVector = VistleVector<float, d>;

template<int d>
struct VistleScalarVector {
    typedef Eigen::Matrix<Scalar, d, 1> type;
};

typedef Eigen::Matrix<Scalar, 1, 1> Vector1;
typedef Eigen::Matrix<Scalar, 2, 1> Vector2;
typedef Eigen::Matrix<Scalar, 3, 1> Vector3;
typedef Eigen::Matrix<Scalar, 4, 1> Vector4;

//typedef Vector3 Vector; // typedef removed for avoiding confusion with Eigen::Vector

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

} // namespace vistle
#endif
