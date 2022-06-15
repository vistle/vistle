#ifndef GENISODAT_H
#define GENISODAT_H

#include <vistle/module/module.h>

class GenIsoDat: public vistle::Module {
public:
    GenIsoDat(const std::string &name, int moduleID, mpi::communicator comm);
    ~GenIsoDat();

private:
    bool prepare() override;

    vistle::IntParameter *m_cellTypeParam;
    vistle::IntParameter *m_caseNumParam;
};

#endif
