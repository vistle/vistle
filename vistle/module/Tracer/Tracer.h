#ifndef TRACER_H
#define TRACER_H
#include <core/unstr.h>
#include <module/module.h>

class Tracer: public vistle::Module {

public:
    Tracer(const std::string &shmname, int rank, int size, int moduleID);
    ~Tracer();


 private:
    virtual bool compute();
    virtual bool prepare();
    virtual bool reduce(int timestep);
    std::vector<vistle::UnstructuredGrid::const_ptr> grid_in;
    std::vector<vistle::Vec<vistle::Scalar,3>::const_ptr> data_in0;
    std::vector<vistle::Vec<vistle::Scalar>::const_ptr> data_in1;
    vistle::Vector3 Integration(const vistle::Vector3 &p0, const vistle::Vector3 &v0, const vistle::Scalar &dt);
    vistle::Index findCell(const vistle::Vector3 &point,
                           vistle::UnstructuredGrid::const_ptr grid,
                           std::array<vistle::Vector3, 2> boundingbox,
                           vistle::Index lastcell);

};

#endif
