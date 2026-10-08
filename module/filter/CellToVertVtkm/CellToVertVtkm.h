#ifndef VISTLE_CELLTOVERTVTKM_CELLTOVERTVTKM_H
#define VISTLE_CELLTOVERTVTKM_CELLTOVERTVTKM_H

#include <vistle/vtkm/vtkm_module.h>

#ifdef VERTTOCELL
#define CellToVertVtkm VertToCellVtkm
#endif

class CellToVertVtkm: public vistle::VtkmModule {
public:
    CellToVertVtkm(const std::string &name, int moduleID, mpi::communicator comm);

private:
    ModuleStatusPtr prepareInputField(const vistle::Port *port, InputData &input, int index) const override;

    std::unique_ptr<viskores::filter::Filter> setUpFilter(const InputData &input) const override;

    vistle::Object::const_ptr prepareOutputGrid(const InputData &input, OutputData &output) const override;

    vistle::DataBase::ptr prepareOutputField(const InputData &input, OutputData &output, int index,
                                             const std::string &fieldName) const override;
};

#endif // CELLTOVERTVTKM_H
