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
    ModuleStatusPtr CheckField(const vistle::Object::const_ptr &grid, const vistle::DataBase::const_ptr &field,
                               std::string &portName) const;
    ModuleStatusPtr TransformField(const vistle::Object::const_ptr &grid, const vistle::DataBase::const_ptr &field,
                                   std::string &fieldName, vtkm::cont::DataSet &dataset) const;

    void runFilter(vtkm::cont::DataSet &input, std::string &fieldName, vtkm::cont::DataSet &output) const;

    bool prepareOutput(const std::shared_ptr<vistle::BlockTask> &task, vistle::Port *port, vtkm::cont::DataSet &dataset,
                       vistle::Object::ptr &outputGrid, vistle::Object::const_ptr &inputGrid,
                       vistle::DataBase::const_ptr &inputField, std::string &fieldName,
                       vistle::DataBase::ptr &outputField) const;
};

#endif // CELLTOVERTVTKM_H
