#ifndef GEO_H
#define GEO_H

#include <array>
#include <numeric>
#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/src/Core/Matrix.h>

namespace vistle {

/**
 * @brief Calculate tetrahedron volume with corner coords.
 *
 *     Vp = volume parallelpiped
 *     Vt = volume tetrahedron
 *     Vt = Vp/6
 *
 *     A = (x1 x2 x3 x4)
 *     B = (y1 y2 y3 y4)
 *     C = (z1 z2 z3 z4)
 *                          | x1 x2 x3 x4 |
 *     Vp = det(A, B, C) =  | y1 y2 y3 y4 |
 *                          | z1 z2 z3 z4 |
 *                          | 1  1  1  1  |
 *
 * @tparam ForwardIt Iterator of container which contain corner coords.
 * @param begin Begin of container.
 *
 * @return Volume of tetrahedron.
 */
template<class ForwardIt>
float calcTetrahedronVolume(ForwardIt begin)
{
    Eigen::MatrixXf mat = Eigen::Map<Eigen::Matrix<float, 4, 4>>(begin);
    auto v_p = mat.determinant();
    return v_p / 6;
}

/**
 * @brief Calculate volume geometry which isn't self-penetrating itself.
 *
 *    VK = sum (det (a,b,c) : a,b,c element cyclic ordered tripel)
 *
 * @tparam ForwardIt Iterator type.
 * @param coords Coordinates container iterator (x1 y1 z1 x2 y2 z2 ... xn yn zn).
 * @param connect Connectivity list iterator to build geometry.
 *
 * @return Volume.
 */
template<class ForwardIt>
float calcGeoVolume(ForwardIt coords, ForwardIt coordsEnd, ForwardIt connect)
{
    //TODO: to implement
    return 0;
}

} // namespace vistle
#endif
