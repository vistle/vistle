#ifndef VISTLE_CELLTOVERTVTKM_CELLTOVERTVTKM_H
#define VISTLE_CELLTOVERTVTKM_CELLTOVERTVTKM_H

#include <vistle/module/module.h>
#include <vistle/core/vector.h>

#ifdef VERTTOCELL
#define CellToVertVtkm VertToCellVtkm
#endif

class CellToVertVtkm: public vistle::Module {
public:
    CellToVertVtkm(const std::string &name, int moduleID, mpi::communicator comm);
    ~CellToVertVtkm();

private:
    static const int NumPorts = 3;

    std::vector<vistle::Port *> m_data_in, m_data_out;

    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;
};

#endif
