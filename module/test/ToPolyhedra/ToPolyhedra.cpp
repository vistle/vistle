#include <sstream>
#include <iomanip>

#include <boost/mpl/for_each.hpp>

#include <vistle/core/object.h>
#include <vistle/core/unstr.h>

#include "ToPolyhedra.h"

MODULE_MAIN(ToPolyhedra)

using namespace vistle;

ToPolyhedra::ToPolyhedra(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    createInputPort("grid_in", "unstructured grid");
    createOutputPort("grid_out", "unstructured grid with tesselated polyhedra");
}

ToPolyhedra::~ToPolyhedra()
{}


bool ToPolyhedra::compute()
{
    auto data = expect<DataBase>("grid_in");
    if (!data) {
        sendError("input does not an unstructured grid and does not have a grid");
        return true;
    }
    auto obj = data->grid();
    if (!obj) {
        obj = data;
        data.reset();
    }

    auto grid = UnstructuredGrid::as(obj);
    if (!grid) {
        sendError("input does not have an unstructured grid");
        return true;
    }

    const Index *iel = &grid->el()[0];
    const Index *icl = &grid->cl()[0];
    const Byte *itl = &grid->tl()[0];
    const Index nel = grid->getNumElements();
    auto poly = std::make_shared<UnstructuredGrid>(nel, 0, 0);
    poly->d()->x[0] = grid->d()->x[0];
    poly->d()->x[1] = grid->d()->x[1];
    poly->d()->x[2] = grid->d()->x[2];
    Index *oel = &poly->el()[0];
    Byte *otl = &poly->tl()[0];
    auto &ocl = poly->cl();
    for (Index elem = 0; elem < nel; ++elem) {
        oel[elem] = ocl.size();
        Byte t = itl[elem];
        otl[elem] = UnstructuredGrid::POLYHEDRON;
        poly->setGhost(elem, grid->isGhost(elem));
        poly->setConvex(elem, grid->isConvex(elem));
        // create COVISE polyhedra (lists of faces, where each face is terminated by repeating its first vertex)
        if (t == UnstructuredGrid::POLYHEDRON) {
            const Index begin = iel[elem], end = iel[elem + 1];
            for (Index i = begin; i < end; ++i) {
                ocl.push_back(icl[i]);
            }
        } else {
            switch (t) {
            case UnstructuredGrid::TETRAHEDRON:
            case UnstructuredGrid::PYRAMID:
            case UnstructuredGrid::PRISM:
            case UnstructuredGrid::HEXAHEDRON: {
                const auto numFaces = UnstructuredGrid::NumFaces[t];
                const auto &faces = UnstructuredGrid::FaceVertices[t];
                const Index begin = iel[elem];
                for (int f = 0; f < numFaces; ++f) {
                    const auto &face = faces[f];
                    const auto facesize = UnstructuredGrid::FaceSizes[t][f];
                    for (unsigned k = 0; k < facesize; ++k) {
                        ocl.push_back(icl[begin + face[k]]);
                    }
                    ocl.push_back(icl[begin + face[0]]);
                }
                break;
            }
            default:
                std::cerr << "unhandled cell type " << int(t) << std::endl;
            }
        }
    }
    oel[nel] = ocl.size();

    if (poly) {
        poly->setMeta(obj->meta());
        poly->copyAttributes(obj);
        updateMeta(poly);
    }

    if (data) {
        auto ndata = data->clone();
        ndata->setMeta(data->meta());
        ndata->copyAttributes(data);
        ndata->setGrid(poly);
        updateMeta(ndata);
        addObject("grid_out", ndata);
    } else {
        addObject("grid_out", poly);
    }

    return true;
}
