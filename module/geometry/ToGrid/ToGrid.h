#ifndef TOGRID_H
#define TOGRID_H

#include <vistle/module/module.h>
#include <vistle/core/object.h>
#include <vistle/core/points.h>
#include <vistle/core/unstr.h>

class ToGrid: public vistle::Module {
public:
    ToGrid(const std::string &name, int moduleID, mpi::communicator comm);
    ~ToGrid();

private:
    virtual bool compute();
    template<unsigned Dim>
    vistle::Object::ptr calculateGrid(vistle::Points::const_ptr points,
                                      typename vistle::Vec<vistle::Scalar, Dim>::const_ptr data);
    vistle::Points::const_ptr m_points;
    vistle::UnstructuredGrid::const_ptr m_grid;
};

#endif
