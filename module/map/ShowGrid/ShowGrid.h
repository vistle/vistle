#ifndef VISTLE_SHOWGRID_SHOWGRID_H
#define VISTLE_SHOWGRID_SHOWGRID_H

#include <vistle/module/module.h>
#include <vistle/core/lines.h>

class ShowGrid: public vistle::Module {
public:
    ShowGrid(const std::string &name, int moduleID, mpi::communicator comm);

private:
    bool compute() override;

    vistle::FloatParameter *m_cellScale = nullptr;
    vistle::IntParameter *m_CellNrMin = nullptr;
    vistle::IntParameter *m_CellNrMax = nullptr;
    vistle::StringParameter *m_cells = nullptr;

    vistle::ResultCache<vistle::Lines::ptr> m_cache;
};

#endif
