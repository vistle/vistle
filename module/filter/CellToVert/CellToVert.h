#ifndef VISTLE_CELLTOVERT_CELLTOVERT_H
#define VISTLE_CELLTOVERT_CELLTOVERT_H

#include <vistle/module/module.h>
#include <vistle/core/vector.h>

class CellToVert: public vistle::Module {
public:
    CellToVert(const std::string &name, int moduleID, mpi::communicator comm);

private:
    static const int NumPorts = 3;

    std::vector<vistle::Port *> m_data_in, m_data_out;

    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;
};

#endif
