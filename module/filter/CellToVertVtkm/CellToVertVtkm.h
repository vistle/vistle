#ifndef VISTLE_CELLTOVERTVTKM_CELLTOVERTVTKM_H
#define VISTLE_CELLTOVERTVTKM_CELLTOVERTVTKM_H

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
    //mutable std::array<vistle::DataComponents, NumPorts> m_splits;
    mutable std::array<std::string, NumPorts> m_fieldNames;

    ModuleStatusPtr prepareInput(const std::shared_ptr<vistle::BlockTask> &task, vistle::Object::const_ptr &grid,
                                 vistle::DataBase::const_ptr &field, vtkm::cont::DataSet &dataset,
                                 std::array<vistle::DataComponents, NumPorts> &m_splits) const;
    void runFilter(vtkm::cont::DataSet &input, vtkm::cont::DataSet &output) const override;
    bool prepareOutput(const std::shared_ptr<vistle::BlockTask> &task, vtkm::cont::DataSet &dataset,
                       vistle::Object::ptr &outputGrid, vistle::Object::const_ptr &inputGrid,
                       vistle::DataBase::const_ptr &inputField,
                       std::array<vistle::DataComponents, NumPorts> &m_splits) const;
    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;
};


#endif // CELLTOVERTVTKM_H
