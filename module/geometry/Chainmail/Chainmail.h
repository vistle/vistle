#ifndef QUADSTOCIRCLES_H
#define QUADSTOCIRCLES_H

#include <vistle/module/module.h>
#include <array>
#include <vector>
#include <vistle/core/vectortypes.h>

/*
Extrapolates rings from quads and triangles of an uniform grid
*/

class Chainmail: public vistle::Module {
public:
    Chainmail(const std::string &name, int moduleID, mpi::communicator comm);
    ~Chainmail();

private:
    virtual bool compute();
    std::vector<vistle::Vector3> toTorus(const std::vector<vistle::Vector3> &points, vistle::Index numTorusSegments,
                                         vistle::Index numDiameterSegments);
    std::vector<vistle::Vector3> toTorusCircle(const std::vector<vistle::Vector3> &points,
                                               const vistle::Vector3 &middle, vistle::Index numTorusSegments,
                                               vistle::Index numDiameterSegments);
    std::vector<vistle::Vector3> toTorusSpline(const std::vector<vistle::Vector3> &points,
                                               const vistle::Vector3 &middle, vistle::Index numTorusSegments,
                                               vistle::Index numDiameterSegments);

    vistle::Port* m_circlesOut = nullptr;
    vistle::IntParameter *m_geoMode; 
    vistle::FloatParameter *m_radius; //the radius of the rings in the qua
    vistle::IntParameter *m_numXSegments; //number of segemnts in the quad/triangle plane
    vistle::IntParameter *m_numYSegments; //number of segments to crate a tube around the torus center line

    float m_radiusValue = 1.0;
};

#endif // QUADSTOCIRCLES_H
