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
    createInputPort("grid_in");
    createOutputPort("grid_out");

    p_facestream =
        addIntParameter("facestream", "create facestream (VTK) or repeat initial vertex of polygon faces (COVISE)",
                        UnstructuredGrid::POLYHEDRON == UnstructuredGrid::VPOLYHEDRON, Parameter::Boolean);
}

ToPolyhedra::~ToPolyhedra()
{}


bool ToPolyhedra::compute()
{
    bool facestream = p_facestream->getValue();

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
        Byte t = itl[elem] & UnstructuredGrid::TYPE_MASK;
        if (facestream) {
            otl[elem] = UnstructuredGrid::VPOLYHEDRON | (itl[elem] & ~UnstructuredGrid::TYPE_MASK);
            // create VTK polyhedra (lists of faces, where each face starts with the number of vertices followed by its vertices)
            if (t == UnstructuredGrid::CPOLYHEDRON) {
                const Index begin = iel[elem], end = iel[elem + 1];
                Index facestart = InvalidIndex;
                Index term = 0;
                for (Index i = begin; i < end; ++i) {
                    if (facestart == InvalidIndex) {
                        facestart = i;
                        term = icl[i];
                    } else if (icl[i] == term) {
                        const Index N = i - facestart;
                        ocl.push_back(N);
                        for (Index j = 0; j < N; ++j) {
                            ocl.push_back(icl[facestart + j]);
                        }
                        facestart = InvalidIndex;
                    }
                }
            } else if (t == UnstructuredGrid::VPOLYHEDRON) {
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
                        ocl.push_back(facesize);
                        for (unsigned k = 0; k < facesize; ++k) {
                            ocl.push_back(icl[begin + face[k]]);
                        }
                    }
                    break;
                }
                default:
                    std::cerr << "unhandled cell type " << int(t) << std::endl;
                }
            }
        } else {
            otl[elem] = UnstructuredGrid::CPOLYHEDRON | (itl[elem] & ~UnstructuredGrid::TYPE_MASK);
            // create COVISE polyhedra (lists of faces, where each face is terminated by repeating its first vertex)
            if (t == UnstructuredGrid::VPOLYHEDRON) {
                const Index begin = iel[elem], end = iel[elem + 1];
                for (Index i = begin; i < end; ++i) {
                    const Index N = icl[i];
                    Index facestart = i + 1;
                    for (Index j = 0; j < N; ++j) {
                        ocl.push_back(icl[facestart + j]);
                    }
                    ocl.push_back(icl[facestart]);
                    i += N;
                }
            } else if (t == UnstructuredGrid::CPOLYHEDRON) {
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
