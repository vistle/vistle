#include <sstream>
#include <iomanip>

#include <algorithm>
#include <cmath>
#include <tuple>
#include <vistle/core/coords.h>
#include <vistle/core/lines.h>
#include <vistle/core/object.h>
#include <vistle/core/points.h>
#include <vistle/core/polygons.h>
#include <vistle/core/quads.h>
#include <vistle/core/spheres.h>
#include <vistle/core/tubes.h>
#include <vistle/core/unstr.h>
#include <vistle/util/math.h>

#include "Chainmail.h"
#include <eigen3/unsupported/Eigen/src/Splines/Spline.h>
#include <eigen3/unsupported/Eigen/src/Splines/SplineFitting.h>


MODULE_MAIN(Chainmail)


DEFINE_ENUM_WITH_STRING_CONVERSIONS(GeoMode, (Circle)(Spline))

using namespace vistle;

Chainmail::Chainmail(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    createInputPort("quads_in", "Unstructured grid with quads");
    createInputPort("data_in", "mapped data");
    createOutputPort("circles_out", "circles");
    createOutputPort("data_out", "tubes or spheres with mapped data");

    m_geoMode = addIntParameter("geo_mode", "geometry generation mode", Circle, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_geoMode, GeoMode);

    m_radius = addFloatParameter("radius", "radius of the rings (diameter = radius / 20)", -1.0);
    m_numXSegments =
        addIntParameter("number of torus segments", "number of quads used to aproximate the torus in it's plane", 20);
    m_numYSegments = addIntParameter("number of diameter segments",
                                     "number of quads used to aproximate the torus around its axis", 5);
}

Chainmail::~Chainmail()
{}


template<typename T>
Vector3 vec(const T &t, Index i)
{
    return Vector3{t->x()[i], t->y()[i], t->z()[i]};
}

std::vector<Vector3> interpolateCirclePoints(const Vector3 &middle, const Vector3 &normale, Index precition,
                                             float radius)
{
    static const Vector3 base1{0, 0, 1};
    static const Vector3 base2{1, 0, 0};
    auto v1 = base1.cross(normale);
    if (v1 == Vector3{0, 0, 0})
        v1 = base2.cross(normale);
    auto v2 = v1.cross(normale);
    v1.normalize();
    v2.normalize();

    std::vector<Vector3> v;
    auto thetaBase = 2 * EIGEN_PI / precition / precition;
    for (Index i = 0; i < precition; i++) {
        auto tethaH = thetaBase * (i * precition);
        auto d = v1 * sin(tethaH) + v2 * cos(tethaH);
        for (Index j = 0; j < precition; j++) {
            auto p = middle + radius * d;
            auto tethaV = 2 * EIGEN_PI * j / precition;

            auto pp = p + radius / 20 * (normale * sin(tethaV) + d * cos(tethaV));
            v.push_back(pp);
        }
    }
    return v;
}

Vector3 center(const std::vector<Vector3> &points)
{
    return std::accumulate(points.begin(), points.end(), Vector3{0, 0, 0}) / points.size();
}

std::vector<Vector3> Chainmail::toTorus(const std::vector<Vector3> &points, Index numTorusSegments,
                                      Index numDiameterSegments)
{
    auto middle = center(points);
    m_radiusValue = (middle - points[0]).norm();
    if (m_radius->getValue() < 0)
        setFloatParameter(m_radius->getName(), m_radiusValue);
    switch (m_geoMode->getValue()) {
    case Circle:
        return toTorusCircle(points, middle, numTorusSegments, numDiameterSegments);
    case Spline:
        return toTorusSpline(points, middle, numTorusSegments, numDiameterSegments);
    }
    return std::vector<Vector3>();
}

std::vector<Vector3> Chainmail::toTorusCircle(const std::vector<Vector3> &points, const Vector3 &middle,
                                            Index numTorusSegments, Index numDiameterSegments)
{
    assert(points.size() >= 3 && "toCircle expected 3 or 4 points");


    auto normale = (points[0] - points[1]).cross(points[0] - points[2]);
    normale.normalize();


    Vector3 v1 = points[0] - points[1];
    Vector3 v2 = v1.cross(normale);
    v1.normalize();
    v2.normalize();

    std::vector<Vector3> v;
    auto thetaBase = 2 * EIGEN_PI / numTorusSegments / numDiameterSegments;
    for (Index i = 0; i < numTorusSegments; i++) {
        auto tethaH = thetaBase * (i * numDiameterSegments);
        auto d = v1 * sin(tethaH) + v2 * cos(tethaH);
        auto p = middle + m_radius->getValue() * d;
        for (Index j = 0; j < numDiameterSegments; j++) {
            auto tethaV = 2 * EIGEN_PI * j / numDiameterSegments;

            auto pp = p + m_radius->getValue() / 20 * (normale * sin(tethaV) + d * cos(tethaV));
            v.push_back(pp);
        }
    }
    return v;
}

std::vector<Eigen::Vector3f> toCircularSpline(const Eigen::MatrixXf &points, const Eigen::MatrixXf &derivatives,
                                              const Eigen::VectorXi &derivativeIndices, Index precition)
{
    using namespace Eigen;
    //const Spline3f spline = SplineFitting<Spline3f>::Interpolate(points, 2);


    const Spline3f spline =
        SplineFitting<Spline3f>::InterpolateWithDerivatives(points, derivatives, derivativeIndices, 2);
    std::vector<Eigen::Vector3f> v(precition);
    for (vistle::Index i = 0; i < precition; i++) {
        v[i] = spline((float)i / precition);
    }
    return v;
}

std::vector<Vector3> Chainmail::toTorusSpline(const std::vector<Vector3> &points, const Vector3 &middle,
                                            Index numTorusSegments, Index numDiameterSegments)
{
    auto ps = points.size();
    Eigen::MatrixXf pointMatrix(3, ps + 1);
    Eigen::MatrixXf derivatives(3, ps + 1);
    Eigen::VectorXi derivativeIndices(ps + 1);
    for (size_t i = 0; i < ps; i++) {
        auto &currentPoint = points[i];
        auto &nextPoint = points[(i + 1) % ps];
        auto &previousPoint = points[(i + ps - 1) % ps];
        derivativeIndices(i) = i;
        //(side_1 x side_2) x radius
        derivatives.col(i) =
            (currentPoint - nextPoint).cross(currentPoint - previousPoint).cross(currentPoint - middle).normalized();
        derivativeIndices(i) = i;
        pointMatrix.col(i) = middle + (currentPoint - middle);
    }
    pointMatrix.col(ps) = pointMatrix.col(0);
    derivatives.col(ps) = derivatives.col(0);
    derivativeIndices(ps) = ps;

    auto v1 = toCircularSpline(pointMatrix, derivatives, derivativeIndices, numTorusSegments);
    //extrapolate points for the 3D torus around the spline
    std::vector<Vector3> v2;
    auto theta = 2 * EIGEN_PI / numDiameterSegments;
    for (int i = 0; i < v1.size(); i++) {
        auto currentPoint = middle + (v1[i] - middle) * m_radius->getValue() / m_radiusValue;
        Vector3 s1 = middle - currentPoint;
        Vector3 s2 = v1[(i - 1) % numTorusSegments] - v1[(i + 1) % numTorusSegments];
        s1.normalize();
        s2 = s1.cross(s2);
        s2.normalize();
        for (Index j = 0; j < numDiameterSegments; j++) {
            v2.push_back(currentPoint + m_radius->getValue() / 20 * (s1 * sin(theta * j) + s2 * cos(theta * j)));
        }
    }

    return v2;
}

bool Chainmail::compute()
{
    auto unstrIn = expect<UnstructuredGrid>("quads_in");
    if (!unstrIn) {
        sendError("no input grid");
        return true;
    }
    Index circlesPerTorus = m_numXSegments->getValue();
    Index vertsPerCircle = m_numYSegments->getValue();
    Index vertsPerTorus = circlesPerTorus * vertsPerCircle;
    Index numTori = 0;
    for (Index elementIndex = 0; elementIndex < unstrIn->getNumElements(); elementIndex++) {
        if (unstrIn->tl()[elementIndex] == UnstructuredGrid::QUAD ||
            unstrIn->tl()[elementIndex] == UnstructuredGrid::TRIANGLE)
            ++numTori;
    }

    Index numCoords = numTori * vertsPerTorus;
    auto toriOut = make_ptr<Quads>(numCoords * Index(4), numCoords, unstrIn->meta());
    toriOut->copyAttributes(unstrIn);
    Index currentQuadIndex = 0;
    for (Index elementIndex = 0; elementIndex < unstrIn->getNumElements(); elementIndex++) {
        if (unstrIn->tl()[elementIndex] == UnstructuredGrid::QUAD ||
            unstrIn->tl()[elementIndex] == UnstructuredGrid::TRIANGLE) {
            auto numVerts = UnstructuredGrid::NumVertices[unstrIn->tl()[elementIndex]];
            std::vector<Vector3> points(numVerts);
            for (Index p = 0; p < numVerts; p++) {
                points[p] = vec(unstrIn, unstrIn->cl()[unstrIn->el()[elementIndex] + p]);
            }
            auto torusPoints = toTorus(points, circlesPerTorus, vertsPerCircle);
            assert(torusPoints.size() == vertsPerTorus);
            for (Index i = 0; i < torusPoints.size(); i++) {
                toriOut->x()[currentQuadIndex + i] = torusPoints[i](0);
                toriOut->y()[currentQuadIndex + i] = torusPoints[i](1);
                toriOut->z()[currentQuadIndex + i] = torusPoints[i](2);
            }
            currentQuadIndex += vertsPerTorus;
        }
    }
    for (Index torus = 0; torus < numTori; torus++) {
        for (Index circle = 0; circle < circlesPerTorus; circle++) {
            for (Index point = 0; point < vertsPerCircle; point++) {
                auto p1 = torus * circlesPerTorus * vertsPerCircle + circle * vertsPerCircle + point;
                auto p2 = torus * circlesPerTorus * vertsPerCircle + ((circle + 1) % circlesPerTorus) * vertsPerCircle +
                          point;
                auto p3 = torus * circlesPerTorus * vertsPerCircle + ((circle + 1) % circlesPerTorus) * vertsPerCircle +
                          (point + 1) % vertsPerCircle;
                auto p4 =
                    torus * circlesPerTorus * vertsPerCircle + circle * vertsPerCircle + (point + 1) % vertsPerCircle;
                toriOut->cl()[4 * p1] = p1;
                toriOut->cl()[4 * p1 + 1] = p2;
                toriOut->cl()[4 * p1 + 2] = p3;
                toriOut->cl()[4 * p1 + 3] = p4;
                // std::cerr << "torus " << torus << ", circle " << circle << ", point " << point << " is connceted to:" << std::endl;
                // std::cerr << "[" << ringsOut->cl()[4 * p1] << ", " << ringsOut->cl()[4*p1 + 1] << ", " << ringsOut->cl()[4*p1 +2] << ", " << ringsOut->cl()[4*p1 +3] << std::endl;
            }
        }
    }
    auto normals = make_ptr<Normals>(numCoords);
    for (Index i = 0; i < numCoords; i++) {
        std::array<Index, 4> indices{
            Index((i / vertsPerCircle) * vertsPerCircle + (i + 1) % vertsPerCircle),
            Index((i / vertsPerCircle) * vertsPerCircle + (i - 1) % vertsPerCircle),
            Index((i / vertsPerTorus) * vertsPerTorus + (i + vertsPerCircle) % (vertsPerTorus)),
            Index((i / vertsPerTorus) * vertsPerTorus + (i - vertsPerCircle) % (vertsPerTorus))};

        Vector3 n{0, 0, 0};
        auto currentVec = vec(toriOut, i);
        for (size_t j = 0; j < 4; j++) {
            Vector3 nn = currentVec - vec(toriOut, indices[j]);
            nn.normalize();
            n += nn;
        }
        // std::cerr << "neigbors for vert " << i << " are :";
        // for (size_t k = 0; k < 4; k++) {
        //     std::cerr << indices[k] << ", ";
        // }
        // std::cerr << std::endl;
        // std::cerr << "normale = " << n << std::endl << std::endl;
        normals->x()[i] = n.x();
        normals->y()[i] = n.y();
        normals->z()[i] = n.z();
    }
    updateMeta(normals);
    toriOut->setNormals(normals);
    updateMeta(toriOut);

    std::cerr << "circlesOut numElements " << toriOut->getNumElements() << ", numCorners " << toriOut->getNumCorners()
              << ", numVerts " << toriOut->getNumVertices() << std::endl;
    std::cerr << "circlesOut->cl()[circlesOut->getNumCorners() - 1] = " << toriOut->cl()[toriOut->getNumCorners() - 1]
              << std::endl;

    addObject("circles_out", toriOut);
    return true;
}

#if 0
std::vector<std::array<float, 3>> generate_points_on_spline(std::array<float, 3> point1, std::array<float, 3> point2,
                                                            std::array<float, 3> point3, std::array<float, 3> point4,
                                                            int number_of_points)
{
    std::vector<std::array<float, 3>> points_on_path;
    std::vector<std::array<float, 4>> coefficients;
    std::array<float, 4> coeff;

    // coefficients for the spline
    coeff = {-point1[0] + 3 * point2[0] - 3 * point3[0] + point4[0], 3 * point1[0] - 6 * point2[0] + 3 * point3[0],
             -3 * point1[0] + 3 * point2[0], point1[0]};
    coefficients.push_back(coeff);
    coeff = {-point1[1] + 3 * point2[1] - 3 * point3[1] + point4[1], 3 * point1[1] - 6 * point2[1] + 3 * point3[1],
             -3 * point1[1] + 3 * point2[1], point1[1]};
    coefficients.push_back(coeff);
    coeff = {-point1[2] + 3 * point2[2] - 3 * point3[2] + point4[2], 3 * point1[2] - 6 * point2[2] + 3 * point3[2],
             -3 * point1[2] + 3 * point2[2], point1[2]};
    coefficients.push_back(coeff);

    for (int i = 0; i < number_of_points; i++) {
        float t = (float)i / number_of_points;
        float x = coefficients[0][0] * pow(t, 3) + coefficients[0][1] * pow(t, 2) + coefficients[0][2] * t +
                  coefficients[0][3];
        float y = coefficients[1][0] * pow(t, 3) + coefficients[1][1] * pow(t, 2) + coefficients[1][2] * t +
                  coefficients[1][3];
        float z = coefficients[2][0] * pow(t, 3) + coefficients[2][1] * pow(t, 2) + coefficients[2][2] * t +
                  coefficients[2][3];
        points_on_path.push_back({x, y, z});
    }
    return points_on_path;
}

std::vector<Eigen::Vector3f> quadToCircleChatGpt(const Eigen::MatrixXf &points, Index precition)
{
    std::array<float, 3> point1 = {points(0, 0), points(1, 0), points(2, 0)};
    std::array<float, 3> point2 = {points(0, 1), points(1, 1), points(2, 1)};
    std::array<float, 3> point3 = {points(0, 2), points(1, 2), points(2, 2)};
    std::array<float, 3> point4 = {points(0, 3), points(1, 3), points(2, 3)};
    auto circlePoints = generate_points_on_spline(point1, point2, point3, point4, precition);
    std::vector<Eigen::Vector3f> v;
    for (const auto &point: circlePoints)
        v.push_back(Eigen::Vector3f{point[0], point[1], point[2]});
    return v;
}


#endif
