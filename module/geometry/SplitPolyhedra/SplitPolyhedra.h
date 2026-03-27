#ifndef VISTLE_SPLITPOLYHEDRA_SPLITPOLYHEDRA_H
#define VISTLE_SPLITPOLYHEDRA_SPLITPOLYHEDRA_H

#include <vistle/module/module.h>
#include <vistle/core/unstr.h>

#include <vector>

class SplitPolyhedra: public vistle::Module {
public:
    SplitPolyhedra(const std::string &name, int moduleID, mpi::communicator comm);

private:
    bool prepare() override;
    bool compute() override;
    vistle::IntParameter *m_mode = nullptr;
    static const unsigned NumPorts = 3;
    std::array<vistle::Port *, NumPorts> m_inPorts;
    std::array<vistle::Port *, NumPorts> m_outPorts;
    struct Result {
        vistle::UnstructuredGrid::ptr simple;
        std::vector<vistle::Index> elementMapping;
    };
    vistle::ResultCache<Result> m_grids;
};

#endif
