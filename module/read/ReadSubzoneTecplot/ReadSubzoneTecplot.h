#ifndef VISTLE_READSUBZONETECPLOT_READIAGTECPLOT_H
#define VISTLE_READSUBZONETECPLOT_READIAGTECPLOT_H

#include <vistle/module/reader.h>
<<<<<<< HEAD
#include <vistle/core/structuredgrid.h>

namespace vistle {
=======

>>>>>>> a4e5dc83 (New reader for subzone Tecplot (.szplt))
class ReadSubzoneTecplot: public vistle::Reader {
public:
    ReadSubzoneTecplot(const std::string &name, int moduleID, mpi::communicator comm);
    ~ReadSubzoneTecplot();

private:
<<<<<<< HEAD
    static const int NumPorts = 5;
    void* fileHandle;
=======
    static const int NumPorts = 3;
>>>>>>> a4e5dc83 (New reader for subzone Tecplot (.szplt))

#if 0
   bool changeParameter(const vistle::Parameter *p) override;
   bool prepare() override;
#endif

    bool examine(const vistle::Parameter *param) override;
    bool read(Token &token, int timestep, int block) override;
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
#endif
