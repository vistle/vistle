#ifndef VISTLE_FILTERNODE_FILTERNODE_H
#define VISTLE_FILTERNODE_FILTERNODE_H

#include <vistle/module/module.h>
#include <vistle/core/vector.h>
#include <vistle/core/vec.h>

class FilterNode: public vistle::Module {
public:
    FilterNode(const std::string &name, int moduleID, mpi::communicator comm);

private:
    virtual bool compute() override;
    virtual bool changeParameter(const vistle::Parameter *p) override;
    vistle::IntParameter *m_criterionParam;
    vistle::IntParameter *m_nodeParam;
    vistle::IntParameter *m_invertParam;
};

#endif
