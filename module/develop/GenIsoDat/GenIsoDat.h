#ifndef VISTLE_GENISODAT_GENISODAT_H
#define VISTLE_GENISODAT_GENISODAT_H

#include <vistle/module/module.h>

class GenIsoDat: public vistle::Module {
public:
    GenIsoDat(const std::string &name, int moduleID, mpi::communicator comm);

private:
    bool prepare() override;

    vistle::IntParameter *m_cellTypeParam;
    vistle::IntParameter *m_caseNumParam;
};

#endif
