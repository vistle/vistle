#ifndef GHOSTCELLGENERATOR_H
#define GHOSTCELLGENERATOR_H

#include <vistle/module/module.h>
#include <vistle/core/unstr.h>
#include <vistle/core/polygons.h>
#include <vistle/core/quads.h>

class GhostCellGenerator: public vistle::Module {
public:
    GhostCellGenerator(const std::string &name, int moduleID, mpi::communicator comm);
    ~GhostCellGenerator();

    typedef std::vector<vistle::Index> DataMapping;

private:
    bool compute(std::shared_ptr<vistle::BlockTask> task) const override;

    vistle::Polygons::ptr createSurface(vistle::UnstructuredGrid::const_ptr m_grid_in, DataMapping &em,
                                        bool haveElementData) const;
    vistle::Quads::ptr createSurface(vistle::StructuredGridBase::const_ptr m_grid_in, DataMapping &em,
                                     bool haveElementData) const;
    void renumberVertices(vistle::Coords::const_ptr coords, vistle::Indexed::ptr poly, DataMapping &vm) const;
    void renumberVertices(vistle::Coords::const_ptr coords, vistle::Quads::ptr quad, DataMapping &vm) const;
    void createVertices(vistle::StructuredGridBase::const_ptr grid, vistle::Quads::ptr quad, DataMapping &vm) const;
    //bool checkNormal(vistle::Index v1, vistle::Index v2, vistle::Index v3, vistle::Scalar x_center, vistle::Scalar y_center, vistle::Scalar z_center);
};

#endif
