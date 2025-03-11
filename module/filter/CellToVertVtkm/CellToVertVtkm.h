#ifndef VISTLE_CELLTOVERTVTKM_CELLTOVERTVTKM_H
#define VISTLE_CELLTOVERTVTKM_CELLTOVERTVTKM_H

#include <array>
#include <vistle/vtkm/VtkmModule.h>

//TODO: add TestVertToCellVtkm to CMakeLists.txt!
#ifdef VERTTOCELL
#define CellToVertVtkm TestVertToCellVtkm
#endif

class CellToVertVtkm: public VtkmModule {
public:
    CellToVertVtkm(const std::string &name, int moduleID, mpi::communicator comm);
    ~CellToVertVtkm();

private:
    static const int NumPorts = 3;

    std::vector<vistle::Port *> m_inputPorts, m_outputPorts;

    ModuleStatusPtr ReadInPorts(const std::shared_ptr<vistle::BlockTask> &task, vistle::Object::const_ptr &grid,
                                std::array<vistle::DataBase::const_ptr, NumPorts> &fields) const;
    ModuleStatusPtr transformInputField(vistle::DataBase::const_ptr &field, std::string &fieldName,
                                        vtkm::cont::DataSet &dataset, const std::string &portName = "") const;
    ModuleStatusPtr prepareInputField(vistle::Object::const_ptr &grid, vistle::DataBase::const_ptr &field,
                                      std::string &fieldName, vtkm::cont::DataSet &dataset) const;


    void runFilter(vtkm::cont::DataSet &input, vtkm::cont::DataSet &output) const override;
    void runFilter(vtkm::cont::DataSet &input, std::string &fieldName, vtkm::cont::DataSet &output) const;


    bool prepareOutput(vtkm::cont::DataSet &dataset, std::string &fieldName, vistle::Object::const_ptr &inputGrid,
                       vistle::DataBase::const_ptr &inputField, vistle::Object::ptr &outputGrid,
                       vistle::DataBase::ptr &outputField) const;
    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;
};

#endif // CELLTOVERTVTKM_H
