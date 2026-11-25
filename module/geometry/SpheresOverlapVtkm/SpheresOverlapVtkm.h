#ifndef VISTLE_SPHERESOVERLAPVTKM_SPHERESOVERLAPVTKM_H
#define VISTLE_SPHERESOVERLAPVTKM_SPHERESOVERLAPVTKM_H

#include <string>

#include <vistle/vtkm/vtkm_module.h>

/*
    Detects which spheres overlap and creates a line between them.
*/
class SpheresOverlapVtkm: public vistle::VtkmModule {
public:
    SpheresOverlapVtkm(const std::string &name, int moduleID, mpi::communicator comm);

private:
    vistle::FloatParameter *m_radiusCoefficient;
    vistle::IntParameter *m_thicknessDeterminer;

    std::string getFieldName(int index, bool output = false) const override;

    ModuleStatusPtr prepareInputGrid(InputData &input) const override;

    std::unique_ptr<viskores::filter::Filter> setUpFilter(const InputData &input) const override;

    vistle::DataBase::ptr prepareOutputField(const InputData &input, OutputData &output, int index,
                                             const std::string &fieldName) const override;
};

#endif
