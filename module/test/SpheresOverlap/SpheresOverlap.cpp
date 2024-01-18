
#include <math.h>

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
}

SpheresOverlap::~SpheresOverlap()
{}

bool SpheresOverlap::compute(const std::shared_ptr<BlockTask> &task) const
{
    auto spheres = task->expect<Spheres>("spheres_in");
    if (!spheres) {
        sendError("input port expects spheres");
        return true;
    }

    auto xSpheres = spheres->x();
    auto ySpheres = spheres->y();
    auto zSpheres = spheres->z();

    auto radii = spheres->r();

    std::vector<std::pair<Index, Index>> overlaps;

    for (Index i = 0; i < spheres->getNumCoords(); i++) {
        for (Index j = 0; j < spheres->getNumCoords(); j++) {
            if (i < j) {
                // no need to compute sqrt
                auto distance = pow((xSpheres[i] - xSpheres[j]), 2) + pow((ySpheres[i] - ySpheres[j]), 2) +
                                pow((zSpheres[i] - zSpheres[j]), 2);
                // sphere overlap if euclidean distance between centers is <= sum of radii
                if (distance <= pow(radii[i] + radii[j], 2)) {
                    overlaps.push_back({i, j});
                }
            }
        }
    }

    Index nrOverlaps = overlaps.size();

    Lines::ptr lines(new Lines(nrOverlaps, 2 * nrOverlaps, 2 * nrOverlaps));

    auto &xLines = lines->x();
    auto &yLines = lines->y();
    auto &zLines = lines->z();

    auto &clLines = lines->cl();
    auto &elLines = lines->el();
    elLines[0] = 0;

    for (Index i = 0; i < nrOverlaps; i++) {
        // draw line between spheres
        auto lhs = overlaps[i].first;
        auto rhs = overlaps[i].second;

        xLines[2 * i] = xSpheres[lhs];
        yLines[2 * i] = ySpheres[lhs];
        zLines[2 * i] = zSpheres[lhs];

        xLines[2 * i + 1] = xSpheres[rhs];
        yLines[2 * i + 1] = ySpheres[rhs];
        zLines[2 * i + 1] = zSpheres[rhs];


        clLines[2 * i] = 2 * i;
        clLines[2 * i + 1] = 2 * i + 1;

        elLines[i + 1] = 2 * (i + 1);
    }
    if (lines) {
        updateMeta(lines);
        task->addObject(m_linesOut, lines);
    }

    return true;
}
