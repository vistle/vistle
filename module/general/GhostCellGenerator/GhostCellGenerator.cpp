/**************************************************************************\
 **                                                                      **
 **                                                                      **
 ** Description: GhostCellGenerator for unstructured grids.              **
 **                                                                      **
 ** Based on paper: Parallel Multi-Layer Ghost Cell Generation for       **
 **                 Distributed UnstructuredGrid Grids.                  **
 ** DOI: 10.1109/LDAV.2017.8231854                                       **
 **                                                                      **
 **                                                                      **

 TODO:
 [x] 1. Extract external boundary of local partition
    => Domain Surface.
 [ ] 2. Share boundary with potential partition neighbors
    => share with every partition
    [ ] optimize later.
 [ ] 3. Calculate actual partition neighbors
 [ ] 4. create cell list to send to each partition
 [ ] 5. send cells to partition neighbors.
 [ ] 6. receive cells from partition.
 [ ] 7. integrate cells into local partition.

 **                                                                      **
 **                                                                      **
 **                                                                      **
 ** Author:    Marko Djuric                                              **
 **                                                                      **
 **                                                                      **
 **                                                                      **
 ** Date:  10.05.2021                                                    **
\**************************************************************************/

#include <boost/mpi.hpp>
#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/vec.h>
#include <vistle/core/unstr.h>
#include <vistle/core/polygons.h>
#include <vistle/core/structuredgrid.h>

#include "GhostCellGenerator.h"

using namespace vistle;

GhostCellGenerator::GhostCellGenerator(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    createInputPort("data_in", "input grid");
    createOutputPort("data_out", "grid with added ghost/halo cells");
}

GhostCellGenerator::~GhostCellGenerator()
{}

void checkData(UnstructuredGrid::const_ptr ugrid)
{}

bool GhostCellGenerator::compute(const std::shared_ptr<BlockTask> &task) const
{
    DataBase::const_ptr data;
    Polygons::const_ptr surface;
    surface = task->accept<Polygons>("data_in");
    if (!surface) {
        data = task->expect<DataBase>("data_in");
        if (!data) {
            sendError("no grid and no data received");
            return true;
        }
        if (!data->grid()) {
            sendError("no grid attached to data");
            return true;
        }
        surface = Polygons::as(data->grid());
        if (!surface) {
            sendError("no valid grid attached to data");
            return true;
        }
    }
    Object::const_ptr surface_in = surface;
    assert(surface_in);

#if 0
    bool haveElementData = false;
    if (data && data->guessMapping(surface_in) == DataBase::Element) {
        haveElementData = true;
    }
#endif

    //gather all domainsurfaces => at the moment all to all comm
    /* std::vector<Polygons::const_ptr> surfaceNeighbors; */
    /* b_mpi::all_gather(comm(), surface, surfaceNeighbors); */
    /* MPI_Barrier(comm()); */


    /* const auto num_elem = surface->getNumElements(); */
    /* const auto num_corners = surface->getNumCorners(); */
    /* const auto num_vertices = surface->getNumVertices(); */

    /* int proc{0}; */
    /* for (auto surface_neighbor: surfaceNeighbors) { */
    /*     /1*    _ _ _ _             _ _ _ _ */
    /*          /_/_/_/_/|         /_/_/_/_/               /| */
    /*         /_/_/_/_/ |        /_/_/_/_/               / | */
    /*        /_/_/_/_/  |       /_/_/_/_/               /  | */
    /*       /_/_/_/_/   |      /_/_/_/_/    _ _ _ _    /   | */
    /*       |_|_|_|_|   / =>               |_|_|_|_|   |   / */
    /*       |_|_|_|_|  /                   |_|_|_|_|   |  / */
    /*       |_|_|_|_| /                    |_|_|_|_|   | / */
    /*       |_|_|_|_|/                     |_|_|_|_|   |/   *1/ */

    /*     ++proc; */
    /*     std::vector<int> neighbors; */

    /*     //extract neighbors via vertices + send back to proc */
    /*     const auto n_num_elem = surface_neighbor->getNumElements(); */
    /*     const auto n_num_corners = surface_neighbor->getNumCorners(); */
    /*     const auto n_num_vertices = surface_neighbor->getNumVertices(); */

    /*     Polygons::ptr poly_build{ */
    /*         new Polygons(num_elem + n_num_elem, num_corners + n_num_corners, num_vertices + n_num_vertices)}; */

    /* const Index *el = &surface->el()[0]; */
    /* const Index *cl = &surface->cl()[0]; */
    /* Polygons::VertexOwnerList::const_ptr poly_vol = surface->getVertexOwnerList(); */

    /* Polygons::ptr m_grid_out(new Polygons(0, 0, 0)); */
    /* auto &pl = m_grid_out->el(); */
    /* auto &pcl = m_grid_out->cl(); */

    /* auto nf = m_grid_in->getNeighborFinder(); */
    /* for (Index i=0; i<num_elem; ++i) { */
    /*    const Index elStart = el[i], elEnd = el[i+1]; */
    /*    bool ghost = tl[i] & UnstructuredGrid::GHOST_BIT; */
    /*    if (!showgho && ghost) */
    /*        continue; */
    /*    Byte t = tl[i] & UnstructuredGrid::TYPE_MASK; */
    /*    if (t == UnstructuredGrid::POLYHEDRON) { */
    /*        if (showpol) { */
    /*            Index facestart = InvalidIndex; */
    /*            Index term = 0; */
    /*            for (Index j=elStart; j<elEnd; ++j) { */
    /*                if (facestart == InvalidIndex) { */
    /*                    facestart = j; */
    /*                    term = cl[j]; */
    /*                } else if (cl[j] == term) { */
    /*                    Index numVert = j - facestart; */
    /*                    if (numVert >= 3) { */
    /*                        auto face = &cl[facestart]; */
    /*                        Index neighbour = nf.getNeighborElement(i, face[0], face[1], face[2]); */
    /*                        if (neighbour == InvalidIndex) { */
    /*                            const Index *begin = &face[0], *end=&face[numVert]; */
    /*                            auto rbegin = std::reverse_iterator<const Index *>(end), rend = std::reverse_iterator<const Index *>(begin); */
    /*                            std::copy(rbegin, rend, std::back_inserter(pcl)); */
    /*                            if (haveElementData) */
    /*                                em.emplace_back(i); */
    /*                            pl.push_back(pcl.size()); */
    /*                        } */
    /*                    } */
    /*                    facestart = InvalidIndex; */
    /*                } */
    /*            } */
    /*        } */
    /*    } else { */
    /*        bool show = false; */
    /*        switch(t) { */
    /*        case UnstructuredGrid::PYRAMID: */
    /*            show = showpyr; */
    /*            break; */
    /*        case UnstructuredGrid::PRISM: */
    /*            show = showpri; */
    /*            break; */
    /*        case UnstructuredGrid::TETRAHEDRON: */
    /*            show = showtet; */
    /*            break; */
    /*        case UnstructuredGrid::HEXAHEDRON: */
    /*            show = showhex; */
    /*            break; */
    /*        case UnstructuredGrid::TRIANGLE: */
    /*            show = showtri; */
    /*            break; */
    /*        case UnstructuredGrid::QUAD: */
    /*            show = showqua; */
    /*            break; */
    /*        default: */
    /*            break; */
    /*        } */

    /*        if (show) { */
    /*          const auto numFaces = UnstructuredGrid::NumFaces[t]; */
    /*          const auto &faces = UnstructuredGrid::FaceVertices[t]; */
    /*          for (int f=0; f<numFaces; ++f) { */
    /*             const auto &face = faces[f]; */
    /*             Index neighbour = 0; */
    /*             if (UnstructuredGrid::Dimensionality[t] == 3) */
    /*                 neighbour = nf.getNeighborElement(i, cl[elStart + face[0]], cl[elStart + face[1]], cl[elStart + face[2]]); */
    /*             if (UnstructuredGrid::Dimensionality[t] == 2 || neighbour == InvalidIndex) { */
    /*                const auto facesize = UnstructuredGrid::FaceSizes[t][f]; */
    /*                for (unsigned j=0;j<facesize;++j) { */
    /*                   pcl.push_back(cl[elStart + face[j]]); */
    /*                } */
    /*                if (haveElementData) */
    /*                    em.emplace_back(i); */
    /*                pl.push_back(pcl.size()); */
    /*             } */
    /*          } */
    /*       } */
    /*    } */
    /* } */

    /* if (m_grid_out->getNumElements() == 0) { */
    /*    return Polygons::ptr(); */
    /* } */

    /* return m_grid_out; */
    /* } */
    /* Object::ptr surface; */
    /* broadcastObject(comm(), ugrid, ); */
    /* DataMapping vm; */
    /* DataMapping em; */
    /* if (ugrid) { */
    /*     auto poly = createSurface(ugrid, em, haveElementData); */
    /*     surface = poly; */
    /*     if (!poly) */
    /*         return true; */
    /*     renumberVertices(ugrid, poly, vm); */
    /* } else if (sgrid) { */
    /*     auto quad = createSurface(sgrid, em, haveElementData); */
    /*     surface = quad; */
    /*     if (!quad) */
    /*         return true; */
    /*     if (auto coords = Coords::as(grid_in)) { */
    /*         renumberVertices(coords, quad, vm); */
    /*     } else { */
    /*         createVertices(sgrid, quad, vm); */
    /*     } */
    /* } */

    /* surface->setMeta(grid_in->meta()); */
    /* surface->copyAttributes(grid_in); */

    /* if (!data) { */
    /*     updateMeta(surface); */
    /*     task->addObject("data_out", surface); */
    /*     return true; */
    /* } */

    /* if (!haveElementData && data->guessMapping(grid_in) != DataBase::Vertex) { */
    /*     sendError("data mapping not per vertex and not per element"); */
    /*     return true; */
    /* } */

    /* if (!haveElementData && vm.empty()) { */
    /*     DataBase::ptr dout = data->clone(); */
    /*     dout->setGrid(surface); */
    /*     updateMeta(dout); */
    /*     task->addObject("data_out", dout); */
    /*     return true; */
    /* } */

    /* DataBase::ptr data_obj_out; */
    /* const auto &dm = haveElementData ? em : vm; */
    /* if(auto data_in = Vec<Scalar, 3>::as(data)) { */
    /*     data_obj_out = remapData<Scalar,3>(data_in, dm); */
    /* } else if(auto data_in = Vec<Scalar,1>::as(data)) { */
    /*     data_obj_out = remapData<Scalar,1>(data_in, dm); */
    /* } else if(auto data_in = Vec<Index,3>::as(data)) { */
    /*     data_obj_out = remapData<Index,3>(data_in, dm); */
    /* } else if(auto data_in = Vec<Index,1>::as(data)) { */
    /*     data_obj_out = remapData<Index,1>(data_in, dm); */
    /* } else if(auto data_in = Vec<Byte,3>::as(data)) { */
    /*     data_obj_out = remapData<Byte,3>(data_in, dm); */
    /* } else if(auto data_in = Vec<Byte,1>::as(data)) { */
    /*     data_obj_out = remapData<Byte,1>(data_in, dm); */
    /* } else { */
    /*     std::cerr << "WARNING: No valid 1D or 3D element data on input Port" << std::endl; */
    /* } */

    /* if (data_obj_out) { */
    /*     data_obj_out->setGrid(surface); */
    /*     data_obj_out->setMeta(data->meta()); */
    /*     data_obj_out->copyAttributes(data); */
    /*     updateMeta(data_obj_out); */
    /*     task->addObject("data_out", data_obj_out); */
    /* } */

    return true;
}

Quads::ptr GhostCellGenerator::createSurface(vistle::StructuredGridBase::const_ptr grid,
                                             GhostCellGenerator::DataMapping &em, bool haveElementData) const
{
    /* auto sgrid = std::dynamic_pointer_cast<const StructuredGrid, const StructuredGridBase>(grid); */

    /* Quads::ptr m_grid_out(new Quads(0, 0)); */
    /* auto &pcl = m_grid_out->cl(); */
    /* Index dims[3] = {grid->getNumDivisions(0), grid->getNumDivisions(1), grid->getNumDivisions(2)}; */

    /* for (int d=0; d<3; ++d) { */
    /*     int d1 = d==0 ? 1 : 0; */
    /*     int d2 = d==d1+1 ? d1+2 : d1+1; */
    /*     assert(d != d1); */
    /*     assert(d != d2); */
    /*     assert(d1 != d2); */

    /*     Index b1 = grid->getNumGhostLayers(d1, StructuredGridBase::Bottom); */
    /*     Index e1 = grid->getNumDivisions(d1); */
    /*     if (grid->getNumGhostLayers(d1, StructuredGridBase::Top)+1 < e1) */
    /*         e1 -= grid->getNumGhostLayers(d1, StructuredGridBase::Top)+1; */
    /*     else */
    /*         e1 = 0; */
    /*     Index b2 = grid->getNumGhostLayers(d2, StructuredGridBase::Bottom); */
    /*     Index e2 = grid->getNumDivisions(d2); */
    /*     if (grid->getNumGhostLayers(d2, StructuredGridBase::Top)+1 < e2) */
    /*         e2 -= grid->getNumGhostLayers(d2, StructuredGridBase::Top)+1; */
    /*     else */
    /*         e2 = 0; */

    /*     if (grid->getNumGhostLayers(d, StructuredGridBase::Bottom) == 0) { */
    /*         for (Index i1 = b1; i1 < e1; ++i1) { */
    /*             for (Index i2 = b2; i2 < e2; ++i2) { */
    /*                 Index idx[3]{0,0,0}; */
    /*                 idx[d1] = i1; */
    /*                 idx[d2] = i2; */
    /*                 if (haveElementData) { */
    /*                     em.emplace_back(grid->cellIndex(idx, dims)); */
    /*                 } */
    /*                 pcl.push_back(grid->vertexIndex(idx,dims)); */
    /*                 idx[d1] = i1+1; */
    /*                 pcl.push_back(grid->vertexIndex(idx,dims)); */
    /*                 idx[d2] = i2+1; */
    /*                 pcl.push_back(grid->vertexIndex(idx,dims)); */
    /*                 idx[d1] = i1; */
    /*                 pcl.push_back(grid->vertexIndex(idx,dims)); */
    /*             } */
    /*         } */
    /*     } */
    /*     if (grid->getNumDivisions(d) > 1 && grid->getNumGhostLayers(d, StructuredGridBase::Top) == 0) { */
    /*         for (Index i1 = b1; i1 < e1; ++i1) { */
    /*             for (Index i2 = b2; i2 < e2; ++i2) { */
    /*                 Index idx[3]{0,0,0}; */
    /*                 idx[d] = grid->getNumDivisions(d)-1; */
    /*                 idx[d1] = i1; */
    /*                 idx[d2] = i2; */
    /*                 if (haveElementData) { */
    /*                     --idx[d]; */
    /*                     em.emplace_back(grid->cellIndex(idx, dims)); */
    /*                     idx[d] = grid->getNumDivisions(d)-1; */
    /*                 } */
    /*                 pcl.push_back(grid->vertexIndex(idx,dims)); */
    /*                 idx[d1] = i1+1; */
    /*                 pcl.push_back(grid->vertexIndex(idx,dims)); */
    /*                 idx[d2] = i2+1; */
    /*                 pcl.push_back(grid->vertexIndex(idx,dims)); */
    /*                 idx[d1] = i1; */
    /*                 pcl.push_back(grid->vertexIndex(idx,dims)); */
    /*             } */
    /*         } */

    /*     } */
    /* } */

    /* return m_grid_out; */
    return Quads::ptr();
}

void GhostCellGenerator::renumberVertices(Coords::const_ptr coords, Indexed::ptr poly, DataMapping &vm) const
{
    /* const bool reuseCoord = getIntParameter("reuseCoordinates"); */

    /* if (reuseCoord) { */
    /*    poly->d()->x[0] = coords->d()->x[0]; */
    /*    poly->d()->x[1] = coords->d()->x[1]; */
    /*    poly->d()->x[2] = coords->d()->x[2]; */
    /* } else { */
    /*    vm.clear(); */

    /*    std::map<Index, Index> mapped; */
    /*    for (Index &v: poly->cl()) { */
    /*       if (mapped.emplace(v, vm.size()).second) { */
    /*          Index vv = vm.size(); */
    /*          vm.push_back(v); */
    /*          v=vv; */
    /*       } else { */
    /*          v=mapped[v]; */
    /*       } */
    /*    } */
    /*    mapped.clear(); */

    /*    const Scalar *xcoord = &coords->x()[0]; */
    /*    const Scalar *ycoord = &coords->y()[0]; */
    /*    const Scalar *zcoord = &coords->z()[0]; */
    /*    auto &px = poly->x(); */
    /*    auto &py = poly->y(); */
    /*    auto &pz = poly->z(); */
    /*    px.resize(vm.size()); */
    /*    py.resize(vm.size()); */
    /*    pz.resize(vm.size()); */

    /*    for (Index i=0; i<vm.size(); ++i) { */
    /*       px[i] = xcoord[vm[i]]; */
    /*       py[i] = ycoord[vm[i]]; */
    /*       pz[i] = zcoord[vm[i]]; */
    /*    } */
    /* } */
}

void GhostCellGenerator::renumberVertices(Coords::const_ptr coords, Quads::ptr quad, DataMapping &vm) const
{
    /* const bool reuseCoord = getIntParameter("reuseCoordinates"); */

    /* if (reuseCoord) { */
    /*    quad->d()->x[0] = coords->d()->x[0]; */
    /*    quad->d()->x[1] = coords->d()->x[1]; */
    /*    quad->d()->x[2] = coords->d()->x[2]; */
    /* } else { */
    /*    vm.clear(); */

    /*    std::map<Index, Index> mapped; */
    /*    for (Index &v: quad->cl()) { */
    /*       if (mapped.emplace(v,vm.size()).second) { */
    /*          Index vv=vm.size(); */
    /*          vm.push_back(v); */
    /*          v = vv; */
    /*       } else { */
    /*          v=mapped[v]; */
    /*       } */
    /*    } */
    /*    mapped.clear(); */

    /*    const Scalar *xcoord = &coords->x()[0]; */
    /*    const Scalar *ycoord = &coords->y()[0]; */
    /*    const Scalar *zcoord = &coords->z()[0]; */
    /*    auto &px = quad->x(); */
    /*    auto &py = quad->y(); */
    /*    auto &pz = quad->z(); */
    /*    px.resize(vm.size()); */
    /*    py.resize(vm.size()); */
    /*    pz.resize(vm.size()); */

    /*    for (Index i=0; i<vm.size(); ++i) { */
    /*       px[i] = xcoord[vm[i]]; */
    /*       py[i] = ycoord[vm[i]]; */
    /*       pz[i] = zcoord[vm[i]]; */
    /*    } */
    /* } */
}

void GhostCellGenerator::createVertices(StructuredGridBase::const_ptr grid, Quads::ptr quad, DataMapping &vm) const
{
    /* vm.clear(); */
    /* std::map<Index, Index> mapped; */
    /* for (Index &v: quad->cl()) { */
    /*     if (mapped.emplace(v,vm.size()).second) { */
    /*         Index vv=vm.size(); */
    /*         vm.push_back(v); */
    /*         v = vv; */
    /*     } else { */
    /*         v=mapped[v]; */
    /*     } */
    /* } */
    /* mapped.clear(); */

    /* auto &px = quad->x(); */
    /* auto &py = quad->y(); */
    /* auto &pz = quad->z(); */
    /* px.resize(vm.size()); */
    /* py.resize(vm.size()); */
    /* pz.resize(vm.size()); */

    /* for (Index i=0; i<vm.size(); ++i) { */
    /*     Vector3 p = grid->getVertex(vm[i]); */
    /*     px[i] = p[0]; */
    /*     py[i] = p[1]; */
    /*     pz[i] = p[2]; */
    /* } */
}

Polygons::ptr GhostCellGenerator::createSurface(vistle::UnstructuredGrid::const_ptr m_grid_in,
                                                GhostCellGenerator::DataMapping &em, bool haveElementData) const
{
    /* const bool showgho = getIntParameter("ghost"); */
    /* const bool showtet = getIntParameter("tetrahedron"); */
    /* const bool showpyr = getIntParameter("pyramid"); */
    /* const bool showpri = getIntParameter("prism"); */
    /* const bool showhex = getIntParameter("hexahedron"); */
    /* const bool showpol = getIntParameter("polyhedron"); */
    /* const bool showtri = getIntParameter("triangle"); */
    /* const bool showqua = getIntParameter("quad"); */

    /* const Index num_elem = m_grid_in->getNumElements(); */
    /* const Index *el = &m_grid_in->el()[0]; */
    /* const Index *cl = &m_grid_in->cl()[0]; */
    /* const Byte *tl = &m_grid_in->tl()[0]; */
    /* UnstructuredGrid::VertexOwnerList::const_ptr vol=m_grid_in->getVertexOwnerList(); */

    /* Polygons::ptr m_grid_out(new Polygons(0, 0, 0)); */
    /* auto &pl = m_grid_out->el(); */
    /* auto &pcl = m_grid_out->cl(); */

    /* auto nf = m_grid_in->getNeighborFinder(); */
    /* for (Index i=0; i<num_elem; ++i) { */
    /*    const Index elStart = el[i], elEnd = el[i+1]; */
    /*    bool ghost = tl[i] & UnstructuredGrid::GHOST_BIT; */
    /*    if (!showgho && ghost) */
    /*        continue; */
    /*    Byte t = tl[i] & UnstructuredGrid::TYPE_MASK; */
    /*    if (t == UnstructuredGrid::POLYHEDRON) { */
    /*        if (showpol) { */
    /*            Index facestart = InvalidIndex; */
    /*            Index term = 0; */
    /*            for (Index j=elStart; j<elEnd; ++j) { */
    /*                if (facestart == InvalidIndex) { */
    /*                    facestart = j; */
    /*                    term = cl[j]; */
    /*                } else if (cl[j] == term) { */
    /*                    Index numVert = j - facestart; */
    /*                    if (numVert >= 3) { */
    /*                        auto face = &cl[facestart]; */
    /*                        Index neighbour = nf.getNeighborElement(i, face[0], face[1], face[2]); */
    /*                        if (neighbour == InvalidIndex) { */
    /*                            const Index *begin = &face[0], *end=&face[numVert]; */
    /*                            auto rbegin = std::reverse_iterator<const Index *>(end), rend = std::reverse_iterator<const Index *>(begin); */
    /*                            std::copy(rbegin, rend, std::back_inserter(pcl)); */
    /*                            if (haveElementData) */
    /*                                em.emplace_back(i); */
    /*                            pl.push_back(pcl.size()); */
    /*                        } */
    /*                    } */
    /*                    facestart = InvalidIndex; */
    /*                } */
    /*            } */
    /*        } */
    /*    } else { */
    /*        bool show = false; */
    /*        switch(t) { */
    /*        case UnstructuredGrid::PYRAMID: */
    /*            show = showpyr; */
    /*            break; */
    /*        case UnstructuredGrid::PRISM: */
    /*            show = showpri; */
    /*            break; */
    /*        case UnstructuredGrid::TETRAHEDRON: */
    /*            show = showtet; */
    /*            break; */
    /*        case UnstructuredGrid::HEXAHEDRON: */
    /*            show = showhex; */
    /*            break; */
    /*        case UnstructuredGrid::TRIANGLE: */
    /*            show = showtri; */
    /*            break; */
    /*        case UnstructuredGrid::QUAD: */
    /*            show = showqua; */
    /*            break; */
    /*        default: */
    /*            break; */
    /*        } */

    /*        if (show) { */
    /*          const auto numFaces = UnstructuredGrid::NumFaces[t]; */
    /*          const auto &faces = UnstructuredGrid::FaceVertices[t]; */
    /*          for (int f=0; f<numFaces; ++f) { */
    /*             const auto &face = faces[f]; */
    /*             Index neighbour = 0; */
    /*             if (UnstructuredGrid::Dimensionality[t] == 3) */
    /*                 neighbour = nf.getNeighborElement(i, cl[elStart + face[0]], cl[elStart + face[1]], cl[elStart + face[2]]); */
    /*             if (UnstructuredGrid::Dimensionality[t] == 2 || neighbour == InvalidIndex) { */
    /*                const auto facesize = UnstructuredGrid::FaceSizes[t][f]; */
    /*                for (unsigned j=0;j<facesize;++j) { */
    /*                   pcl.push_back(cl[elStart + face[j]]); */
    /*                } */
    /*                if (haveElementData) */
    /*                    em.emplace_back(i); */
    /*                pl.push_back(pcl.size()); */
    /*             } */
    /*          } */
    /*       } */
    /*    } */
    /* } */

    /* if (m_grid_out->getNumElements() == 0) { */
    /*    return Polygons::ptr(); */
    /* } */

    /* return m_grid_out; */
    return Polygons::ptr();
}

//bool GhostCellGenerator::checkNormal(Index v1, Index v2, Index v3, Scalar x_center, Scalar y_center, Scalar z_center) {
//   Scalar *xcoord = m_grid_in->x().data();
//   Scalar *ycoord = m_grid_in->y().data();
//   Scalar *zcoord = m_grid_in->z().data();
//   Scalar a[3], b[3], c[3], n[3];

//   // compute normal of a=v2v1 and b=v2v3
//   a[0] = xcoord[v1] - xcoord[v2];
//   a[1] = ycoord[v1] - ycoord[v2];
//   a[2] = zcoord[v1] - zcoord[v2];
//   b[0] = xcoord[v3] - xcoord[v2];
//   b[1] = ycoord[v3] - ycoord[v2];
//   b[2] = zcoord[v3] - zcoord[v2];
//   n[0] = a[1] * b[2] - b[1] * a[2];
//   n[1] = a[2] * b[0] - b[2] * a[0];
//   n[2] = a[0] * b[1] - b[0] * a[1];

//   // compute vector from base-point to volume-center
//   c[0] = x_center - xcoord[v2];
//   c[1] = y_center - ycoord[v2];
//   c[2] = z_center - zcoord[v2];
//   // look if normal is correct or not
//   if ((c[0] * n[0] + c[1] * n[1] + c[2] * n[2]) > 0)
//       return false;
//   else
//       return true;
//}


MODULE_MAIN(GhostCellGenerator)
