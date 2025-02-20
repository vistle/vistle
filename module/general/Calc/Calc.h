#ifndef CALC_H
#define CALC_H

#include <vistle/module/module.h>
#include <vistle/core/vector.h>
#include <vistle/core/vec.h>

class Calc: public vistle::Module {
public:
    Calc(const std::string &name, int moduleID, mpi::communicator comm);
    ~Calc();

private:
    static const int NumPorts = 3;

    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;
    bool prepare() override;
    bool reduce(int t) override;

    vistle::Port *m_dataIn[NumPorts] = {nullptr, nullptr, nullptr};
    vistle::Port *m_dataOut = nullptr;
    vistle::IntParameter *m_outputType = nullptr;
    vistle::StringParameter *m_dataTerm = nullptr, *m_gridTerm = nullptr, *m_normTerm = nullptr;
    vistle::StringParameter *m_species = nullptr;
};

#endif
