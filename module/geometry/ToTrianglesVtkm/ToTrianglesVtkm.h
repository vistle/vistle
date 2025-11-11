#ifndef VISTLE_TOTRIANGLESVTKM_TOTRIANGLESVTKM_H
#define VISTLE_TOTRIANGLESVTKM_TOTRIANGLESVTKM_H

#include <vistle/vtkm/vtkm_module.h>

class ToTrianglesVtkm: public vistle::VtkmModule {
public:
    ToTrianglesVtkm(const std::string &name, int moduleID, mpi::communicator comm);

private:
    std::unique_ptr<viskores::filter::Filter> setUpFilter() const override;
};

#endif // VISTLE_TOTRIANGLESVTKM_TOTRIANGLESVTKM_H
