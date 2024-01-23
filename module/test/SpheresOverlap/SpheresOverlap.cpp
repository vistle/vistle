
#include <math.h>

#include <vistle/alg/objalg.h>
#include <vistle/core/lines.h>
#include <vistle/core/spheres.h>

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

    setReducePolicy(message::ReducePolicy::OverAll);
}

SpheresOverlap::~SpheresOverlap()
{}

bool SpheresOverlap::compute(const std::shared_ptr<BlockTask> &task) const
{
    auto dataIn = task->expect<Object>("spheres_in");
    auto splitDataIn = splitContainerObject(dataIn);

    if (!Spheres::as(splitDataIn.geometry)) {
        sendError("input port expects spheres");
        return true;
    }
    
    auto spheres = Spheres::as(splitDataIn.geometry);

    auto xSpheres = spheres->x();
    auto ySpheres = spheres->y();
    auto zSpheres = spheres->z();

    auto radii = spheres->r();

    // lines connecting overlapping spheres
    Lines::ptr lines(new Lines(0, 0, 0));

    auto &xLines = lines->x();
    auto &yLines = lines->y();
    auto &zLines = lines->z();

    auto &clLines = lines->cl();
    auto &elLines = lines->el();
    elLines.push_back(0);

    Index numCoordsLines = 0;

    for (Index i = 0; i < spheres->getNumCoords(); i++) {
        for (Index j = 0; j < spheres->getNumCoords(); j++) {
            if (i < j) {
                // no need to compute sqrt for euclidean distance
                auto distance = pow((xSpheres[i] - xSpheres[j]), 2) + pow((ySpheres[i] - ySpheres[j]), 2) +
                                pow((zSpheres[i] - zSpheres[j]), 2);
                // spheres overlap if euclidean distance between centers is <= sum of radii
                if (distance <= pow(radii[i] + radii[j], 2)) {
                    xLines.push_back(xSpheres[i]);
                    yLines.push_back(ySpheres[i]);
                    zLines.push_back(zSpheres[i]);

                    xLines.push_back(xSpheres[j]);
                    yLines.push_back(ySpheres[j]);
                    zLines.push_back(zSpheres[j]);

                    clLines.push_back(numCoordsLines++);
                    clLines.push_back(numCoordsLines++);

                    elLines.push_back(numCoordsLines);
                }
            }
        }
    }

    if (numCoordsLines) {
        lines->copyAttributes(spheres);
        updateMeta(lines);
        task->addObject(m_linesOut, lines);
    }

    return true;
}
