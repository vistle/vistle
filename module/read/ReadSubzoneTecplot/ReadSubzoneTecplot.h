#ifndef VISTLE_READSUBZONETECPLOT_READSUBZONETECPLOT_H
#define VISTLE_READSUBZONETECPLOT_READSUBZONETECPLOT_H

#include <vistle/module/reader.h>
#include <vistle/core/structuredgrid.h>
#include <string>
#include <vector>
#include <unordered_map>


namespace vistle {
class ReadSubzoneTecplot: public vistle::Reader {
public:
    ReadSubzoneTecplot(const std::string &name, int moduleID, mpi::communicator comm);
    ~ReadSubzoneTecplot() override;

private:
    static const int NumPorts = 5;
    int numFiles = 0;
    // void *fileHandle; commenting to avoid a shared file handle -> gets clobbered by recurrent reads during parallel processing

#if 0
   bool changeParameter(const vistle::Parameter *p) override;
   bool prepare() override;
#endif
    bool examine(const vistle::Parameter *param) override;
    bool read(Token &token, int timestep, int block) override;
    bool prepareRead() override;
    bool finishRead() override;
    Byte tecToVistleType(int tecType);
    Vec<Scalar, 1>::ptr
    readVariables(void *fileHandle, int64_t numValues, const int32_t inputZone,
                  int32_t var); //modified int32_t -> int64_t (numValues) so huge zones dont overflow 32bit
    std::unordered_map<std::string, std::vector<int>> setFieldChoices(void *fileHandle);
    std::vector<int> getIndexOfTecVar(const std::string &varName,
                                      const std::unordered_map<std::string, std::vector<int>> &indicesCombinedVariables,
                                      void *fileHandle) const;
    Vec<Scalar, 3>::ptr combineVarstoOneOutput(Vec<Scalar, 1>::ptr x, Vec<Scalar, 1>::ptr y, Vec<Scalar, 1>::ptr z,
                                               int32_t numValues);
    bool emptyValue(vistle::StringParameter *ch) const;
    template<typename T = float>
    Vec<Scalar, 3>::ptr combineVarstoOneOutput(std::vector<T> x, std::vector<T> y, std::vector<T> z, int32_t numValues);
    std::unordered_map<std::string, std::vector<int>> findSimilarStrings(const std::vector<std::string> &strings);

    vistle::StructuredGrid::ptr createStructuredGrid(void *fileHandle, int32_t inputZone);
    bool inspectDir();

    // orderSolutionTimes and getTimestepForSolutionTime can be used to order the solution times in the fileList
    // if the times are not given in the file names.
    std::unordered_map<int, double> orderSolutionTimes(std::vector<std::string> fileList);
    int getTimestepForSolutionTime(std::unordered_map<int, double> &orderedSolutionTimes, double solutionTime);

    vistle::StringParameter *m_filedir;
    vistle::Port *m_grid = nullptr;
    vistle::StringParameter *m_fieldChoice[NumPorts];
    vistle::Port *m_fieldsOut[NumPorts];
    std::vector<std::string> fileList;
    std::unordered_map<int, double> solutionTimes; // maps timestep to solution time


    std::unordered_map<std::string, std::vector<int>>
        m_indicesCombinedVariables; //cached combined variable indices for parallel-safe reads


    std::vector<std::string> m_varChoices; // cached choice labels

    double time1; // Record start time for debugging MPI issues
};
} // namespace vistle
#endif
