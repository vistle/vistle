#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/coords.h>
#include <vistle/core/points.h>
#include <vistle/core/spheres.h>
#include <vistle/core/lines.h>
#include <vistle/core/tubes.h>
#include <vistle/util/math.h>
#include <vistle/core/unstr.h>
#include <vistle/core/quads.h>
#include <vistle/core/polygons.h>
#include <cmath>
#include <tuple>

#include "QuadToCircle.h"
#include <eigen3/unsupported/Eigen/src/Splines/Spline.h>
#include <eigen3/unsupported/Eigen/src/Splines/SplineFitting.h>


MODULE_MAIN(QuadToCircle)


using namespace vistle;

QuadToCircle::QuadToCircle(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    createInputPort("quads_in", "Unstructured grid with quads");
    createInputPort("data_in", "mapped data");
    createOutputPort("circles_out", "circles");
    createOutputPort("data_out", "tubes or spheres with mapped data");

    m_radius = addFloatParameter("radius", "radius to create a torus", -1.0);
    m_precition = addIntParameter("precition", "number of lines/triangles extrapolated to create circle/torus", 10);
    m_numKnots = addIntParameter("numKnots", "", 5);
    static const std::array<int, 5> initialKnotValues{0, 10, 20, 30, 39};
    for (size_t i = 0; i < 5; i++) {
        m_knots[i] = addIntParameter("knot_" + std::to_string(i), "", initialKnotValues[i]);
    }
}

QuadToCircle::~QuadToCircle()
{}


std::vector<Eigen::Vector3f> quadToCircleEigen(const Eigen::MatrixXf &points, size_t precition,
                                               const Eigen::Vector<float, 5> &knots)
{
    using namespace Eigen;
    const Spline3f spline = SplineFitting<Spline3f>::Interpolate(points, 3, knots);
    std::vector<Eigen::Vector3f> v(precition + 1);
    for (size_t i = 0; i <= precition; i++) {
        v[i] = spline((float)i / precition);
    }
    return v;
}

Eigen::Vector3f getMiddle(const Eigen::MatrixXf &points)
{
    if (points.rows() != 3 || points.cols() < 4) {
        std::cerr << "getMiddle requires 4 points, " << points.rows() << " rows and " << points.cols()
                  << " columns given!";
        return Eigen::Vector3f{};
    }
    auto p1 = points.col(0);
    auto p2 = points.col(1);
    auto p3 = points.col(2);
    auto p4 = points.col(3);
    std::cerr << "middle: " << p1 + (p3 - p1) / 2 << " = " << p2 + (p4 - p2) / 2 << std::endl;
    return p1 + (p3 - p1) / 2;
}


std::vector<Eigen::Vector3f> QuadToCircle::quadToCircle(const Eigen::MatrixXf &points, size_t precition)
{
    auto middle = getMiddle(points);
    std::vector<Eigen::Vector3f> v;
    vistle::Vector3 p1 = points.col(0);
    vistle::Vector3 p2 = points.col(1);
    vistle::Vector3 p3 = points.col(2);
    vistle::Vector3 p4 = points.col(3);
    vistle::Vector3 side1 = p1 - p2;
    vistle::Vector3 side = p1 - p3;
    vistle::Vector3 normal = side1.cross(side);
    vistle::Vector3 side2 = normal.cross(side1);

    auto t1 = side1(0);
    auto t2 = side2(0);
    side1.normalize();
    side2.normalize();
    if (m_radius->getValue() < 0)
        setFloatParameter(m_radius->getName(), t1 / side1(0) + t2 / side2(0));

    for (size_t i = 0; i < precition; i++) {
        auto tetha = 2 * EIGEN_PI * i / precition;
        v.push_back(middle + m_radius->getValue() * (side1 * sin(tetha) + side2 * cos(tetha)));
    }

    return v;
}

bool QuadToCircle::compute()
{
    auto unstrIn = expect<UnstructuredGrid>("quads_in");
    if (!unstrIn) {
        sendError("no input grid");
        return true;
    }
    auto pointsPerCircle = m_precition->getValue();
    std::vector<Eigen::Vector3f> circlePoints;
    for (size_t elementIndex = 0; elementIndex < unstrIn->getNumElements(); elementIndex++) {
        if (unstrIn->tl()[elementIndex] == UnstructuredGrid::QUAD) {
            Eigen::MatrixXf points(3, 5);
            for (size_t p = 0; p < 4; p++) {
                points(0, p) = unstrIn->x()[unstrIn->cl()[unstrIn->el()[elementIndex] + p]];
                points(1, p) = unstrIn->y()[unstrIn->cl()[unstrIn->el()[elementIndex] + p]];
                points(2, p) = unstrIn->z()[unstrIn->cl()[unstrIn->el()[elementIndex] + p]];
            }
            points(0, 4) = unstrIn->x()[unstrIn->cl()[unstrIn->el()[elementIndex]]];
            points(1, 4) = unstrIn->y()[unstrIn->cl()[unstrIn->el()[elementIndex]]];
            points(2, 4) = unstrIn->z()[unstrIn->cl()[unstrIn->el()[elementIndex]]];
            Eigen::Vector<float, 5> knots;
            for (size_t i = 0; i < 5; i++) {
                knots(i) = m_knots[i]->getValue();
            }

            knots = knots / knots[4];

            auto cps = quadToCircle(points, pointsPerCircle);
            //auto cps = quadToCircleEigen(points, pointsPerCircle - 1, knots);
            assert(cps.size() == pointsPerCircle);
            circlePoints.insert(circlePoints.end(), cps.begin(), cps.end());
        }
    }
    auto numElements = circlePoints.size() / pointsPerCircle;
    //const size_t numElements, const size_t numCorners, const size_t numVertices
    auto circlesOut =
        make_ptr<Polygons>(numElements, numElements + circlePoints.size(), circlePoints.size(), unstrIn->meta());
    circlesOut->copyAttributes(unstrIn);
    for (size_t i = 0; i < circlePoints.size(); i++) {
        circlesOut->x()[i] = circlePoints[i](0);
        circlesOut->y()[i] = circlePoints[i](1);
        circlesOut->z()[i] = circlePoints[i](2);
    }
    for (size_t i = 0; i < numElements; i++) {
        for (size_t j = 0; j < pointsPerCircle; j++) {
            circlesOut->cl()[i * (pointsPerCircle + 1) + j] = i * pointsPerCircle + j;
        }
        circlesOut->cl()[i * (pointsPerCircle + 1) + pointsPerCircle] =
            i * pointsPerCircle; //point back to the beginning
        circlesOut->el()[i] = i * (pointsPerCircle + 1);
    }
    circlesOut->el()[numElements] = numElements * (pointsPerCircle + 1);
    updateMeta(circlesOut);

    std::cerr << "circlesOut numElements " << circlesOut->getNumElements() << ", numCorners "
              << circlesOut->getNumCorners() << ", numVerts " << circlesOut->getNumVertices() << std::endl;
    std::cerr << "circlesOut->cl()[circlesOut->getNumCorners() - 1] = "
              << circlesOut->cl()[circlesOut->getNumCorners() - 1] << std::endl;

    addObject("circles_out", circlesOut);
    return true;
}


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

std::vector<Eigen::Vector3f> quadToCircleChatGpt(const Eigen::MatrixXf &points, size_t precition)
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