#include "surfacetools.h"

namespace vistle {
void addSurfaceFaces(const Polygons::ptr poly, const UnstructuredGrid::ptr &ugrid)
{
    const auto &nf = ugrid->getNeighborFinder();
    const auto &tl = ugrid->tl().data();
    const auto &el = ugrid->el().data();
    const auto &cl = ugrid->cl().data();
    auto &pl = poly->el();
    auto &pcl = poly->cl();
    for (Index i = 0; i < ugrid->getSize(); ++i) {
        Byte t = tl[i];
        const Index elStart = el[i];
        const auto numFaces = UnstructuredGrid::NumFaces[t];
        const auto &faces = UnstructuredGrid::FaceVertices[t];
        for (int f = 0; f < numFaces; ++f) {
            const auto &face = faces[f];
            Index neighbour = 0;
            if (UnstructuredGrid::Dimensionality[t] == 3)
                neighbour =
                    nf.getNeighborElement(i, cl[elStart + face[0]], cl[elStart + face[1]], cl[elStart + face[2]]);
            if (UnstructuredGrid::Dimensionality[t] == 2 || neighbour == InvalidIndex) {
                const auto facesize = UnstructuredGrid::FaceSizes[t][f];
                for (unsigned j = 0; j < facesize; ++j) {
                    pcl.push_back(cl[elStart + face[j]]);
                }
                pl.push_back(pcl.size());
            }
        }
    }
}
} // namespace vistle
