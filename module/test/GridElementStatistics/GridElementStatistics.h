#include <vistle/module/module.h>

class GridElementStatistics: public vistle::Module {
public:
    GridElementStatistics(const std::string &name, int moduleID, mpi::communicator comm);
    bool compute() override;

private:
    vistle::IntParameter *m_elementIndex;
};
