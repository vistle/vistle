#ifndef VISTLE_READSUBZONETECPLOT_READIAGTECPLOT_H
#define VISTLE_READSUBZONETECPLOT_READIAGTECPLOT_H

#include <vistle/module/reader.h>
#include <vistle/core/structuredgrid.h>

namespace vistle {
class ReadSubzoneTecplot: public vistle::Reader {
public:
    ReadSubzoneTecplot(const std::string &name, int moduleID, mpi::communicator comm);
    ~ReadSubzoneTecplot();

private:
    static const int NumPorts = 5;
    int numFiles = 0;
    void *fileHandle;

#if 0
   bool changeParameter(const vistle::Parameter *p) override;
   bool prepare() override;
#endif

    bool examine(const vistle::Parameter *param) override;
    bool read(Token &token, int timestep, int block) override;
    template<typename T = float>
    Vec<Scalar, 1>::ptr readVariables(void *fileHandle, int32_t numValues, int32_t inputZone, int32_t var);
    std::unordered_map<std::string, std::vector<size_t>> setFieldChoices(void *fileHandle);
    std::vector<int>
    getIndexOfTecVar(const std::string &varName,
                     const std::unordered_map<std::string, std::vector<size_t>> &indicesCombinedVariables,
                     void *fileHandle) const;
    template<typename T = float>
    Vec<Scalar, 1>::ptr combineVarstoOneOutput(std::vector<T> x, std::vector<T> y, std::vector<T> z, int32_t numValues);
    bool emptyValue(vistle::StringParameter *ch) const;
    std::unordered_map<std::string, std::vector<size_t>> findSimilarStrings(const std::vector<std::string> &strings);
    //template<typename T>
    vistle::StructuredGrid::ptr createStructuredGrid(void *fileHandle, int32_t inputZone);
    bool inspectDir();
    vistle::StringParameter *m_filedir;
    vistle::Port *m_grid = nullptr;
    vistle::StringParameter *m_fieldChoice[NumPorts];
    vistle::Port *m_fieldsOut[NumPorts];
    std::vector<std::string> fileList;
};
} // namespace vistle
#endif
