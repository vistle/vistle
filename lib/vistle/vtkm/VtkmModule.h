#ifndef VISTLE_VTKM_VTKMMODULE_H
#define VISTLE_VTKM_VTKMMODULE_H

#include <vistle/module/module.h>

class VtkmModule: public vistle::Module {
public:
    VtkmModule(const std::string &name, int moduleID, mpi::communicator comm);
    ~VtkmModule();

private:
    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;

    bool inputToVistle(const std::shared_ptr<vistle::BlockTask> &task) const;
    bool outputToVtkm(const std::shared_ptr<vistle::BlockTask> &task) const;
    virtual bool runFilter() const = 0;

    vistle::Port *m_mapDataIn, *m_dataOut;
};

#endif // VISTLE_VTKM_VTKMMODULE_H