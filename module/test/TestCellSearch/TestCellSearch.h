#ifndef TESTCELLSEARCH_H
#define TESTCELLSEARCH_H

#include <vistle/module/module.h>

class TestCellSearch: public vistle::Module {
public:
    TestCellSearch(const std::string &name, int moduleID, mpi::communicator comm);
    ~TestCellSearch();

private:
    virtual bool compute();

    vistle::VectorParameter *m_point;
    vistle::IntParameter *m_block, *m_cell;
    vistle::IntParameter *m_createCelltree;
};

#endif
