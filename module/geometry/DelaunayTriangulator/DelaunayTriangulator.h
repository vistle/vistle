/*
Generate an unstructured grid from vertices using the 3D Delaunay Triangulator TetGen
https://wias-berlin.de/software/index.jsp?id=TetGen&lang=1
*/


#ifndef TOGRID_H
#define TOGRID_H

#include <vistle/module/module.h>
#include <vistle/core/object.h>
#include <vistle/core/points.h>
#include <vistle/core/unstr.h>

class DelaunayTriangulator: public vistle::Module {
public:
    DelaunayTriangulator(const std::string &name, int moduleID, mpi::communicator comm);
    ~DelaunayTriangulator();

private:
    virtual bool compute();
    template<unsigned Dim>
    vistle::Object::ptr calculateGrid(vistle::Points::const_ptr points,
                                      typename vistle::Vec<vistle::Scalar, Dim>::const_ptr data);
    vistle::Points::const_ptr m_points;
    vistle::UnstructuredGrid::const_ptr m_grid;
};

#endif
