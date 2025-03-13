#ifndef VISTLE_CELLTOVERTVTKM_CELLTOVERTVTKM_H
#define VISTLE_CELLTOVERTVTKM_CELLTOVERTVTKM_H

#include <array>
#include <vistle/vtkm/VtkmModule2.h>

//TODO: add TestVertToCellVtkm to CMakeLists.txt!
#ifdef VERTTOCELL
#define CellToVertVtkm TestVertToCellVtkm
#endif

class CellToVertVtkm: public VtkmModule2 {
public:
    CellToVertVtkm(const std::string &name, int moduleID, mpi::communicator comm);
    ~CellToVertVtkm();

private:
    ModuleStatusPtr checkInputField(const vistle::Object::const_ptr &grid, const vistle::DataBase::const_ptr &field,
                                    const std::string &portName) const override;
    ModuleStatusPtr transformInputField(const vistle::Object::const_ptr &grid, const vistle::DataBase::const_ptr &field,
                                        std::string &fieldName, vtkm::cont::DataSet &dataset) const override;

    void runFilter(vtkm::cont::DataSet &input, std::string &fieldName, vtkm::cont::DataSet &output) const override;

    vistle::Object::ptr prepareOutputGrid(vtkm::cont::DataSet &dataset, const vistle::Object::const_ptr &inputGrid,
                                          vistle::Object::ptr &outputGrid) const override;
    vistle::DataBase::ptr prepareOutputField(vtkm::cont::DataSet &dataset,
                                             const vistle::DataBase::const_ptr &inputField, std::string &fieldName,
                                             const vistle::Object::const_ptr &inputGrid,
                                             vistle::Object::ptr &outputGrid) const override;
    void addResultToPort(const std::shared_ptr<vistle::BlockTask> &task, vistle::Port *port,
                         vistle::Object::ptr &outputGrid, vistle::DataBase::ptr &outputField,
                         vistle::Object::const_ptr &inputGrid, vistle::DataBase::const_ptr &inputField) const override;
};

#endif // CELLTOVERTVTKM_H
