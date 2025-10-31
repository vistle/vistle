#ifndef VISTLE_TESTCELLSEARCH_TESTCELLSEARCH_H
#define VISTLE_TESTCELLSEARCH_TESTCELLSEARCH_H

#include <vistle/module/module.h>

class TestCellSearch: public vistle::Module {
public:
    TestCellSearch(const std::string &name, int moduleID, mpi::communicator comm);

private:
    bool compute() override;

    vistle::VectorParameter *m_point;
    vistle::IntParameter *m_block, *m_cell;
    vistle::IntParameter *m_createCelltree;
};

#endif
