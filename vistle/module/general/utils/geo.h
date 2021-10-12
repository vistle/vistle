#ifndef GEO_H
#define GEO_H

#include <eigen3/Eigen/Core>

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
} // namespace vistle
#endif
