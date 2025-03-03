#ifndef VISTLE_CELLTOVERTVTKM_CELLTOVERTVTKM_H
#define VISTLE_CELLTOVERTVTKM_CELLTOVERTVTKM_H

#include <vistle/vtkm/VtkmModule.h>
#include <vistle/core/vector.h>

#ifdef VERTTOCELL
#define CellToVertVtkm VertToCellVtkm
#endif

class CellToVertVtkm: public VtkmModule {
public:
    CellToVertVtkm(const std::string &name, int moduleID, mpi::communicator comm);
    ~CellToVertVtkm();

private:
    static const int NumPorts = 3;

    std::vector<vistle::Port *> m_inputPorts, m_outputPorts;

    mutable std::array<vistle::DataComponents, NumPorts> m_splits;
    mutable std::array<std::string, NumPorts> m_fieldNames;

    ModuleStatusPtr prepareInput(const std::shared_ptr<vistle::BlockTask> &task, vistle::Object::const_ptr &grid,
                                 vistle::DataBase::const_ptr &field, vtkm::cont::DataSet &dataset) const override;

    bool prepareOutput(const std::shared_ptr<vistle::BlockTask> &task, vtkm::cont::DataSet &dataset,
                       vistle::Object::ptr &outputGrid, vistle::Object::const_ptr &inputGrid,
                       vistle::DataBase::const_ptr &inputField) const override;

    void runFilter(vtkm::cont::DataSet &input, vtkm::cont::DataSet &output) const override;
};

#endif
