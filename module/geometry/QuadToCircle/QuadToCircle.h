#ifndef QUADSTOCIRCLES_H
#define QUADSTOCIRCLES_H

#include <vistle/module/module.h>
#include <array>
#include <vector>
class QuadToCircle: public vistle::Module {
public:
    QuadToCircle(const std::string &name, int moduleID, mpi::communicator comm);
    ~QuadToCircle();

private:
    virtual bool compute();
    std::vector<Eigen::Vector3f> quadToCircle(const Eigen::MatrixXf &points, size_t precition);

    vistle::FloatParameter *m_radius;
    vistle::IntParameter *m_precition;
    vistle::IntParameter *m_numKnots;
    std::array<vistle::IntParameter *, 5> m_knots;
    float m_radiusValue = -1.0;
};

#endif // QUADSTOCIRCLES_H
