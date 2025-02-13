#ifndef VISTLE_CORE_VECTOR_H
#define VISTLE_CORE_VECTOR_H

#include "scalar.h"
#include <cmath>
#include <vistle/util/math.h>
#include "vectortypes.h"

#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Geometry>
#include <eigen3/Eigen/StdVector>

namespace vistle {

typedef Eigen::Quaternion<vistle::Scalar> Quaternion;
typedef Eigen::AngleAxis<vistle::Scalar> AngleAxis;

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

inline Vector3 cross(const Vector3 &a, const Vector3 &b)
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
