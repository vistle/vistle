#ifndef VISTLE_VECTOR_H
#define VISTLE_VECTOR_H

#include "scalar.h"
#include <cmath>
#include <vistle/util/math.h>

#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Geometry>
#include <eigen3/Eigen/StdVector>

namespace vistle {

typedef Eigen::Quaternion<vistle::Scalar> Quaternion;
typedef Eigen::AngleAxis<vistle::Scalar> AngleAxis;

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

typedef Vector3 Vector;

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

template<class Archive, class M>
void serializeMatrix(Archive &ar, M &m, const unsigned int version)
{
    for (int j = 0; j < m.rows(); ++j) {
        for (int i = 0; i < m.cols(); ++i) {
            ar &m(j, i);
        }
    }
}

template<class FourColMat>
Vector3 transformPoint(const FourColMat &t, const Vector3 &v)
{
    Vector4 v4;
    v4 << v, 1;
    v4 = t * v4;
    return v4.block<3, 1>(0, 0) / v4[3];
}

inline Vector3 cross(const Vector &a, const Vector &b)
{
    return Vector3(difference_of_products(a[1], b[2], a[2], b[1]), difference_of_products(a[2], b[0], a[0], b[2]),
                   difference_of_products(a[0], b[1], a[1], b[0]));
}

} // namespace vistle

EIGEN_DEFINE_STL_VECTOR_SPECIALIZATION(vistle::Vector1)
EIGEN_DEFINE_STL_VECTOR_SPECIALIZATION(vistle::Vector2)
EIGEN_DEFINE_STL_VECTOR_SPECIALIZATION(vistle::Vector3)
EIGEN_DEFINE_STL_VECTOR_SPECIALIZATION(vistle::Vector4)

EIGEN_DEFINE_STL_VECTOR_SPECIALIZATION(vistle::Matrix2x3)
EIGEN_DEFINE_STL_VECTOR_SPECIALIZATION(vistle::Matrix3x2)

//EIGEN_DEFINE_STL_VECTOR_SPECIALIZATION(vistle::Matrix1)
EIGEN_DEFINE_STL_VECTOR_SPECIALIZATION(vistle::Matrix2)
EIGEN_DEFINE_STL_VECTOR_SPECIALIZATION(vistle::Matrix3)
EIGEN_DEFINE_STL_VECTOR_SPECIALIZATION(vistle::Matrix4)


namespace boost {
namespace serialization {

template<class Archive>
void serialize(Archive &ar, vistle::Vector1 &v, const unsigned int version)
{
    vistle::serializeMatrix(ar, v, version);
}

template<class Archive>
void serialize(Archive &ar, vistle::Vector2 &v, const unsigned int version)
{
    vistle::serializeMatrix(ar, v, version);
}

template<class Archive>
void serialize(Archive &ar, vistle::Vector3 &v, const unsigned int version)
{
    vistle::serializeMatrix(ar, v, version);
}

template<class Archive>
void serialize(Archive &ar, vistle::Vector4 &v, const unsigned int version)
{
    vistle::serializeMatrix(ar, v, version);
}

template<class Archive>
void serialize(Archive &ar, vistle::Matrix4 &m, const unsigned int version)
{
    vistle::serializeMatrix(ar, m, version);
}

} // namespace serialization
} // namespace boost

#endif
