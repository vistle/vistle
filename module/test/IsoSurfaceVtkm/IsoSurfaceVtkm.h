#ifndef VISTLE_VTKM_ISOSURFACE_H
#define VISTLE_VTKM_ISOSURFACE_H

#include <vistle/module/module.h>

class IsoSurfaceVtkm: public vistle::Module {
public:
    IsoSurfaceVtkm(const std::string &name, int moduleID, mpi::communicator comm);
    ~IsoSurfaceVtkm();

private:
    vistle::Port *m_dataOut;
    vistle::FloatParameter *m_isovalue;
    // for testing it can be specified whether to use array handles
    // or std::vectors to create the vtkm dataset
    vistle::IntParameter *m_useArrayHandles = nullptr;

    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;
};

#endif // VISTLE_VTKM_ISOSURFACE_H