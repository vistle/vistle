#ifndef VISTLE_CLIPVTKM_CLIPVTKM_H
#define VISTLE_CLIPVTKM_CLIPVTKM_H

#include <vistle/vtkm/ImplFuncController.h>
#include <vistle/module/module.h>


class ClipVtkm: public vistle::Module {
public:
    ClipVtkm(const std::string &name, int moduleID, mpi::communicator comm);
    ~ClipVtkm();

private:
    vistle::Port *m_dataOut = nullptr;

    vistle::IntParameter *m_flip = nullptr;

    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;
    bool changeParameter(const vistle::Parameter *param) override;

    ImplFuncController m_implFuncControl;
};

#endif
