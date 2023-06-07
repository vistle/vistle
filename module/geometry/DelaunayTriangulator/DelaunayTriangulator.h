#ifndef DELAUNAYTRIANGULATOR_H
#define DELAUNAYTRIANGULATOR_H

/*
Generate an unstructured grid from vertices using the 3D Delaunay Triangulator TetGen
https://wias-berlin.de/software/index.jsp?id=TetGen&lang=1
*/


#include <vistle/module/module.h>
#include <vistle/module/resultcache.h>
#include <vistle/core/object.h>
#include <vistle/core/points.h>
#include <vistle/core/unstr.h>

class DelaunayTriangulator: public vistle::Module {
public:
    DelaunayTriangulator(const std::string &name, int moduleID, mpi::communicator comm);
    ~DelaunayTriangulator();

private:
    bool compute() override;
    template<unsigned Dim>
    vistle::Object::ptr calculateGrid(vistle::Points::const_ptr points,
                                      typename vistle::Vec<vistle::Scalar, Dim>::const_ptr data) const;

    vistle::IntParameter *m_methodChoice = nullptr;

    vistle::ResultCache<vistle::Object::ptr> m_results;
};

#endif
