#include "SpheresOverlap.h"

using namespace vistle;

MODULE_MAIN(SpheresOverlap)

/*
    TODO: 
    - maybe rename to SphereNetwork? -> also change port descriptions
*/
SpheresOverlap::SpheresOverlap(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    m_spheresIn = createInputPort("spheres_in", "spheres for which overlap will be calculated");
    m_linesOut = createOutputPort("lines_out", "lines between all overlapping spheres");
}

SpheresOverlap::~SpheresOverlap()
{}

bool SpheresOverlap::compute(const std::shared_ptr<BlockTask> &task) const
{
    return true;
}
