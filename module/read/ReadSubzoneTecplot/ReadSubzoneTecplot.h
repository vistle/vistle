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
    static const int NumPorts = 3;
    void* fileHandle;

#if 0
   bool changeParameter(const vistle::Parameter *p) override;
   bool prepare() override;
#endif

    bool examine(const vistle::Parameter *param) override;
    bool read(Token &token, int timestep, int block) override;
    //template<typename T> 
    //void readVariables(std::vector<T> &values, void* fileHandle, int32_t numValues, int32_t inputZone, int32_t var);
    //template<typename T>
    vistle::StructuredGrid::ptr createStructuredGrid(void* fileHandle, int32_t inputZone);

    vistle::StringParameter *m_filename;
    vistle::Port *m_grid = nullptr;
    vistle::Port *m_p = nullptr;
    vistle::Port *m_rho = nullptr;
    vistle::Port *m_n = nullptr;
    vistle::Port *m_u = nullptr;
    vistle::Port *m_v = nullptr;
};
} // namespace vistle
#endif
