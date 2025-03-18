#ifndef VISTLE_CELLTOVERTVTKM_CELLTOVERTVTKM_H
#define VISTLE_CELLTOVERTVTKM_CELLTOVERTVTKM_H

#include <array>
#include <vistle/vtkm/VtkmModule.h>

#ifdef VERTTOCELL
#define CellToVertVtkm TestVertToCellVtkm
#endif

class CellToVertVtkm: public VtkmModule {
public:
    CellToVertVtkm(const std::string &name, int moduleID, mpi::communicator comm);
    ~CellToVertVtkm();

private:
    ModuleStatusPtr checkInputField(const vistle::Object::const_ptr &grid, const vistle::DataBase::const_ptr &field,
                                    const std::string &portName) const override;

    ModuleStatusPtr transformInputField(const vistle::Object::const_ptr &grid, const vistle::DataBase::const_ptr &field,
                                        std::string &fieldName, vtkm::cont::DataSet &dataset) const override;

    void runFilter(const vtkm::cont::DataSet &input, const std::string &fieldName,
                   vtkm::cont::DataSet &output) const override;

    vistle::Object::ptr prepareOutputGrid(const vtkm::cont::DataSet &dataset,
                                          const vistle::Object::const_ptr &inputGrid) const override;

    vistle::DataBase::ptr prepareOutputField(const vtkm::cont::DataSet &dataset,
                                             const vistle::Object::const_ptr &inputGrid,
                                             const vistle::DataBase::const_ptr &inputField,
                                             const std::string &fieldName,
                                             const vistle::Object::ptr &outputGrid) const override;

    void writeResultToPort(const std::shared_ptr<vistle::BlockTask> &task, const vistle::Object::const_ptr &inputGrid,
                           const vistle::DataBase::const_ptr &inputField, vistle::Port *port,
                           vistle::Object::ptr &outputGrid, vistle::DataBase::ptr &outputField) const override;
};

#endif // CELLTOVERTVTKM_H
