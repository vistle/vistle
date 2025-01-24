#ifndef CUTTING_SURFACE_VTKM_H
#define CUTTING_SURFACE_VTKM_H

#include <vistle/module/module.h>

class CuttingSurfaceVtkm: public vistle::Module {
public:
    CuttingSurfaceVtkm(const std::string &name, int moduleID, mpi::communicator comm);
    ~CuttingSurfaceVtkm();

private:
    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;

    vistle::Port *m_mapDataIn, *m_dataOut;

    vistle::VectorParameter *m_point;
    vistle::VectorParameter *m_vertex;
    vistle::FloatParameter *m_scalar;
    vistle::IntParameter *m_option;
    vistle::VectorParameter *m_direction;
    vistle::IntParameter *m_computeNormals;
};

#endif // CUTTING_SURFACE_VTKM_H
