#ifndef VISTLE_SPLITDIMENSIONS_SPLITDIMENSIONS_H
#define VISTLE_SPLITDIMENSIONS_SPLITDIMENSIONS_H

#include <vistle/module/module.h>
#include <vistle/core/unstr.h>
#include <vistle/core/polygons.h>
#include <vistle/core/quads.h>

class SplitDimensions: public vistle::Module {
public:
    SplitDimensions(const std::string &name, int moduleID, mpi::communicator comm);

    typedef std::map<vistle::Index, vistle::Index> VerticesMapping;

private:
    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;

    vistle::Polygons::ptr createSurface(vistle::UnstructuredGrid::const_ptr m_grid_in) const;
    vistle::Quads::ptr createSurface(vistle::StructuredGridBase::const_ptr m_grid_in) const;
    void renumberVertices(vistle::Coords::const_ptr coords, vistle::Points::ptr points, VerticesMapping &vm) const;
    void renumberVertices(vistle::Coords::const_ptr coords, vistle::Indexed::ptr poly, VerticesMapping &vm) const;
    void renumberVertices(vistle::Coords::const_ptr coords, vistle::Quads::ptr quad, VerticesMapping &vm) const;
    void createVertices(vistle::StructuredGridBase::const_ptr grid, vistle::Quads::ptr quad, VerticesMapping &vm) const;

    vistle::IntParameter *p_reuse = nullptr;
    vistle::Port *p_in = nullptr;
    vistle::Port *p_out[4];
};

#endif
