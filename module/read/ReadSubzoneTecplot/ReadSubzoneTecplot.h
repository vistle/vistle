#ifndef VISTLE_READSUBZONETECPLOT_READIAGTECPLOT_H
#define VISTLE_READSUBZONETECPLOT_READIAGTECPLOT_H

#include <vistle/module/reader.h>
<<<<<<< HEAD
<<<<<<< HEAD
#include <vistle/core/structuredgrid.h>

namespace vistle {
=======

>>>>>>> a4e5dc83 (New reader for subzone Tecplot (.szplt))
=======
#include <vistle/core/structuredgrid.h>

namespace vistle {
>>>>>>> 0a328e4ca4839a30a171cceeb8087331664e992f
class ReadSubzoneTecplot: public vistle::Reader {
public:
    ReadSubzoneTecplot(const std::string &name, int moduleID, mpi::communicator comm);
    ~ReadSubzoneTecplot();

private:
<<<<<<< HEAD
<<<<<<< HEAD
    static const int NumPorts = 5;
    void* fileHandle;
=======
    static const int NumPorts = 3;
>>>>>>> a4e5dc83 (New reader for subzone Tecplot (.szplt))
=======
    static const int NumPorts = 5;
    int numFiles = 0;
    void *fileHandle;
>>>>>>> 0a328e4ca4839a30a171cceeb8087331664e992f

#if 0
   bool changeParameter(const vistle::Parameter *p) override;
   bool prepare() override;
#endif

    bool examine(const vistle::Parameter *param) override;
    bool read(Token &token, int timestep, int block) override;
<<<<<<< HEAD
<<<<<<< HEAD
    template<typename T = float> 
    Vec<Scalar, 1>::ptr readVariables(void* fileHandle, int32_t numValues, int32_t inputZone, int32_t var);
    //template<typename T>
    vistle::StructuredGrid::ptr createStructuredGrid(void* fileHandle, int32_t inputZone);
    vistle::StringParameter *m_filename;
    vistle::Port *m_grid = nullptr;
    vistle::StringParameter *m_fieldChoice[NumPorts];
    vistle::Port *m_fieldsOut[NumPorts];
};
} // namespace vistle
=======

    vistle::StringParameter *m_filename;
    vistle::Port *m_grid = nullptr;
    vistle::Port *m_p = nullptr;
    vistle::Port *m_rho = nullptr;
    vistle::Port *m_n = nullptr;
    vistle::Port *m_u = nullptr;
    vistle::Port *m_v = nullptr;
};
>>>>>>> a4e5dc83 (New reader for subzone Tecplot (.szplt))
=======
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
>>>>>>> 0a328e4ca4839a30a171cceeb8087331664e992f
#endif
