#ifndef VISTLE_SHOWCELLTREE_SHOWCELLTREE_H
#define VISTLE_SHOWCELLTREE_SHOWCELLTREE_H

#include <vistle/module/module.h>

class ShowCelltree: public vistle::Module {
public:
    ShowCelltree(const std::string &name, int moduleID, mpi::communicator comm);

private:
    vistle::IntParameter *m_validate, *m_minDepth, *m_maxDepth, *m_showLeft, *m_showRight;
    virtual bool compute();
};

#endif
