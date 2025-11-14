#ifndef VISTLE_TRIANGULATEVTKM_TRIANGULATEVTKM_H
#define VISTLE_TRIANGULATEVTKM_TRIANGULATEVTKM_H

#include <vistle/vtkm/vtkm_module.h>

class TriangulateVtkm: public vistle::VtkmModule {
public:
    TriangulateVtkm(const std::string &name, int moduleID, mpi::communicator comm);

private:
    std::unique_ptr<viskores::filter::Filter> setUpFilter() const override;
};

#endif // VISTLE_TRIANGULATEVTKM_TRIANGULATEVTKM_H
