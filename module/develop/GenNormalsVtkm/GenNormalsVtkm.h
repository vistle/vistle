#ifndef VISTLE_GENNORMALSVTKM_GENNORMALSVTKM_H
#define VISTLE_GENNORMALSVTKM_GENNORMALSVTKM_H

#include <vistle/vtkm/vtkm_module.h>

class GenNormalsVtkm: public vistle::VtkmModule {
public:
    GenNormalsVtkm(const std::string &name, int moduleID, mpi::communicator comm);

    std::string getFieldName(int index, bool output = false) const override;
    bool changeParameter(const vistle::Parameter *p) override;

private:
    vistle::IntParameter *m_perVertex = nullptr;
    vistle::IntParameter *m_normalize = nullptr;
    vistle::IntParameter *m_autoOrient = nullptr;
    vistle::IntParameter *m_inward = nullptr;

    std::unique_ptr<viskores::filter::Filter> setUpFilter(const InputData &input) const override;

    vistle::Object::const_ptr prepareOutputGrid(const InputData &input, OutputData &output) const override;

    vistle::DataBase::ptr prepareOutputField(const InputData &input, OutputData &output, int index,
                                             const std::string &fieldName) const override;
};

#endif
