#ifndef INDEXMANIFOLDS_H
#define INDEXMANIFOLDS_H

#include <vistle/module/module.h>

class IndexManifolds: public vistle::Module {
public:
    IndexManifolds(const std::string &name, int moduleID, mpi::communicator comm);
    ~IndexManifolds();

private:
    bool compute(std::shared_ptr<vistle::PortTask> task) const override;

    vistle::Port *p_data_in = nullptr;
    vistle::Port *p_surface_out = nullptr, *p_line_out = nullptr, *p_point_out = nullptr;

    vistle::IntVectorParameter *p_coord = nullptr;
    vistle::IntParameter *p_direction = nullptr;
};

#endif
