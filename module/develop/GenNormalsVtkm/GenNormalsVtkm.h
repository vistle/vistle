#ifndef VISTLE_GENNORMALSVTKM_GENNORMALSVTKM_H
#define VISTLE_GENNORMALSVTKM_GENNORMALSVTKM_H

#include <vistle/vtkm/vtkm_module.h>

class GenNormalsVtkm: public vistle::VtkmModule {
public:
    GenNormalsVtkm(const std::string &name, int moduleID, mpi::communicator comm);
    ~GenNormalsVtkm();
    std::string getFieldName(int i, bool output = false) const override;
    bool changeParameter(const vistle::Parameter *p) override;

private:
    vistle::IntParameter *m_perVertex = nullptr;
    vistle::IntParameter *m_normalize = nullptr;
    vistle::IntParameter *m_autoOrient = nullptr;
    vistle::IntParameter *m_inward = nullptr;

    std::unique_ptr<viskores::filter::Filter> setUpFilter() const override;
    vistle::Object::const_ptr prepareOutputGrid(const viskores::cont::DataSet &dataset,
                                                const vistle::Object::const_ptr &inputGrid) const override;
    vistle::DataBase::ptr prepareOutputField(const viskores::cont::DataSet &dataset,
                                             const vistle::Object::const_ptr &inputGrid,
                                             const vistle::DataBase::const_ptr &inputField,
                                             const std::string &fieldName,
                                             const vistle::Object::const_ptr &outputGrid) const override;
};

#endif
